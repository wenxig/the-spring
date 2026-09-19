#include "ui_core.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#if !defined(CONFIG_SPRING_LVGL_DECLARATIVE_UI) || !CONFIG_SPRING_LVGL_DECLARATIVE_UI
#include "cjk_font.inc"
#define SPRING_LEGACY_CJK 1
#else
#define SPRING_LEGACY_CJK 0
#endif
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
#if SPRING_LEGACY_CJK
      const auto* begin = std::begin(detail::cjk_codepoints);
      const auto* end = std::end(detail::cjk_codepoints);
      const auto* found = std::find(begin, end, codepoint.value);
      if (cursor <= kWidth - 16) {
        if (found == end) {
          box({cursor, y, 16, 16});
        } else {
          const auto glyph = static_cast<std::size_t>(found - begin);
          for (std::uint16_t dy{}; dy < 16; ++dy)
            for (std::uint16_t dx{}; dx < 16; ++dx) {
              const auto source_row = static_cast<std::uint16_t>(dy * detail::cjk_glyph_size / 16);
              const auto source_column = static_cast<std::uint16_t>(dx * detail::cjk_glyph_size / 16);
              if ((detail::cjk_glyphs[glyph * detail::cjk_glyph_bytes +
                                      (detail::cjk_glyph_size - 1 - source_row) * detail::cjk_glyph_row_bytes +
                                      source_column / 8] &
                   (0x80U >> (source_column % 8))) != 0)
                pixel(cursor + dx, y + dy);
            }
            }
      }
#else
      if (cursor <= kWidth - 16) box({cursor, y, 16, 16});
#endif
      cursor = static_cast<std::uint16_t>(cursor + 18);
    }
    offset += codepoint.width;
  }
}

void Frame::text_utf8_sized(std::uint16_t x, std::uint16_t y, const char* value, std::uint8_t size) {
  if (value == nullptr || size == 0 || y > kHeight - size) return;
  auto cursor = x;
  const auto* input = reinterpret_cast<const unsigned char*>(value);
  const auto length = std::strlen(value);
  for (std::size_t offset{}; offset < length;) {
    const auto codepoint = decode_utf8(input + offset, length - offset);
    if (codepoint.value < 0x80U) {
      char ascii[] = {static_cast<char>(codepoint.value), '\0'};
      if (size == 16) text(cursor, y, ascii);
      cursor = static_cast<std::uint16_t>(cursor + (size == 16 ? 6 : size));
    } else {
#if SPRING_LEGACY_CJK
      const auto* begin = std::begin(detail::cjk_codepoints);
      const auto* end = std::end(detail::cjk_codepoints);
      const auto* found = std::find(begin, end, codepoint.value);
      if (cursor <= kWidth - size) {
        const auto glyph = found == end ? 0U : static_cast<std::size_t>(found - begin);
        for (std::uint16_t row{}; row < size; ++row)
          for (std::uint16_t column{}; column < size; ++column) {
            const auto scaled_row = static_cast<std::uint16_t>(row * detail::cjk_glyph_size / size);
            const auto scaled_column = static_cast<std::uint16_t>(column * detail::cjk_glyph_size / size);
            const auto bits = found == end
                                  ? 0U
                                  : detail::cjk_glyphs[glyph * detail::cjk_glyph_bytes +
                                                       (detail::cjk_glyph_size - 1 - scaled_row) *
                                                           detail::cjk_glyph_row_bytes +
                                                       scaled_column / 8];
            if ((bits & (0x80U >> (scaled_column % 8))) != 0)
              pixel(static_cast<std::uint16_t>(cursor + column), static_cast<std::uint16_t>(y + row));
          }
      }
#else
      if (cursor <= kWidth - size) box({cursor, y, size, size});
#endif
      cursor = static_cast<std::uint16_t>(cursor + size + 2);
    }
    offset += codepoint.width;
  }
}

void Frame::text_utf8_vertical_sized(std::uint16_t x, std::uint16_t y, const char* value, std::uint8_t size,
                                     std::uint8_t gap) {
  if (value == nullptr || size == 0) return;
  auto cursor = y;
  const auto* input = reinterpret_cast<const unsigned char*>(value);
  const auto length = std::strlen(value);
  for (std::size_t offset{}; offset < length;) {
    const auto codepoint = decode_utf8(input + offset, length - offset);
    if (codepoint.value == '|') {
      const auto line_x = static_cast<std::uint16_t>(x + size / 2);
      for (std::uint16_t row{1}; row + 1 < size && cursor + row < kHeight; ++row)
        for (std::uint16_t column{}; column < 2 && line_x + column < kWidth; ++column)
          pixel(static_cast<std::uint16_t>(line_x + column), static_cast<std::uint16_t>(cursor + row));
    } else if (codepoint.value >= 0x80U) {
#if SPRING_LEGACY_CJK
      const auto* begin = std::begin(detail::cjk_codepoints);
      const auto* end = std::end(detail::cjk_codepoints);
      const auto* found = std::find(begin, end, codepoint.value);
      if (found != end && x <= kWidth - size && cursor <= kHeight - size) {
        const auto glyph = static_cast<std::size_t>(found - begin);
        for (std::uint16_t row{}; row < size; ++row)
          for (std::uint16_t column{}; column < size; ++column) {
            const auto source_row = static_cast<std::uint16_t>(row * detail::cjk_glyph_size / size);
            const auto source_column = static_cast<std::uint16_t>(column * detail::cjk_glyph_size / size);
            const auto bits = detail::cjk_glyphs[glyph * detail::cjk_glyph_bytes +
                                                 (detail::cjk_glyph_size - 1 - source_row) *
                                                     detail::cjk_glyph_row_bytes +
                                                 source_column / 8];
            if ((bits & (0x80U >> (source_column % 8))) != 0)
              pixel(static_cast<std::uint16_t>(x + column), static_cast<std::uint16_t>(cursor + row));
          }
      }
#else
      if (x <= kWidth - size && cursor <= kHeight - size) box({x, cursor, size, size});
#endif
    }
    cursor = static_cast<std::uint16_t>(cursor + size + gap);
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
    } else if (value[i] == '/') {
      for (std::uint8_t row{}; row < 7; ++row)
        for (std::uint8_t py{}; py < scale; ++py)
          for (std::uint8_t px{}; px < scale; ++px)
            pixel(cursor + static_cast<std::uint16_t>((4 - row / 2) * scale + px),
                  y + static_cast<std::uint16_t>(row * scale + py));
      cursor = static_cast<std::uint16_t>(cursor + 6 * scale);
    } else if (value[i] == '-') {
      for (std::uint8_t py{}; py < scale; ++py)
        for (std::uint8_t px{}; px < 5 * scale; ++px) pixel(cursor + px, y + 3 * scale + py);
      cursor = static_cast<std::uint16_t>(cursor + 6 * scale);
    }
  }
}

