#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <string>
#include <string_view>

struct esp_netif_obj;

namespace spring::network {
class HttpTransport;
}

namespace spring::modem {
enum class Result : std::uint8_t { ok, timeout, transport_error, rejected };
struct Snapshot {
  std::uint64_t revision{};
  bool registered{};
  bool data_attached{};
  bool ppp_has_ip{};
  bool ppp_has_ipv6{};
  bool ppp_ipv6_global{};
  int ppp_ipv6_index{-1};
  std::array<std::uint32_t, 4> ppp_ipv6{};
  std::uint8_t ppp_ipv6_zone{};
  int ppp_error{};
  int signal_quality{99};
  bool call_active{};
  bool location_valid{};
  double latitude{};
  double longitude{};
};
void start();
Result execute(std::string_view command, std::uint32_t timeout_ms);
Result execute_capture(std::string_view command, std::uint32_t timeout_ms, std::string& response);
void set_location(double latitude, double longitude);
Snapshot snapshot();
bool data_link_available();
esp_netif_obj* data_netif();
spring::network::HttpTransport& transport();
void consume_urc(std::string_view line);
bool is_urc(std::string_view line);
bool is_final_ok(std::string_view line);
bool is_final_error(std::string_view line);
} // namespace spring::modem
