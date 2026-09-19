#pragma once
#include <chrono>
#include <cstdint>

namespace spring::ui {
struct ExamCountdown {
  std::uint16_t days{};
  bool active{};
};
[[nodiscard]] constexpr ExamCountdown exam_countdown(int year, unsigned month, unsigned day) {
  using namespace std::chrono;
  const auto date = std::chrono::year{year} / static_cast<int>(month) / static_cast<int>(day);
  if (!date.ok()) return {};
  if (month == 6 && day >= 7 && day <= 9) return {.active = true};
  const auto target_year = year + ((month > 6 || (month == 6 && day > 9)) ? 1 : 0);
  const auto remaining = sys_days{std::chrono::year{target_year} / June / 7} - sys_days{date};
  return {.days = static_cast<std::uint16_t>(remaining.count())};
}
}
