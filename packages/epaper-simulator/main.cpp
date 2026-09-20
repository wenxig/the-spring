#include "calendar.hpp"
#include "declarative_ui.hpp"
#include "ui_core.hpp"

#include <fstream>
#include <string>
#include <string_view>

int main(int argc, char** argv) {
  const auto path = std::string{argc > 1 ? argv[1] : "clock.pbm"};
  const auto mode = std::string_view{argc > 2 ? argv[2] : "sample"};
  const auto fallback = mode == "fallback";
  auto snapshot = spring::ui::Snapshot{};
  const auto router = spring::ui::Router{};
  // Reference fixture for layout comparison; firmware binds its service snapshot.
  snapshot.hour = 12;
  snapshot.minute = 0;
  snapshot.year = 2026;
  snapshot.month = 1;
  snapshot.day = 2;
  snapshot.weekday = 5;
  snapshot.location = fallback ? "" : "河北 · 秦皇岛";
  snapshot.date_valid = !fallback;
  snapshot.weather_valid = !fallback;
  snapshot.forecast_count = static_cast<std::uint8_t>(fallback ? 0 : 4);
  snapshot.forecast[0] = {
      .hour = 0, .temperature_low_c = 8, .temperature_high_c = 18, .description = {"晴"}};
  snapshot.forecast[1] = {
      .hour = 17, .temperature_low_c = 6, .temperature_high_c = 16, .description = {"阴"}};
  snapshot.forecast[2] = {
      .hour = 21, .temperature_low_c = 4, .temperature_high_c = 14, .description = {"雨"}};
  snapshot.forecast[3] = {
      .hour = 0, .temperature_low_c = 6, .temperature_high_c = 17, .description = {"晴"}};
  if (mode == "stress") {
    snapshot.hour = 23;
    snapshot.minute = 59;
    snapshot.month = 12;
    snapshot.day = 31;
    snapshot.weekday = 4;
    snapshot.location = "内蒙古 · 呼和浩特";
    snapshot.forecast[0] = {.hour = 0,
                            .temperature_low_c = -28,
                            .temperature_high_c = -16,
                            .description = {"小到中雪"}};
    snapshot.forecast[1] = {
        .hour = 3, .temperature_low_c = -23, .temperature_high_c = -11, .description = {"多云"}};
    snapshot.forecast[2] = {
        .hour = 6, .temperature_low_c = -25, .temperature_high_c = -13, .description = {"雷阵雨"}};
    snapshot.forecast[3] = {
        .hour = 12, .temperature_low_c = -20, .temperature_high_c = -10, .description = {"阴转晴"}};
  }
  if (mode == "exam") {
    snapshot.month = 6;
    snapshot.day = 8;
    snapshot.weekday = 1;
  }
  const auto exam = spring::ui::exam_countdown(snapshot.year, snapshot.month, snapshot.day);
  snapshot.countdown_days = exam.days;
  snapshot.exam_active = exam.active;
  auto frame = spring::ui::Frame{};
  spring::ui::render_declarative(frame, snapshot, router);
  auto out = std::ofstream{path, std::ios::binary};
  out << "P4\n400 300\n";
  const auto& bytes = frame.bytes();
  out.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
  return out.good() && bytes.size() == spring::ui::kBytes ? 0 : 1;
}
