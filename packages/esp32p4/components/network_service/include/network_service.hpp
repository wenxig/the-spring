#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace spring::network {
constexpr std::size_t kForecastSlots = 4;
enum class Link { wifi, cellular, unavailable };
struct Response {
  int status{};
  std::string body{};
};
struct ForecastPoint {
  std::uint8_t hour{};
  std::int16_t temperature_c{};
  std::int16_t temperature_low_c{};
  std::int16_t temperature_high_c{};
  std::array<char, 24> description{};
};
struct WeatherSnapshot {
  bool valid{};
  bool fake_data{};
  std::uint8_t count{};
  std::array<ForecastPoint, kForecastSlots> forecast{};
  std::uint64_t revision{};
};
void start();
void set_link_available(Link link, bool available);
void mark_wifi_attempt_complete();
[[nodiscard]] bool wifi_attempt_complete();
Link active_link();
Response get(std::string_view url);
Response post(std::string_view url, std::string_view body);
WeatherSnapshot weather();
} // namespace spring::network
