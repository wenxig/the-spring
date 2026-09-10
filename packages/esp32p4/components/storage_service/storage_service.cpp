#include "storage_service.hpp"

#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sqlite3.h"
#include <sys/stat.h>

namespace {
constexpr char kTag[] = "storage";
bool mounted = false;
sqlite3* database = nullptr;
}

bool spring::storage::mount_sdcard() {
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
  slot.width = 4;
  slot.clk = GPIO_NUM_43;
  slot.cmd = GPIO_NUM_44;
  slot.d0 = GPIO_NUM_39;
  slot.d1 = GPIO_NUM_40;
  slot.d2 = GPIO_NUM_41;
  slot.d3 = GPIO_NUM_42;
  esp_vfs_fat_sdmmc_mount_config_t config{.format_if_mount_failed = false,
                                          .max_files = 8,
                                          .allocation_unit_size = 16 * 1024,
                                          .disk_status_check_enable = false};
  sdmmc_card_t* card = nullptr;
  const auto result = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot, &config, &card);
  mounted = result == ESP_OK;
  if (mounted) {
    mkdir("/sdcard/data", 0755);
  }
  if (mounted && sqlite3_open("/sdcard/data/spring.sqlite3", &database) == SQLITE_OK) {
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
  if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK) return false;
  sqlite3_bind_text(statement, 1, type.data(), static_cast<int>(type.size()), SQLITE_TRANSIENT);
  sqlite3_bind_text(statement, 2, payload.data(), static_cast<int>(payload.size()), SQLITE_TRANSIENT);
  const bool success = sqlite3_step(statement) == SQLITE_DONE;
  sqlite3_finalize(statement);
  return success;
}
