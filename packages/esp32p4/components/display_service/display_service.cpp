#include "display_service.hpp"
#include "driver/gpio.h"

namespace {
bool baseline_valid = false;
}

void spring::display::start() {
  gpio_set_direction(GPIO_NUM_22, GPIO_MODE_INPUT);
  gpio_set_direction(GPIO_NUM_21, GPIO_MODE_OUTPUT);
  gpio_set_direction(GPIO_NUM_6, GPIO_MODE_OUTPUT);
  gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);
  gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
  gpio_set_direction(GPIO_NUM_3, GPIO_MODE_OUTPUT);
  baseline_valid = false;
}

void spring::display::invalidate(Rect area) {
  (void)area;
  if (!baseline_valid) force_full_refresh();
}

void spring::display::force_full_refresh() { baseline_valid = true; }
