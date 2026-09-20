#include "app_runtime.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "weather_service.hpp"

#include <mutex>

namespace {
std::mutex polling_mutex;
TaskHandle_t polling_task{};
void poll_weather(void*) {
  auto last = xTaskGetTickCount();
  while (true) {
    spring::weather::refresh();
    xTaskDelayUntil(&last, pdMS_TO_TICKS(600'000));
  }
}
} // namespace
void spring::app::start_weather_polling() {
  std::lock_guard lock(polling_mutex);
  if (polling_task)
    return;
  if (xTaskCreate(poll_weather, "weather_poll", 8192, nullptr, 3, &polling_task) != pdPASS) {
    polling_task = nullptr;
    ESP_LOGE("weather", "polling task allocation failed");
  }
}
