#pragma once

#include <cstdint>
#include <string_view>
#include <string>

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
struct HttpResponse { int status{}; std::string body{}; };

void start();
Result execute(std::string_view command, std::uint32_t timeout_ms);
Result execute_capture(std::string_view command, std::uint32_t timeout_ms, std::string& response);
Result execute_with_payload(std::string_view command, std::string_view payload,
                            std::uint32_t timeout_ms, std::string& response);
HttpResponse http_get(std::string_view url);
void set_location(double latitude, double longitude);
Snapshot snapshot();
void consume_urc(std::string_view line);
bool is_urc(std::string_view line);
bool is_final_ok(std::string_view line);
bool is_final_error(std::string_view line);
}  // namespace spring::modem
