#include "esp_log.h"

namespace {
constexpr char kTag[] = "app_manager";
}

extern "C" void app_manager_start() {
  ESP_LOGI(kTag, "application runtime ready");
}
