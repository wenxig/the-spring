#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "app_runtime.hpp"
#include "clock_service.hpp"
#include "input_service.hpp"
#include "at_engine.hpp"
#include "display_service.hpp"

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
  vTaskDelay(pdMS_TO_TICKS(1000));
}
