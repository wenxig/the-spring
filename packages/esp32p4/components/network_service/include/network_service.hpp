#pragma once

#include <string>
#include <string_view>
#include <array>
#include <cstdint>

namespace spring::network {
constexpr std::size_t kForecastSlots = 3;
enum class Link { wifi, cellular, unavailable };
struct Response { int status{}; std::string body{}; };
struct ForecastPoint {
  std::uint8_t hour{};
  std::int16_t temperature_c{};
  std::array<char, 24> description{};
};
struct WeatherSnapshot {
  bool valid{};
  std::uint8_t count{};
  std::array<ForecastPoint, kForecastSlots> forecast{};
  std::uint64_t revision{};
};
void start();
void set_link_available(Link link, bool available);
Link active_link();
Response get(std::string_view url);
Response post(std::string_view url, std::string_view body);
WeatherSnapshot weather();
}  // namespace spring::network
