#include "ui_core.hpp"
#include <algorithm>
#include <array>

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

[[nodiscard]] Utf8Codepoint decode_utf8(const unsigned char* input) {
  if (input[0] < 0x80U) return {input[0], 1};
  if ((input[0] & 0xE0U) == 0xC0U && (input[1] & 0xC0U) == 0x80U)
    return {static_cast<std::uint32_t>(((input[0] & 0x1FU) << 6U) | (input[1] & 0x3FU)), 2};
  if ((input[0] & 0xF0U) == 0xE0U && (input[1] & 0xC0U) == 0x80U && (input[2] & 0xC0U) == 0x80U)
    return {static_cast<std::uint32_t>(((input[0] & 0x0FU) << 12U) | ((input[1] & 0x3FU) << 6U) | (input[2] & 0x3FU)), 3};
  if ((input[0] & 0xF8U) == 0xF0U && (input[1] & 0xC0U) == 0x80U && (input[2] & 0xC0U) == 0x80U && (input[3] & 0xC0U) == 0x80U)
    return {static_cast<std::uint32_t>(((input[0] & 7U) << 18U) | ((input[1] & 0x3FU) << 12U) | ((input[2] & 0x3FU) << 6U) | (input[3] & 0x3FU)), 4};
  return {0xFFFD, 1};
}
}

void Frame::text_utf8(std::uint16_t x, std::uint16_t y, const char* value) {
  auto cursor = x;
  for (auto input = reinterpret_cast<const unsigned char*>(value); *input != 0;) {
    const auto codepoint = decode_utf8(input);
    if (codepoint.value < 0x80U) {
      char ascii[] = {static_cast<char>(codepoint.value), '\0'};
      text(cursor, y, ascii);
      cursor = static_cast<std::uint16_t>(cursor + 6);
    } else {
      // 16x16 placeholder cell. The flash font table will replace this cell
      // when the generated CJK asset is added; unknown glyphs stay visible.
      box({cursor, y, 16, 16});
      cursor = static_cast<std::uint16_t>(cursor + 18);
    }
    input += codepoint.width;
  }
}
bool Router::dispatch(Event e) { if (e == Event::home) { route_ = Route::clock; return true; } if (e == Event::sleep) { route_ = route_ == Route::sleep ? Route::clock : Route::sleep; return true; } if (e == Event::cancel) { route_ = Route::clock; return true; } if (route_ == Route::sleep) return false; if (route_ == Route::settings && (e == Event::up || e == Event::down || e == Event::confirm)) { if (e == Event::up) setting_index_ = static_cast<std::uint8_t>((setting_index_ + 2) % 3); if (e == Event::down) setting_index_ = static_cast<std::uint8_t>((setting_index_ + 1) % 3); return true; } if (e == Event::right) { route_ = static_cast<Route>((static_cast<int>(route_) + 1) % 5); return true; } if (e == Event::left) { route_ = static_cast<Route>((static_cast<int>(route_) + 4) % 5); return true; } return false; }
const char* Router::title(Route r) { constexpr const char* titles[] = {"CLOCK", "NETWORK", "LOCATION", "CALL", "SETTINGS", "SLEEP"}; return titles[static_cast<int>(r)]; }
void Router::render(Frame& f, const Snapshot& s) const { f.clear(); f.box({0, 0, kWidth, kHeight}); f.text(20, 20, title(route_)); if (route_ == Route::clock) { char time[6] = {static_cast<char>('0' + s.hour / 10), static_cast<char>('0' + s.hour % 10), ':', static_cast<char>('0' + s.minute / 10), static_cast<char>('0' + s.minute % 10), '\0'}; f.text(120, 125, time); f.text(120, 170, s.registered ? "ONLINE" : "OFFLINE"); } else if (route_ == Route::network) f.text(100, 130, s.registered ? "REGISTERED" : "SEARCHING"); else if (route_ == Route::location) f.text(100, 130, s.locating ? "LOCATING" : "READY"); else if (route_ == Route::call) f.text(130, 130, s.in_call ? "IN CALL" : "IDLE"); else if (route_ == Route::settings) { f.text(100, 120, "BRIGHTNESS"); f.text(100, 155, "AUTO SLEEP"); f.text(100, 190, "VERSION"); f.box({80, static_cast<std::uint16_t>(105 + setting_index_ * 35), 160, 25}); } else if (route_ == Route::sleep) f.text(150, 145, "SLEEP"); }
}
