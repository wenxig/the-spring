#include "input_service.hpp"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <cstddef>

namespace {
constexpr gpio_num_t kPins[] = {GPIO_NUM_4, GPIO_NUM_20, GPIO_NUM_23, GPIO_NUM_26,
                                GPIO_NUM_27, GPIO_NUM_46, GPIO_NUM_47, GPIO_NUM_48};
QueueHandle_t events = nullptr;
void scan(void*) {
  bool previous[8]{};
  while (true) {
    for (std::size_t index = 0; index < 8; ++index) {
      const bool pressed = gpio_get_level(kPins[index]) == 0;
      if (pressed && !previous[index]) {
        const auto key = static_cast<spring::input::Key>(index);
        xQueueSend(events, &key, 0);
      }
      previous[index] = pressed;
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}
}

void spring::input::start() {
  events = xQueueCreate(16, sizeof(Key));
  for (const auto pin : kPins) {
    gpio_config_t config{};
    config.pin_bit_mask = 1ULL << pin;
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLUP_ENABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&config);
  }
  xTaskCreate(scan, "input_scan", 3072, nullptr, 5, nullptr);
}

bool spring::input::next(Key& key, std::uint32_t timeout_ms) {
  return events != nullptr && xQueueReceive(events, &key, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}
