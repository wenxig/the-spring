#pragma once
#include <array>
#include <cstdint>

namespace spring::ui {
constexpr std::uint16_t kWidth = 400, kHeight = 300;
constexpr std::size_t kBytes = kWidth * kHeight / 8;
enum class Route : std::uint8_t { clock, network, location, call, settings, sleep };
enum class Event : std::uint8_t { up, down, left, right, confirm, cancel, sleep, home };
struct Snapshot { std::uint8_t hour{8}, minute{24}, signal{4}; bool registered{true}, locating{false}, in_call{false}; };
struct Rect { std::uint16_t x{}, y{}, width{}, height{}; };
class Frame {
 public:
  void clear(); void pixel(std::uint16_t x, std::uint16_t y, bool black = true);
  void box(Rect rect); void text(std::uint16_t x, std::uint16_t y, const char* value);
  [[nodiscard]] const auto& bytes() const { return data_; }
  [[nodiscard]] bool is_black(std::uint16_t x, std::uint16_t y) const;
 private: std::array<std::uint8_t, kBytes> data_{};
};
class Router {
 public:
  [[nodiscard]] Route route() const { return route_; }
  bool dispatch(Event event);
  void render(Frame& frame, const Snapshot& snapshot) const;
  [[nodiscard]] static const char* title(Route route);
 private: Route route_{Route::clock};
};
}
