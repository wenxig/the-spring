#include "ui_core.hpp"
#include "declarative_ui.hpp"
#include "reference_preview.inc"
#include <fstream>
#include <string>
int main(int argc, char** argv) {
  const std::string path = argc > 1 ? argv[1] : "clock.pbm";
  const auto fallback = argc > 2 && std::string(argv[2]) == "fallback";
  const auto reference_preview = argc > 2 && std::string(argv[2]) == "reference";
  spring::ui::Snapshot snapshot{};
  spring::ui::Router router;
  // Deterministic values from docs/example.png for pixel-level layout review.
  snapshot.hour = 12;
  snapshot.minute = 0;
  snapshot.year = 2026;
  snapshot.month = 1;
  snapshot.day = 2;
  snapshot.weekday = 1;
  snapshot.countdown_days = 123;
  snapshot.date_valid = !fallback;
  snapshot.weather_valid = !fallback;
  snapshot.forecast_count = static_cast<std::uint8_t>(fallback ? 0 : 4);
  snapshot.forecast[0] = {.hour = 0, .temperature_c = 13, .temperature_low_c = 8, .temperature_high_c = 18, .description = {"晴"}};
  snapshot.forecast[1] = {.hour = 17, .temperature_c = 13, .temperature_low_c = 6, .temperature_high_c = 16, .description = {"阴"}};
  snapshot.forecast[2] = {.hour = 21, .temperature_c = 12, .temperature_low_c = 4, .temperature_high_c = 14, .description = {"雨"}};
  snapshot.forecast[3] = {.hour = 0, .temperature_c = 12, .temperature_low_c = 6, .temperature_high_c = 17, .description = {"晴"}};
  spring::ui::Frame frame;
  if (reference_preview) {
    for (std::uint16_t y{}; y < spring::ui::kHeight; ++y)
      for (std::uint16_t x{}; x < spring::ui::kWidth; ++x)
        if ((kReferencePreview[static_cast<std::size_t>(y) * 50U + x / 8U] & (0x80U >> (x % 8U))) != 0)
          frame.pixel(x, y);
  } else {
    spring::ui::render_declarative(frame, snapshot, router);
  }
  std::ofstream out(path, std::ios::binary);
  out << "P4\n400 300\n";
  const auto& bytes = frame.bytes();
  out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  return out.good() && bytes.size() == spring::ui::kBytes ? 0 : 1;
}
