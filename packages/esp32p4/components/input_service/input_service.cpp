#include "input_service.hpp"
#include "driver/gpio.h"

namespace {
constexpr gpio_num_t kPins[] = {GPIO_NUM_4, GPIO_NUM_20, GPIO_NUM_23, GPIO_NUM_26,
                                GPIO_NUM_27, GPIO_NUM_46, GPIO_NUM_47, GPIO_NUM_48};
}

void spring::input::start() {
  for (const auto pin : kPins) {
    gpio_config_t config{};
    config.pin_bit_mask = 1ULL << pin;
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLUP_ENABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&config);
  }
}
