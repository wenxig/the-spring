#include "storage_service.hpp"

#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"

namespace {
constexpr char kTag[] = "storage";
bool mounted = false;
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
  ESP_LOGI(kTag, "SD card mount: %s", esp_err_to_name(result));
  return mounted;
}

bool spring::storage::append_event(std::string_view type, std::string_view payload) {
  if (!mounted || type.empty() || payload.empty()) return false;
  return true;
}
