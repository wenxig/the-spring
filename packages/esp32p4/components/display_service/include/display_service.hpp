#pragma once

#include <cstdint>
#include <cstddef>
namespace spring::ui { class Frame; }

namespace spring::display {
enum class Backend : std::uint8_t { buffer_only, epaper };
struct Rect { std::uint16_t x, y, width, height; };
constexpr std::size_t kFrameBytes = 15000;
void start(Backend backend = Backend::buffer_only);
Backend backend();
void present(const spring::ui::Frame& frame);
void invalidate(Rect area);
void force_full_refresh();
bool write_pixel(std::uint16_t x, std::uint16_t y, bool black);
Rect pending_area();
Rect take_pending_area();
}  // namespace spring::display
