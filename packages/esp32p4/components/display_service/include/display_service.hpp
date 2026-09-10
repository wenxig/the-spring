#pragma once

#include <cstdint>

namespace spring::display {
struct Rect { std::uint16_t x, y, width, height; };
void start();
void invalidate(Rect area);
void force_full_refresh();
}  // namespace spring::display
