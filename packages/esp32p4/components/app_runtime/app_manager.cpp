#include "esp_log.h"
#include "app_runtime.hpp"

namespace {
constexpr char kTag[] = "app_manager";
spring::app::Application* current = nullptr;
}

void spring::app::start() {
  ESP_LOGI(kTag, "application runtime ready");
}

bool spring::app::register_application(Application& application) {
  ESP_LOGI(kTag, "registered application: %s", application.name());
  if (current == nullptr) current = &application;
  return true;
}

bool spring::app::navigate_home() {
  if (current == nullptr) return false;
  current->on_event(0);
  return true;
}
