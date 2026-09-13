#include "ui_components.hpp"
#include "material_weather_icons.inc"
#include <array>
#include <cstdio>
#include <cstring>

namespace spring::ui::components {
namespace {
constexpr std::array<std::uint16_t, 4> kCardLeft{0, 69, 139, 208};
constexpr std::array<const char*, 4> kCardTimes{"现在", "17:00", "21:00", "明天"};
constexpr std::array<const char*, 4> kCardConditions{"晴", "阴", "雨", "晴"};

std::uint8_t weather_kind(const ForecastPoint& point) {
  if (std::strstr(point.description.data(), "雨") != nullptr) return 2;
  if (std::strstr(point.description.data(), "云") != nullptr) return 1;
  return 0;
}

void draw_weather_icon(Frame& frame, std::uint16_t x, std::uint16_t y, std::uint8_t kind) {
  const auto icon = static_cast<std::uint8_t>(std::min<std::uint8_t>(kind, 2));
  for (std::uint16_t row{}; row < 40; ++row)
    for (std::uint16_t column{}; column < 40; ++column) {
      const auto source_row = static_cast<std::uint16_t>(row * 32 / 40);
      const auto source_column = static_cast<std::uint16_t>(column * 32 / 40);
      if ((detail::material_weather_glyphs[icon][source_row * 4 + source_column / 8] &
           (0x80U >> (source_column % 8))) != 0)
        frame.pixel(static_cast<std::uint16_t>(x + column), static_cast<std::uint16_t>(y + row));
    }
}
}

void draw_header(Frame& frame, const Snapshot& snapshot) {
  frame.text_utf8_sized(28, 53, snapshot.hour < 12 ? "上午" : "下午", 18);
  frame.text(72, 58, "- 12:00");
  char time[6] = {static_cast<char>('0' + snapshot.hour / 10), static_cast<char>('0' + snapshot.hour % 10), ':',
                  static_cast<char>('0' + snapshot.minute / 10), static_cast<char>('0' + snapshot.minute % 10), '\0'};
  frame.text_scaled(54, 76, time, 8);
}

void draw_countdown(Frame& frame) {
  frame.text_utf8_sized(88, 146, "距离高考还有", 14);
  frame.text(176, 150, "123");
  frame.text_utf8_sized(218, 146, "天", 14);
}

void draw_date_panel(Frame& frame, const Snapshot& snapshot) {
  constexpr const char* weekdays[] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};
  frame.text_utf8_sized(300, 53, weekdays[snapshot.weekday % 7], 18);
  frame.box({300, 81, 17, 1});
  char year[8]{};
  std::snprintf(year, sizeof(year), "%u", snapshot.year);
  frame.text(300, 98, year);
  char month_day[8]{};
  std::snprintf(month_day, sizeof(month_day), "%u/%u", snapshot.month, snapshot.day);
  frame.text_scaled(300, 118, month_day, 5);
  frame.text_utf8_sized(303, 154, "月", 14);
  frame.text_utf8_sized(352, 154, "日", 14);
}

void draw_forecast_card(Frame& frame, const Snapshot& snapshot, std::size_t index) {
  const auto left = kCardLeft[index];
  const auto* point = index < snapshot.forecast_count ? &snapshot.forecast[index] : nullptr;
  const auto kind = point == nullptr ? static_cast<std::uint8_t>(index % 3) : weather_kind(*point);
  draw_weather_icon(frame, static_cast<std::uint16_t>(left + 10), 202, kind);
  frame.box({static_cast<std::uint16_t>(left + 51), 212, 1, 35});
  char temperature[8]{};
  if (point == nullptr) std::strcpy(temperature, "--");
  else std::snprintf(temperature, sizeof(temperature), "%d", point->temperature_c);
  frame.text(static_cast<std::uint16_t>(left + 56), 214, temperature);
  frame.text_utf8_sized(static_cast<std::uint16_t>(left + 16), 258, kCardTimes[index], 12);
  frame.text_utf8_sized(static_cast<std::uint16_t>(left + 25), 278, kCardConditions[index], 10);
}

void draw_dividers(Frame& frame) {
  frame.box({277, 0, 1, 300});
  frame.box({0, 186, 277, 1});
  for (const auto x : std::array<std::uint16_t, 3>{69, 139, 208}) frame.box({x, 204, 1, 76});
}
}
