#include "ui_core.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include "cjk_font.inc"
#include "material_weather_icons.inc"

namespace spring::ui {
void Frame::clear() { data_.fill(0); }
bool Frame::is_black(std::uint16_t x, std::uint16_t y) const { if (x >= kWidth || y >= kHeight) return false; return (data_[y * 50 + x / 8] & static_cast<std::uint8_t>(0x80 >> (x % 8))) != 0; }
Rect align_dirty(Rect r) { if (r.width == 0 || r.height == 0 || r.x >= kWidth || r.y >= kHeight) return {}; const auto left = static_cast<std::uint16_t>(r.x & ~7U); const auto right = std::min<std::uint16_t>(kWidth, static_cast<std::uint16_t>((std::min<std::uint32_t>(kWidth, static_cast<std::uint32_t>(r.x) + r.width) + 7U) & ~7U)); const auto bottom = std::min<std::uint16_t>(kHeight, static_cast<std::uint16_t>(std::min<std::uint32_t>(kHeight, static_cast<std::uint32_t>(r.y) + r.height))); return {left, r.y, static_cast<std::uint16_t>(right - left), static_cast<std::uint16_t>(bottom - r.y)}; }
Rect Frame::difference(const Frame& previous) const { std::uint16_t left{kWidth}, top{kHeight}, right{}, bottom{}; for (std::uint16_t y{}; y < kHeight; ++y) for (std::uint16_t x{}; x < 50; ++x) if (data_[y * 50 + x] != previous.data_[y * 50 + x]) { left = std::min(left, static_cast<std::uint16_t>(x * 8)); right = std::max(right, static_cast<std::uint16_t>(x * 8 + 8)); top = std::min(top, y); bottom = std::max(bottom, static_cast<std::uint16_t>(y + 1)); } return left == kWidth ? Rect{} : align_dirty({left, top, static_cast<std::uint16_t>(right - left), static_cast<std::uint16_t>(bottom - top)}); }
void Frame::pixel(std::uint16_t x, std::uint16_t y, bool black) { if (x < kWidth && y < kHeight) { auto& b = data_[y * 50 + x / 8]; const auto m = static_cast<std::uint8_t>(0x80 >> (x % 8)); if (black) b |= m; else b &= static_cast<std::uint8_t>(~m); } }
void Frame::box(Rect r) { for (auto y = r.y; y < r.y + r.height; ++y) for (auto x = r.x; x < r.x + r.width; ++x) if (y == r.y || x == r.x || y + 1 == r.y + r.height || x + 1 == r.x + r.width) pixel(x, y); }
void Frame::text(std::uint16_t x, std::uint16_t y, const char* value) {
  constexpr std::array<std::array<const char*, 7>, 10> digits{{
      {{"11111", "10001", "10011", "10101", "11001", "10001", "11111"}},
      {{"00100", "01100", "00100", "00100", "00100", "00100", "01110"}},
      {{"11110", "00001", "00001", "01110", "10000", "10000", "11111"}},
      {{"11110", "00001", "00001", "01110", "00001", "00001", "11110"}},
      {{"10010", "10010", "10010", "11111", "00010", "00010", "00010"}},
      {{"11111", "10000", "10000", "11110", "00001", "00001", "11110"}},
      {{"01110", "10000", "10000", "11110", "10001", "10001", "01110"}},
      {{"11111", "00001", "00010", "00100", "01000", "01000", "01000"}},
      {{"01110", "10001", "10001", "01110", "10001", "10001", "01110"}},
      {{"01110", "10001", "10001", "01111", "00001", "00001", "01110"}},
  }};
  constexpr std::array<std::array<const char*, 7>, 26> letters{{
      {{"01110", "10001", "10001", "11111", "10001", "10001", "10001"}},
      {{"11110", "10001", "10001", "11110", "10001", "10001", "11110"}},
      {{"01111", "10000", "10000", "10000", "10000", "10000", "01111"}},
      {{"11110", "10001", "10001", "10001", "10001", "10001", "11110"}},
      {{"11111", "10000", "10000", "11110", "10000", "10000", "11111"}},
      {{"11111", "10000", "10000", "11110", "10000", "10000", "10000"}},
      {{"01111", "10000", "10000", "10111", "10001", "10001", "01111"}},
      {{"10001", "10001", "10001", "11111", "10001", "10001", "10001"}},
      {{"11111", "00100", "00100", "00100", "00100", "00100", "11111"}},
      {{"00111", "00010", "00010", "00010", "00010", "10010", "01100"}},
      {{"10001", "10010", "10100", "11000", "10100", "10010", "10001"}},
      {{"10000", "10000", "10000", "10000", "10000", "10000", "11111"}},
      {{"10001", "11011", "10101", "10101", "10001", "10001", "10001"}},
      {{"10001", "11001", "10101", "10011", "10001", "10001", "10001"}},
      {{"01110", "10001", "10001", "10001", "10001", "10001", "01110"}},
      {{"11110", "10001", "10001", "11110", "10000", "10000", "10000"}},
      {{"01110", "10001", "10001", "10001", "10101", "10010", "01101"}},
      {{"11110", "10001", "10001", "11110", "10100", "10010", "10001"}},
      {{"01111", "10000", "10000", "01110", "00001", "00001", "11110"}},
      {{"11111", "00100", "00100", "00100", "00100", "00100", "00100"}},
      {{"10001", "10001", "10001", "10001", "10001", "10001", "01110"}},
      {{"10001", "10001", "10001", "10001", "10001", "01010", "00100"}},
      {{"10001", "10001", "10001", "10101", "10101", "11011", "10001"}},
      {{"10001", "10001", "01010", "00100", "01010", "10001", "10001"}},
      {{"10001", "10001", "01010", "00100", "00100", "00100", "00100"}},
      {{"11111", "00001", "00010", "00100", "01000", "10000", "11111"}},
  }};
  for (std::size_t i = 0; value[i] != '\0'; ++i) {
    const auto ox = static_cast<std::uint16_t>(x + i * 6);
    if (value[i] >= '0' && value[i] <= '9') { for (auto dy = 0; dy < 7; ++dy) for (auto dx = 0; dx < 5; ++dx) if (digits[value[i] - '0'][dy][dx] == '1') pixel(ox + dx, y + dy); }
    else if (value[i] >= 'A' && value[i] <= 'Z') {
      const auto& glyph = letters[value[i] - 'A'];
      for (auto dy = 0; dy < 7; ++dy) for (auto dx = 0; dx < 5; ++dx) if (glyph[dy][dx] == '1') pixel(ox + dx, y + dy);
    } else if (value[i] == ':') {
      pixel(ox + 2, y + 2); pixel(ox + 2, y + 5);
    } else if (value[i] != ' ') {
      for (auto dy = 0; dy < 7; ++dy) for (auto dx = 0; dx < 5; ++dx) if (dx == 0 || dx == 4 || dy == 0 || dy == 6) pixel(ox + dx, y + dy);
    }
  }
}
namespace {
struct Utf8Codepoint { std::uint32_t value{}; std::size_t width{}; };

[[nodiscard]] bool is_continuation(unsigned char value) { return (value & 0xC0U) == 0x80U; }

[[nodiscard]] Utf8Codepoint decode_utf8(const unsigned char* input, std::size_t remaining) {
  if (input[0] < 0x80U) return {input[0], 1};
  if (remaining >= 2 && input[0] >= 0xC2U && input[0] <= 0xDFU && is_continuation(input[1]))
    return {static_cast<std::uint32_t>(((input[0] & 0x1FU) << 6U) | (input[1] & 0x3FU)), 2};
  if (remaining >= 3 && input[0] >= 0xE0U && input[0] <= 0xEFU && is_continuation(input[1]) &&
      is_continuation(input[2]) && !(input[0] == 0xE0U && input[1] < 0xA0U) &&
      !(input[0] == 0xEDU && input[1] >= 0xA0U))
    return {static_cast<std::uint32_t>(((input[0] & 0x0FU) << 12U) | ((input[1] & 0x3FU) << 6U) |
                                       (input[2] & 0x3FU)),
            3};
  if (remaining >= 4 && input[0] >= 0xF0U && input[0] <= 0xF4U && is_continuation(input[1]) &&
      is_continuation(input[2]) && is_continuation(input[3]) && !(input[0] == 0xF0U && input[1] < 0x90U) &&
      !(input[0] == 0xF4U && input[1] >= 0x90U))
    return {static_cast<std::uint32_t>(((input[0] & 7U) << 18U) | ((input[1] & 0x3FU) << 12U) |
                                       ((input[2] & 0x3FU) << 6U) | (input[3] & 0x3FU)),
            4};
  return {0xFFFD, 1};
}
}

void Frame::text_utf8(std::uint16_t x, std::uint16_t y, const char* value) {
  if (value == nullptr || y > kHeight - 16) return;
  auto cursor = x;
  const auto* input = reinterpret_cast<const unsigned char*>(value);
  const auto length = std::strlen(value);
  for (std::size_t offset{}; offset < length;) {
    const auto codepoint = decode_utf8(input + offset, length - offset);
    if (codepoint.value < 0x80U) {
      char ascii[] = {static_cast<char>(codepoint.value), '\0'};
      if (cursor < kWidth) text(cursor, y, ascii);
      cursor = static_cast<std::uint16_t>(cursor + 6);
    } else {
      const auto* begin = std::begin(detail::cjk_codepoints);
      const auto* end = std::end(detail::cjk_codepoints);
      const auto* found = std::find(begin, end, codepoint.value);
      if (cursor <= kWidth - 16) {
        if (found == end) {
          box({cursor, y, 16, 16});
        } else {
          const auto glyph = static_cast<std::size_t>(found - begin);
          for (std::uint16_t dy{}; dy < 16; ++dy)
            for (std::uint16_t dx{}; dx < 16; ++dx)
              if ((detail::cjk_glyphs[glyph * 32 + (15 - dy) * 2 + dx / 8] & (0x80U >> (dx % 8))) != 0)
                pixel(cursor + dx, y + dy);
        }
      }
      cursor = static_cast<std::uint16_t>(cursor + 18);
    }
    offset += codepoint.width;
  }
}

void Frame::text_scaled(std::uint16_t x, std::uint16_t y, const char* value, std::uint8_t scale) {
  if (scale == 0) return;
  constexpr std::array<std::array<const char*, 7>, 10> digits{{
      {{"11111", "10001", "10011", "10101", "11001", "10001", "11111"}},
      {{"00100", "01100", "00100", "00100", "00100", "00100", "01110"}},
      {{"11110", "00001", "00001", "01110", "10000", "10000", "11111"}},
      {{"11110", "00001", "00001", "01110", "00001", "00001", "11110"}},
      {{"10010", "10010", "10010", "11111", "00010", "00010", "00010"}},
      {{"11111", "10000", "10000", "11110", "00001", "00001", "11110"}},
      {{"01110", "10000", "10000", "11110", "10001", "10001", "01110"}},
      {{"11111", "00001", "00010", "00100", "01000", "01000", "01000"}},
      {{"01110", "10001", "10001", "01110", "10001", "10001", "01110"}},
      {{"01110", "10001", "10001", "01111", "00001", "00001", "01110"}},
  }};
  auto cursor = x;
  for (std::size_t i{}; value[i] != '\0'; ++i) {
    if (value[i] >= '0' && value[i] <= '9') {
      const auto& glyph = digits[value[i] - '0'];
      for (std::uint8_t dy{}; dy < 7; ++dy)
        for (std::uint8_t dx{}; dx < 5; ++dx)
          if (glyph[dy][dx] == '1')
            for (std::uint8_t py{}; py < scale; ++py)
              for (std::uint8_t px{}; px < scale; ++px) pixel(cursor + dx * scale + px, y + dy * scale + py);
      cursor = static_cast<std::uint16_t>(cursor + 6 * scale);
    } else if (value[i] == ':') {
      for (std::uint8_t py{}; py < scale; ++py)
        for (std::uint8_t px{}; px < scale; ++px) {
          pixel(cursor + 2 * scale + px, y + 2 * scale + py);
          pixel(cursor + 2 * scale + px, y + 5 * scale + py);
        }
      cursor = static_cast<std::uint16_t>(cursor + 4 * scale);
    }
  }
}

namespace {
void segment_digit(Frame& frame, std::uint16_t x, std::uint16_t y, std::uint8_t digit, std::uint8_t width,
                   std::uint8_t height, std::uint8_t thickness) {
  constexpr std::uint8_t masks[] = {0x7E, 0x30, 0x6D, 0x79, 0x33, 0x5B, 0x5F, 0x70, 0x7F, 0x7B};
  if (digit > 9) return;
  const auto mask = masks[digit];
  const auto half = static_cast<std::uint16_t>((height - thickness) / 2);
  const auto draw = [&](std::uint8_t bit, std::uint16_t px, std::uint16_t py, std::uint16_t w, std::uint16_t h) {
    if ((mask & (1U << bit)) == 0) return;
    for (auto row = std::uint16_t{}; row < h; ++row)
      for (auto column = std::uint16_t{}; column < w; ++column)
        frame.pixel(static_cast<std::uint16_t>(x + px + column), static_cast<std::uint16_t>(y + py + row));
  };
  draw(6, thickness, 0, width - 2 * thickness, thickness);
  draw(5, 0, thickness, thickness, half);
  draw(4, width - thickness, thickness, thickness, half);
  draw(3, thickness, half + thickness, width - 2 * thickness, thickness);
  draw(2, 0, half + 2 * thickness, thickness, half);
  draw(1, width - thickness, half + 2 * thickness, thickness, half);
  draw(0, thickness, height - thickness, width - 2 * thickness, thickness);
}

void centered_number(Frame& frame, std::uint16_t left, std::uint16_t top, std::uint16_t width, std::uint8_t value) {
  const auto tens = static_cast<std::uint8_t>(value / 10);
  const auto ones = static_cast<std::uint8_t>(value % 10);
  constexpr std::uint16_t digit_width = 25;
  constexpr std::uint16_t digit_height = 48;
  constexpr std::uint16_t gap = 5;
  const auto start = static_cast<std::uint16_t>(left + (width - 2 * digit_width - gap) / 2);
  segment_digit(frame, start, top, tens, digit_width, digit_height, 4);
  segment_digit(frame, static_cast<std::uint16_t>(start + digit_width + gap), top, ones, digit_width, digit_height, 4);
}

void weather_icon(Frame& frame, std::uint16_t x, std::uint16_t y, std::uint8_t kind) {
  const auto icon = static_cast<std::uint8_t>(std::min<std::uint8_t>(kind, 2));
  for (std::uint16_t row{}; row < 32; ++row)
    for (std::uint16_t column{}; column < 32; ++column)
      if ((detail::material_weather_glyphs[icon][row * 4 + column / 8] & (0x80U >> (column % 8))) != 0)
        frame.pixel(static_cast<std::uint16_t>(x + column), static_cast<std::uint16_t>(y + row));
}
}
bool Router::dispatch(Event e) { if (e == Event::home) { route_ = Route::clock; return true; } if (e == Event::sleep) { route_ = route_ == Route::sleep ? Route::clock : Route::sleep; return true; } if (e == Event::cancel) { route_ = Route::clock; return true; } if (route_ == Route::sleep) return false; if (route_ == Route::settings && (e == Event::up || e == Event::down || e == Event::confirm)) { if (e == Event::up) setting_index_ = static_cast<std::uint8_t>((setting_index_ + 2) % 3); if (e == Event::down) setting_index_ = static_cast<std::uint8_t>((setting_index_ + 1) % 3); return true; } if (e == Event::right) { route_ = static_cast<Route>((static_cast<int>(route_) + 1) % 5); return true; } if (e == Event::left) { route_ = static_cast<Route>((static_cast<int>(route_) + 4) % 5); return true; } return false; }
const char* Router::title(Route r) { constexpr const char* titles[] = {"CLOCK", "NETWORK", "LOCATION", "CALL", "SETTINGS", "SLEEP"}; return titles[static_cast<int>(r)]; }
void Router::render(Frame& f, const Snapshot& s) const {
  f.clear();
  f.box({0, 0, kWidth, kHeight});
  if (route_ == Route::clock) {
    char time[6] = {static_cast<char>('0' + s.hour / 10), static_cast<char>('0' + s.hour % 10), ':',
                    static_cast<char>('0' + s.minute / 10), static_cast<char>('0' + s.minute % 10), '\0'};
    f.text_scaled(74, 45, time, 8);
    f.box({0, 214, kWidth, 1});
    f.box({133, 214, 1, 86});
    f.box({266, 214, 1, 86});
    f.text_utf8(48, 222, "日期");
    char date[20]{};
    std::snprintf(date, sizeof(date), "%u月/%u日", s.month, s.day);
    f.text_utf8(42, 254, date);
    f.text_utf8(181, 222, "天气");
    weather_icon(f, 145, 246, 0);
    centered_number(f, 180, 238, 78, s.forecast_count > 0 ? static_cast<std::uint8_t>(s.forecast[0].temperature_c) : 0);
    f.text_utf8(314, 222, "未来");
    if (s.weather_valid && s.forecast_count > 1) {
      weather_icon(f, 282, 246, 1);
      centered_number(f, 316, 238, 78, static_cast<std::uint8_t>(s.forecast[1].temperature_c));
    } else {
      weather_icon(f, 282, 246, 2);
      centered_number(f, 316, 238, 78, 0);
    }
    return;
  }
  f.text_utf8(20, 20, title(route_));
  if (route_ == Route::network) f.text(100, 130, s.registered ? "REGISTERED" : "SEARCHING");
  else if (route_ == Route::location) f.text(100, 130, s.locating ? "LOCATING" : "READY");
  else if (route_ == Route::call) f.text(130, 130, s.in_call ? "IN CALL" : "IDLE");
  else if (route_ == Route::settings) {
    f.text(100, 120, "BRIGHTNESS"); f.text(100, 155, "AUTO SLEEP"); f.text(100, 190, "VERSION");
    f.box({80, static_cast<std::uint16_t>(105 + setting_index_ * 35), 160, 25});
  } else if (route_ == Route::sleep) f.text(150, 145, "SLEEP");
}
}
