#include "ui_core.hpp"
#include <cassert>

int main() {
  spring::ui::Router router;
  spring::ui::Frame offline;
  spring::ui::Frame online;
  spring::ui::Snapshot disconnected{.hour = 0, .minute = 0, .signal = 0, .registered = false, .locating = true, .in_call = false};
  spring::ui::Snapshot connected{.hour = 23,
                                 .minute = 59,
                                 .signal = 4,
                                 .registered = true,
                                 .locating = false,
                                 .in_call = true,
                                 .month = 9,
                                 .day = 11,
                                 .weekday = 5,
                                 .weather_valid = true,
                                 .forecast_count = 4};
  connected.forecast[0] = {.hour = 18, .temperature_c = 26, .description = {"晴"}};
  connected.forecast[1] = {.hour = 21, .temperature_c = 24, .description = {"多云"}};
  connected.forecast[2] = {.hour = 0, .temperature_c = 22, .description = {"小雨"}};
  connected.forecast[3] = {.hour = 3, .temperature_c = 21, .description = {"晴"}};
  router.render(offline, disconnected);
  router.render(online, connected);
  assert(offline.bytes() != online.bytes());
  for (std::uint16_t y = 204; y < 280; ++y) {
    assert(online.is_black(69, y));
    assert(online.is_black(139, y));
    assert(online.is_black(208, y));
  }
  for (const auto left : {std::uint16_t{0}, std::uint16_t{69}, std::uint16_t{139}, std::uint16_t{208}}) {
    auto has_status_pixels = false;
    for (std::uint16_t y = 204; y < 280; ++y)
      for (std::uint16_t x = left; x < left + 42; ++x)
        has_status_pixels = has_status_pixels || online.is_black(x, y);
    assert(has_status_pixels);
  }
  assert(router.dispatch(spring::ui::Event::right));
  router.render(offline, disconnected);
  router.render(online, connected);
  assert(offline.bytes() != online.bytes());
  return 0;
}
