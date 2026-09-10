#include "display_service.hpp"
#include "driver/gpio.h"
#include <algorithm>

namespace {
bool baseline_valid = false;
std::uint8_t frame[spring::display::kFrameBytes]{};
spring::display::Rect dirty{0, 0, 0, 0};
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
  if (dirty.width == 0 || dirty.height == 0) {
    dirty = area;
  } else {
    const auto right = std::max<std::uint16_t>(dirty.x + dirty.width, area.x + area.width);
    const auto bottom = std::max<std::uint16_t>(dirty.y + dirty.height, area.y + area.height);
    dirty.x = std::min(dirty.x, area.x);
    dirty.y = std::min(dirty.y, area.y);
    dirty.width = right - dirty.x;
    dirty.height = bottom - dirty.y;
  }
  if (!baseline_valid) force_full_refresh();
}

void spring::display::force_full_refresh() { baseline_valid = true; }

spring::display::Rect spring::display::pending_area() { return dirty; }

bool spring::display::write_pixel(std::uint16_t x, std::uint16_t y, bool black) {
  if (x >= 400 || y >= 300) return false;
  const auto index = static_cast<std::size_t>(y) * 50U + x / 8U;
  const auto mask = static_cast<std::uint8_t>(0x80U >> (x % 8U));
  if (black) frame[index] |= mask;
  else frame[index] &= static_cast<std::uint8_t>(~mask);
  invalidate({x, y, 1, 1});
  return true;
}
