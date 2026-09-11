#include "ui_core.hpp"
#include <cstring>

namespace spring::ui {
void Frame::clear() { data_.fill(0); }
void Frame::pixel(std::uint16_t x, std::uint16_t y, bool black) { if (x < kWidth && y < kHeight) { auto& b = data_[y * 50 + x / 8]; const auto m = static_cast<std::uint8_t>(0x80 >> (x % 8)); if (black) b |= m; else b &= static_cast<std::uint8_t>(~m); } }
void Frame::box(Rect r) { for (auto y = r.y; y < r.y + r.height; ++y) for (auto x = r.x; x < r.x + r.width; ++x) if (y == r.y || x == r.x || y + 1 == r.y + r.height || x + 1 == r.x + r.width) pixel(x, y); }
void Frame::text(std::uint16_t x, std::uint16_t y, const char* value) { for (std::size_t i = 0; i < std::strlen(value); ++i) { const auto ox = static_cast<std::uint16_t>(x + i * 6); for (auto dy = 0; dy < 5; ++dy) for (auto dx = 0; dx < 4; ++dx) if ((value[i] + dx + dy) % 3 == 0) pixel(ox + dx, y + dy); } }
bool Router::dispatch(Event e) { if (e == Event::home) { route_ = Route::clock; return true; } if (e == Event::sleep) { route_ = route_ == Route::sleep ? Route::clock : Route::sleep; return true; } if (e == Event::cancel) { route_ = Route::clock; return true; } if (e == Event::right) { route_ = static_cast<Route>((static_cast<int>(route_) + 1) % 5); return true; } if (e == Event::left) { route_ = static_cast<Route>((static_cast<int>(route_) + 4) % 5); return true; } return false; }
void Router::render(Frame& f, const Snapshot& s) const { f.clear(); f.box({0, 0, 400, 300}); const char* names[] = {"CLOCK", "NETWORK", "LOCATION", "CALL", "SETTINGS", "SLEEP"}; f.text(20, 20, names[static_cast<int>(route_)]); if (route_ == Route::clock) { f.text(120, 125, "08:24"); f.text(120, 170, s.registered ? "ONLINE" : "OFFLINE"); } else if (route_ == Route::network) f.text(100, 130, s.registered ? "REGISTERED" : "SEARCHING"); else if (route_ == Route::sleep) f.text(150, 145, "SLEEP"); }
}
