#include "storage_service.hpp"

#include "cJSON.h"
#include "driver/gpio.h"
#include "driver/sdmmc_defs.h"
#include "driver/sdmmc_host.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sd_pwr_ctrl_by_on_chip_ldo.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace {
constexpr char kTag[] = "storage";
constexpr char kDbDir[] = "/sdcard/db";
constexpr char kEventsFile[] = "/sdcard/db/events.json";
bool mounted = false;
sd_pwr_ctrl_handle_t pwr_ctrl_handle = nullptr;

constexpr std::array<const char*, 11> kUiAssets{
    "/sdcard/fonts/HYWenHei-65W-3.ttf",       "/sdcard/icons/sunny-outline.svg",
    "/sdcard/icons/partly-sunny-outline.svg", "/sdcard/icons/cloudy-outline.svg",
    "/sdcard/icons/rainy-outline.svg",        "/sdcard/icons/thunderstorm-outline.svg",
    "/sdcard/icons/snow-outline.svg",         "/sdcard/icons/moon-outline.svg",
    "/sdcard/icons/cloudy-night-outline.svg", "/sdcard/icons/help-circle-outline.svg",
    "/sdcard/icons/location-outline.svg",
};
} // namespace

bool spring::storage::mount_sdcard() {
  // Initialize LDO#4 for SD card power (Waveshare ESP32-P4 specific)
  ESP_LOGI(kTag, "Initializing SD card power (LDO#4 + GPIO45)");
  sd_pwr_ctrl_ldo_config_t ldo_config{};
  ldo_config.ldo_chan_id = 4; // LDO_VO4

  esp_err_t ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(kTag, "Failed to initialize LDO power control: %s", esp_err_to_name(ret));
    return false;
  }

  // GPIO45 controls transistor Q1 (LOW = enable)
  gpio_config_t io_conf{};
  io_conf.pin_bit_mask = (1ULL << GPIO_NUM_45);
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_config(&io_conf);
  gpio_set_level(GPIO_NUM_45, 0); // LOW = enable SD card power

  // Wait for power to stabilize
  vTaskDelay(pdMS_TO_TICKS(200));
  ESP_LOGI(kTag, "SD card power enabled");

  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  host.slot = SDMMC_HOST_SLOT_0;
  // ESP-Hosted owns the shared controller; app_main waits for transport setup.
  host.init = []() -> esp_err_t { return ESP_OK; };
  host.max_freq_khz = SDMMC_FREQ_DEFAULT; // 20MHz
  host.pwr_ctrl_handle = pwr_ctrl_handle; // Pass LDO power control handle

  sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
  slot.width = 4; // Use 4-bit mode for better performance
  slot.clk = GPIO_NUM_43;
  slot.cmd = GPIO_NUM_44;
  slot.d0 = GPIO_NUM_39;
  slot.d1 = GPIO_NUM_40;
  slot.d2 = GPIO_NUM_41;
  slot.d3 = GPIO_NUM_42;
  slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

  esp_vfs_fat_sdmmc_mount_config_t config{};
  config.format_if_mount_failed = false;
  // TinyTTF keeps a file open for each cached font size.
  config.max_files = 16;
  config.allocation_unit_size = 16 * 1024;
  config.disk_status_check_enable = false;

  sdmmc_card_t* card = nullptr;
  const auto result = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot, &config, &card);
  ESP_LOGI(kTag, "SD card mount result: %s", esp_err_to_name(result));

  mounted = result == ESP_OK;
  if (!mounted) {
    ESP_LOGE(kTag, "Failed to mount SD card");
    return false;
  }

  ESP_LOGI(kTag, "SD card mounted successfully, creating directories");
  mkdir("/sdcard/data", 0755);
  mkdir(kDbDir, 0755);

  // Test file write/read
  ESP_LOGI(kTag, "Testing SD card write/read...");
  FILE* f = fopen("/sdcard/data/test.txt", "w");
  if (f == nullptr) {
    ESP_LOGE(kTag, "Failed to open test file for writing");
    mounted = false;
    return false;
  }
  fprintf(f, "Hello from ESP32-P4!\n");
  fclose(f);

  f = fopen("/sdcard/data/test.txt", "r");
  if (f == nullptr) {
    ESP_LOGE(kTag, "Failed to open test file for reading");
    mounted = false;
    return false;
  }
  char line[64];
  fgets(line, sizeof(line), f);
  fclose(f);
  ESP_LOGI(kTag, "SD card test successful, read: %s", line);

  // Initialize events.json if it doesn't exist
  f = fopen(kEventsFile, "r");
  if (f == nullptr) {
    ESP_LOGI(kTag, "Creating events database: %s", kEventsFile);
    f = fopen(kEventsFile, "w");
    if (f) {
      fprintf(f, "[]"); // Empty JSON array
      fclose(f);
    } else {
      ESP_LOGE(kTag, "Failed to create events database");
      mounted = false;
      return false;
    }
  } else {
    fclose(f);
    ESP_LOGI(kTag, "Events database exists: %s", kEventsFile);
  }

  ESP_LOGI(kTag, "Storage service initialized successfully (cJSON KV + SD card)");
  return true;
}

bool spring::storage::ui_assets_ready() {
  if (!mounted) {
    ESP_LOGW(kTag, "UI assets unavailable: SD card is not mounted");
    return false;
  }
  auto ready = true;
  for (const auto* path : kUiAssets) {
    if (access(path, R_OK) != 0) {
      ESP_LOGE(kTag, "Missing UI asset: %s", path);
      ready = false;
    } else {
      ESP_LOGI(kTag, "UI asset present: %s", path);
    }
  }
  ESP_LOGI(kTag, "UI assets %s", ready ? "ready" : "incomplete");
  return ready;
}

bool spring::storage::append_event(std::string_view type, std::string_view payload) {
  if (!mounted || type.empty() || payload.empty())
    return false;

  // Read existing events
  FILE* f = fopen(kEventsFile, "r");
  if (!f) {
    ESP_LOGE(kTag, "Failed to open events file for reading");
    return false;
  }

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  char* json_str = static_cast<char*>(malloc(fsize + 1));
  if (!json_str) {
    fclose(f);
    ESP_LOGE(kTag, "Failed to allocate memory for JSON");
    return false;
  }

  fread(json_str, 1, fsize, f);
  json_str[fsize] = '\0';
  fclose(f);

  // Parse JSON array
  cJSON* events = cJSON_Parse(json_str);
  free(json_str);

  if (!events || !cJSON_IsArray(events)) {
    ESP_LOGE(kTag, "Failed to parse events JSON");
    if (events)
      cJSON_Delete(events);
    return false;
  }

  // Create new event object
  cJSON* event = cJSON_CreateObject();
  cJSON_AddStringToObject(event, "type", std::string(type).c_str());
  cJSON_AddStringToObject(event, "payload", std::string(payload).c_str());
  cJSON_AddNumberToObject(event, "timestamp", static_cast<double>(esp_timer_get_time() / 1000000));

  // Append to array
  cJSON_AddItemToArray(events, event);

  // Write back to file
  char* new_json_str = cJSON_Print(events);
  cJSON_Delete(events);

  if (!new_json_str) {
    ESP_LOGE(kTag, "Failed to serialize JSON");
    return false;
  }

  f = fopen(kEventsFile, "w");
  if (!f) {
    cJSON_free(new_json_str);
    ESP_LOGE(kTag, "Failed to open events file for writing");
    return false;
  }

  fprintf(f, "%s", new_json_str);
  fclose(f);
  cJSON_free(new_json_str);

  return true;
}
