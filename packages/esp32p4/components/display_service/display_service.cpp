#include "display_service.hpp"
#include "ui_core.hpp"
#include "esp_log.h"
#include "driver/gpio.h"
#include <algorithm>

namespace {
bool baseline_valid = false;
std::uint8_t frame[spring::display::kFrameBytes]{};
spring::ui::Frame committed_frame;
spring::display::Rect dirty{0, 0, 0, 0};
spring::display::Backend active_backend{spring::display::Backend::buffer_only};
}

void spring::display::start(Backend selected) {
  active_backend = selected;
  ESP_LOGI("display", "backend=%s", selected == Backend::epaper ? "epaper" : "buffer-only");
  if (selected == Backend::buffer_only) return;
  gpio_set_direction(GPIO_NUM_22, GPIO_MODE_INPUT);
  gpio_set_direction(GPIO_NUM_21, GPIO_MODE_OUTPUT);
  gpio_set_direction(GPIO_NUM_6, GPIO_MODE_OUTPUT);
  gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);
  gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
  gpio_set_direction(GPIO_NUM_3, GPIO_MODE_OUTPUT);
  baseline_valid = false;
}

spring::display::Backend spring::display::backend() { return active_backend; }

std::span<const std::uint8_t> spring::display::frame_bytes() {
  return {frame, kFrameBytes};
}

std::uint32_t spring::display::frame_checksum() {
  std::uint32_t hash{2166136261U};
  for (const auto byte : frame) { hash ^= byte; hash *= 16777619U; }
  return hash;
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

void spring::display::force_full_refresh() { baseline_valid = false; dirty = {0, 0, 400, 300}; }

void spring::display::present(const spring::ui::Frame& next) {
  const auto& bytes = next.bytes();
  std::copy(bytes.begin(), bytes.end(), frame);
  if (!baseline_valid) { dirty = {0, 0, 400, 300}; committed_frame = next; baseline_valid = true; return; }
  const auto changed = next.difference(committed_frame);
  if (changed.width != 0) dirty = {changed.x, changed.y, changed.width, changed.height};
  committed_frame = next;
}

spring::display::Rect spring::display::pending_area() { return dirty; }

spring::display::Rect spring::display::take_pending_area() {
  const auto area = dirty;
  dirty = {0, 0, 0, 0};
  return area;
}

bool spring::display::write_pixel(std::uint16_t x, std::uint16_t y, bool black) {
  if (x >= 400 || y >= 300) return false;
  const auto index = static_cast<std::size_t>(y) * 50U + x / 8U;
  const auto mask = static_cast<std::uint8_t>(0x80U >> (x % 8U));
  if (black) frame[index] |= mask;
  else frame[index] &= static_cast<std::uint8_t>(~mask);
  invalidate({x, y, 1, 1});
  return true;
}
