#include "esp_log.h"
#include "app_runtime.hpp"

namespace {
constexpr char kTag[] = "app_manager";
}

void spring::app::start() {
  ESP_LOGI(kTag, "application runtime ready");
}

bool spring::app::register_application(Application& application) {
  ESP_LOGI(kTag, "registered application: %s", application.name());
  return true;
}

bool spring::app::navigate_home() { return true; }
