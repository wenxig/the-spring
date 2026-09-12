#include "ui_core.hpp"
#include <fstream>
#include <string>
int main(int argc, char** argv) {
  const std::string path = argc > 1 ? argv[1] : "clock.pbm";
  const auto fallback = argc > 2 && std::string(argv[2]) == "fallback";
  spring::ui::Snapshot snapshot{};
  snapshot.hour = 21;
  snapshot.minute = 37;
  snapshot.month = 9;
  snapshot.day = 11;
  snapshot.weekday = 5;
  snapshot.weather_valid = !fallback;
  snapshot.forecast_count = static_cast<std::uint8_t>(fallback ? 0 : 4);
  snapshot.forecast[0] = {.hour = 18, .temperature_c = 26, .description = {"晴"}};
  snapshot.forecast[1] = {.hour = 21, .temperature_c = 24, .description = {"多云"}};
  snapshot.forecast[2] = {.hour = 0, .temperature_c = 22, .description = {"小雨"}};
  snapshot.forecast[3] = {.hour = 3, .temperature_c = 21, .description = {"晴"}};
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
