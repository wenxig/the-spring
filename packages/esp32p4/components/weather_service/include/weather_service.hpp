#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace spring::weather {

constexpr std::size_t kForecastSlots{4};

struct ForecastPoint {
  std::uint8_t hour{};
  std::int16_t temperature_c{};
  std::int16_t temperature_low_c{};
  std::int16_t temperature_high_c{};
  std::array<char, 24> description{};
};

struct Snapshot {
  bool valid{};
  bool fake_data{};
  std::uint8_t count{};
  std::array<ForecastPoint, kForecastSlots> forecast{};
  std::uint64_t revision{};
};

void start();
void refresh();
[[nodiscard]] Snapshot snapshot();

} // namespace spring::weather