namespace {
std::uint16_t scaled_text_width(const char* value, std::uint8_t scale) {
  auto width = std::uint16_t{};
  for (std::size_t index{}; value[index] != '\0'; ++index)
    width = static_cast<std::uint16_t>(width + (value[index] == ':' ? 4 : value[index] == '/' ? 6 : 6) * scale);
  return width;
}

void centered_scaled(Frame& frame, std::uint16_t left, std::uint16_t right, std::uint16_t y, const char* value,
                     std::uint8_t scale) {
  const auto width = scaled_text_width(value, scale);
  const auto x = static_cast<std::uint16_t>(left + (right - left - width) / 2);
  frame.text_scaled(x, y, value, scale);
}

std::uint8_t weather_kind(const ForecastPoint& point) {
  if (std::strstr(point.description.data(), "雨") != nullptr) return 2;
  if (std::strstr(point.description.data(), "云") != nullptr) return 1;
  return 0;
}

void weather_icon(Frame& frame, std::uint16_t x, std::uint16_t y, std::uint8_t kind, std::uint8_t size) {
  const auto icon = static_cast<std::uint8_t>(std::min<std::uint8_t>(kind, 2));
  for (std::uint16_t row{}; row < size; ++row)
    for (std::uint16_t column{}; column < size; ++column) {
      const auto source_row = static_cast<std::uint16_t>(row * 32 / size);
      const auto source_column = static_cast<std::uint16_t>(column * 32 / size);
      if ((detail::material_weather_glyphs[icon][source_row * 4 + source_column / 8] &
           (0x80U >> (source_column % 8))) != 0)
        frame.pixel(static_cast<std::uint16_t>(x + column), static_cast<std::uint16_t>(y + row));
    }
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
    f.text_utf8_sized(28, 53, s.hour < 12 ? "上午" : "下午", 18);
    f.text(72, 58, "- 12:00");
    f.text_scaled(54, 76, time, 8);
    f.text_utf8_sized(88, 146, "距离高考还有", 14);
    f.text(176, 150, "123");
    f.text_utf8_sized(218, 146, "天", 14);
    f.box({277, 0, 1, 186});
    f.box({0, 186, 277, 1});
    f.text_utf8_sized(300, 53, "星期一", 18);
    f.box({300, 81, 17, 1});
    f.text(300, 98, "2026");
    f.text_scaled(300, 118, "1/2", 5);
    f.text_utf8_sized(303, 154, "月", 14);
    f.text_utf8_sized(352, 154, "日", 14);
    char date[20]{};
    std::snprintf(date, sizeof(date), "%u/%u", s.month, s.day);
    centered_scaled(f, 4, 96, 254, date, 3);

    const auto count = std::min<std::size_t>(s.forecast_count, s.forecast.size());
    constexpr std::array<std::uint16_t, 4> card_left{0, 69, 139, 208};
    constexpr std::array<std::uint16_t, 4> card_right{69, 139, 208, 277};
    for (std::size_t index{}; index < card_left.size(); ++index) {
      const auto left = card_left[index];
      const auto right = card_right[index];
      if (index != 0) f.box({left, 204, 1, 76});
      const auto* point = index < count ? &s.forecast[index] : nullptr;
      const auto kind = point == nullptr ? static_cast<std::uint8_t>(index % 3) : weather_kind(*point);
      weather_icon(f, static_cast<std::uint16_t>(left + 13), 209, kind, 30);
      f.box({static_cast<std::uint16_t>(right - 18), 212, 1, 35});
      char temperature[12]{};
      char hour[12]{};
      if (point == nullptr) {
        std::strcpy(temperature, "--");
        std::strcpy(hour, "--:--");
      } else {
        std::snprintf(temperature, sizeof(temperature), "%d", point->temperature_c);
        std::snprintf(hour, sizeof(hour), "%02u:00", point->hour);
      }
      centered_scaled(f, static_cast<std::uint16_t>(left + 42), right, 211, temperature, 2);
      centered_scaled(f, left, right, 262, hour, 1);
      if (index == 0) f.text_utf8_sized(static_cast<std::uint16_t>(left + 20), 281, "现在", 12);
      if (index == 3) f.text_utf8_sized(static_cast<std::uint16_t>(left + 20), 281, "明天", 12);
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
