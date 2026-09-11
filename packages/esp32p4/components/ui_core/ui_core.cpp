#include "ui_core.hpp"
#include <array>

namespace spring::ui {
void Frame::clear() { data_.fill(0); }
bool Frame::is_black(std::uint16_t x, std::uint16_t y) const { if (x >= kWidth || y >= kHeight) return false; return (data_[y * 50 + x / 8] & static_cast<std::uint8_t>(0x80 >> (x % 8))) != 0; }
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
  for (std::size_t i = 0; value[i] != '\0'; ++i) {
    const auto ox = static_cast<std::uint16_t>(x + i * 6);
    if (value[i] >= '0' && value[i] <= '9') for (auto dy = 0; dy < 7; ++dy) for (auto dx = 0; dx < 5; ++dx) if (digits[value[i] - '0'][dy][dx] == '1') pixel(ox + dx, y + dy);
    else if (value[i] != ' ') for (auto dy = 0; dy < 7; ++dy) for (auto dx = 0; dx < 5; ++dx) if (dx == 0 || dx == 4 || dy == 0 || dy == 6) pixel(ox + dx, y + dy);
  }
}
bool Router::dispatch(Event e) { if (e == Event::home) { route_ = Route::clock; return true; } if (e == Event::sleep) { route_ = route_ == Route::sleep ? Route::clock : Route::sleep; return true; } if (e == Event::cancel) { route_ = Route::clock; return true; } if (e == Event::right) { route_ = static_cast<Route>((static_cast<int>(route_) + 1) % 5); return true; } if (e == Event::left) { route_ = static_cast<Route>((static_cast<int>(route_) + 4) % 5); return true; } return false; }
const char* Router::title(Route r) { constexpr const char* titles[] = {"CLOCK", "NETWORK", "LOCATION", "CALL", "SETTINGS", "SLEEP"}; return titles[static_cast<int>(r)]; }
void Router::render(Frame& f, const Snapshot& s) const { f.clear(); f.box({0, 0, kWidth, kHeight}); f.text(20, 20, title(route_)); if (route_ == Route::clock) { char time[6] = {static_cast<char>('0' + s.hour / 10), static_cast<char>('0' + s.hour % 10), ':', static_cast<char>('0' + s.minute / 10), static_cast<char>('0' + s.minute % 10), '\0'}; f.text(120, 125, time); f.text(120, 170, s.registered ? "ONLINE" : "OFFLINE"); } else if (route_ == Route::network) f.text(100, 130, s.registered ? "REGISTERED" : "SEARCHING"); else if (route_ == Route::location) f.text(100, 130, s.locating ? "LOCATING" : "READY"); else if (route_ == Route::call) f.text(130, 130, s.in_call ? "IN CALL" : "IDLE"); else if (route_ == Route::sleep) f.text(150, 145, "SLEEP"); }
}
