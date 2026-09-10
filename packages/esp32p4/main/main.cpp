#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "app_runtime.hpp"
#include "clock_service.hpp"
#include "input_service.hpp"
#include "at_engine.hpp"
#include "display_service.hpp"
#include "storage_service.hpp"
#include "network_service.hpp"
#include "clock_app.hpp"
#include "ota_service.hpp"
#include "wifi_service.hpp"
#include "power_service.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
void interaction_task(void*) {
  while (true) {
    spring::input::Key key{};
    if (!spring::input::next(key, 1000)) continue;
    if (key == spring::input::Key::home) spring::app::navigate_home();
    if (key == spring::input::Key::sleep) {
      if (spring::power::state() == spring::power::State::sleeping) {
        spring::power::wake();
      } else {
        spring::power::request_sleep();
      }
    }
  }
}
}

namespace {
constexpr char kTag[] = "the_spring";
}

extern "C" void app_main() {
  ESP_LOGI(kTag, "ESP32-P4 core firmware starting");
  spring::clock::start();
  spring::input::start();
  spring::app::start();
  spring::display::start();
  spring::modem::start();
  spring::storage::mount_sdcard();
  spring::network::start();
  spring::wifi::start();
  spring::power::start();
  xTaskCreate(interaction_task, "interaction", 3072, nullptr, 5, nullptr);
  spring::clock_app::register_app();
  spring::ota::mark_boot_valid();
  
  // Test EC600X modem communication
  vTaskDelay(pdMS_TO_TICKS(2000));  // Wait for EC600X to boot
  ESP_LOGI(kTag, "Testing EC600X modem...");
  
  auto result = spring::modem::execute("AT", 3000);
  ESP_LOGI(kTag, "AT command result: %d", static_cast<int>(result));
  
  if (result == spring::modem::Result::ok) {
    ESP_LOGI(kTag, "EC600X responding, querying module info...");
    result = spring::modem::execute("ATI", 3000);
    ESP_LOGI(kTag, "ATI result: %d", static_cast<int>(result));
    
    result = spring::modem::execute("AT+CPIN?", 3000);
    ESP_LOGI(kTag, "AT+CPIN? result: %d", static_cast<int>(result));
    
    result = spring::modem::execute("AT+CSQ", 3000);
    ESP_LOGI(kTag, "AT+CSQ result: %d", static_cast<int>(result));
  } else {
    ESP_LOGE(kTag, "EC600X not responding - check connections and power");
  }
  
  vTaskDelay(pdMS_TO_TICKS(1000));
}
