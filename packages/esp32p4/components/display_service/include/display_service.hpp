#pragma once

#include <cstdint>
#include <cstddef>

namespace spring::display {
struct Rect { std::uint16_t x, y, width, height; };
constexpr std::size_t kFrameBytes = 15000;
void start();
void invalidate(Rect area);
void force_full_refresh();
bool write_pixel(std::uint16_t x, std::uint16_t y, bool black);
Rect pending_area();
Rect take_pending_area();
}  // namespace spring::display
