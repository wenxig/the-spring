#include "storage_service.hpp"

#include "driver/gpio.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sqlite3.h"
#include <sys/stat.h>

namespace {
constexpr char kTag[] = "storage";
bool mounted = false;
sqlite3* database = nullptr;
}

bool spring::storage::mount_sdcard() {
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  host.max_freq_khz = SDMMC_FREQ_PROBING;  // Use lowest frequency (400kHz) for maximum compatibility
  sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
  slot.width = 1;  // Use 1-bit mode for compatibility
  slot.clk = GPIO_NUM_43;
  slot.cmd = GPIO_NUM_44;
  slot.d0 = GPIO_NUM_39;
  slot.d1 = GPIO_NUM_40;
  slot.d2 = GPIO_NUM_41;
  slot.d3 = GPIO_NUM_42;
  slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;  // Enable internal pull-ups
  esp_vfs_fat_sdmmc_mount_config_t config{};
  config.format_if_mount_failed = false;
  config.max_files = 8;
  config.allocation_unit_size = 16 * 1024;
  config.disk_status_check_enable = false;
  sdmmc_card_t* card = nullptr;
  const auto result = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot, &config, &card);
  mounted = result == ESP_OK;
  if (mounted) {
    mkdir("/sdcard/data", 0755);
  }
  if (mounted && sqlite3_open("/sdcard/data/spring.sqlite3", &database) == SQLITE_OK) {
    sqlite3_busy_timeout(database, 3000);
    sqlite3_exec(database, "PRAGMA journal_mode=WAL; PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);
    constexpr char schema[] =
        "CREATE TABLE IF NOT EXISTS events (id INTEGER PRIMARY KEY, type TEXT NOT NULL, "
        "payload TEXT NOT NULL, created_at INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS snapshots (revision INTEGER PRIMARY KEY, payload TEXT NOT NULL);";
    mounted = sqlite3_exec(database, schema, nullptr, nullptr, nullptr) == SQLITE_OK;
  }
  ESP_LOGI(kTag, "SD card mount: %s", esp_err_to_name(result));
  return mounted;
}

bool spring::storage::append_event(std::string_view type, std::string_view payload) {
  if (!mounted || type.empty() || payload.empty()) return false;
  sqlite3_stmt* statement = nullptr;
  constexpr char sql[] = "INSERT INTO events(type,payload,created_at) VALUES(?,?,unixepoch());";
  if (sqlite3_exec(database, "BEGIN;", nullptr, nullptr, nullptr) != SQLITE_OK) return false;
  if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK) {
    sqlite3_exec(database, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }
  sqlite3_bind_text(statement, 1, type.data(), static_cast<int>(type.size()), SQLITE_TRANSIENT);
  sqlite3_bind_text(statement, 2, payload.data(), static_cast<int>(payload.size()), SQLITE_TRANSIENT);
  const bool success = sqlite3_step(statement) == SQLITE_DONE;
  sqlite3_finalize(statement);
  sqlite3_exec(database, success ? "COMMIT;" : "ROLLBACK;", nullptr, nullptr, nullptr);
  return success;
}
