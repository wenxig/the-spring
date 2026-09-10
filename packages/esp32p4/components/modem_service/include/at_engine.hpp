#pragma once

#include <cstdint>
#include <string_view>

namespace spring::modem {
enum class Result : std::uint8_t { ok, timeout, transport_error, rejected };
struct Snapshot {
  std::uint64_t revision{};
  bool registered{};
  bool data_attached{};
  bool call_active{};
  bool location_valid{};
  double latitude{};
  double longitude{};
};

void start();
Result execute(std::string_view command, std::uint32_t timeout_ms);
Snapshot snapshot();
}  // namespace spring::modem
