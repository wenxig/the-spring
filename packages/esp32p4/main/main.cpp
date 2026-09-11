#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "app_runtime.hpp"
#include "ui_core.hpp"
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
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
void interaction_task(void*) {
  while (true) {
    spring::input::Key key{};
    if (!spring::input::next(key, 1000)) continue;
    if (spring::power::state() == spring::power::State::sleeping) {
      spring::power::wake();
      spring::app::render();
      continue;
    }
    if (key == spring::input::Key::sleep) {
      spring::app::dispatch(spring::ui::Event::sleep);
      spring::power::request_sleep();
      continue;
    }
    if (key == spring::input::Key::home) spring::app::navigate_home();
    else spring::app::dispatch(static_cast<spring::ui::Event>(key));
  }
}

void clock_refresh_task(void*) {
  auto last_minute = std::uint8_t{0xFF};
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(60'000));
    const auto minute = static_cast<std::uint8_t>((spring::clock::now().unix_seconds / 60) % 60);
    if (spring::power::state() == spring::power::State::active && minute != last_minute) {
      last_minute = minute;
      spring::app::render();
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
#if CONFIG_SPRING_DISPLAY_BUFFER_ONLY
  ESP_LOGI(kTag, "display mode: buffer-only (P6 may be disconnected)");
  spring::display::start(spring::display::Backend::buffer_only);
#else
  ESP_LOGI(kTag, "display mode: epaper (P6 must be connected)");
  spring::display::start(spring::display::Backend::epaper);
#endif
  spring::modem::start();
  spring::storage::mount_sdcard();
  spring::network::start();
  spring::wifi::start();
  spring::power::start();
  xTaskCreate(interaction_task, "interaction", 3072, nullptr, 5, nullptr);
  xTaskCreate(clock_refresh_task, "clock_refresh", 3072, nullptr, 4, nullptr);
  spring::clock_app::register_app();
  spring::app::render();
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
