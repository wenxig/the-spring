#include "ui_core.hpp"
#include <fstream>
#include <string>
int main(int argc, char** argv) {
  const std::string path = argc > 1 ? argv[1] : "clock.pbm";
  const spring::ui::Snapshot snapshot{.hour = 21,
                                      .minute = 37,
                                      .month = 9,
                                      .day = 11,
                                      .weekday = 5,
                                      .weather_valid = true,
                                      .forecast_count = 3,
                                      .forecast = {{{.hour = 18, .temperature_c = 26, .description = "晴"},
                                                    {.hour = 21, .temperature_c = 24, .description = "多云"},
                                                    {.hour = 0, .temperature_c = 22, .description = "小雨"}}}};
  spring::ui::Router router;
  spring::ui::Frame frame;
  router.render(frame, snapshot);
  if (router.route() != spring::ui::Route::clock || !frame.is_black(0, 0) || !frame.is_black(399, 299)) return 2;
  std::ofstream out(path, std::ios::binary);
  out << "P4\n400 300\n";
  const auto& bytes = frame.bytes();
  out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  return out.good() && bytes.size() == spring::ui::kBytes ? 0 : 1;
}
