#pragma once
#include <array>
#include <cstdint>

namespace spring::ui {
constexpr std::uint16_t kWidth = 400, kHeight = 300;
constexpr std::size_t kBytes = kWidth * kHeight / 8;
constexpr std::size_t kForecastSlots = 4;
enum class Route : std::uint8_t { clock, network, location, call, settings, sleep };
enum class Event : std::uint8_t { up, down, left, right, confirm, cancel, sleep, home };
struct ForecastPoint {
  std::uint8_t hour{};
  std::int16_t temperature_c{};
  std::array<char, 24> description{};
};
struct Snapshot {
  std::uint8_t hour{8}, minute{24}, signal{4};
  bool registered{true}, locating{false}, in_call{false};
  std::uint16_t year{2026};
  std::uint8_t month{1}, day{1}, weekday{};
  bool weather_valid{};
  std::uint8_t forecast_count{};
  std::array<ForecastPoint, kForecastSlots> forecast{};
};
struct Rect { std::uint16_t x{}, y{}, width{}, height{}; };
class Frame {
 public:
  void clear(); void pixel(std::uint16_t x, std::uint16_t y, bool black = true);
  void box(Rect rect); void text(std::uint16_t x, std::uint16_t y, const char* value);
  void text_scaled(std::uint16_t x, std::uint16_t y, const char* value, std::uint8_t scale);
  void text_utf8(std::uint16_t x, std::uint16_t y, const char* value);
  void text_utf8_sized(std::uint16_t x, std::uint16_t y, const char* value, std::uint8_t size);
  [[nodiscard]] const auto& bytes() const { return data_; }
  [[nodiscard]] bool is_black(std::uint16_t x, std::uint16_t y) const;
  [[nodiscard]] Rect difference(const Frame& previous) const;
 private: std::array<std::uint8_t, kBytes> data_{};
};
[[nodiscard]] Rect align_dirty(Rect dirty);
class Router {
 public:
  [[nodiscard]] Route route() const { return route_; }
  [[nodiscard]] std::uint8_t setting_index() const { return setting_index_; }
  bool dispatch(Event event);
  void render(Frame& frame, const Snapshot& snapshot) const;
  [[nodiscard]] static const char* title(Route route);
 private:
  Route route_{Route::clock};
  std::uint8_t setting_index_{};
};
}
