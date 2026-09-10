#include "ota_service.hpp"

#include "esp_ota_ops.h"

bool spring::ota::mark_boot_valid() {
  return esp_ota_mark_app_valid_cancel_rollback() == ESP_OK;
}

bool spring::ota::begin(std::string_view firmware_url) {
  return !firmware_url.empty();
}
