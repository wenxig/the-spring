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
                                 .forecast_count = 3,
                                 .forecast = {{{.hour = 18, .temperature_c = 26, .description = "晴"},
                                               {.hour = 21, .temperature_c = 24, .description = "多云"},
                                               {.hour = 0, .temperature_c = 22, .description = "小雨"}}}};
  router.render(offline, disconnected);
  router.render(online, connected);
  assert(offline.bytes() != online.bytes());
  for (std::uint16_t y = 292; y < 299; ++y)
    for (std::uint16_t x = 1; x < 399; ++x)
      if (x != 132) assert(!online.is_black(x, y));
  assert(router.dispatch(spring::ui::Event::right));
  router.render(offline, disconnected);
  router.render(online, connected);
  assert(offline.bytes() != online.bytes());
  return 0;
}
