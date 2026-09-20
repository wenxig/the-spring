#include "at_engine.hpp"

#include "cxx_include/esp_modem_api.hpp"
#include "cxx_include/esp_modem_dte.hpp"
#include "cxx_include/esp_modem_usb_api.hpp"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_modem_config.h"
#include "esp_modem_usb_config.h"
#include "esp_netif.h"
#include "esp_netif_ppp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "http_transport.hpp"
#include "sdkconfig.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <memory>
#include <mutex>

namespace {
using namespace spring::modem;
constexpr char kTag[] = "modem";
Snapshot state{};
std::mutex state_mutex;
// Owns DCE lifetime, AT transactions, mode transitions and cellular HTTP operations.
std::timed_mutex transaction_mutex;
std::atomic_bool started{};
std::atomic_bool online{};
std::atomic_bool at_ready{};
std::atomic_bool disconnected{};
std::atomic<esp_netif_t*> ppp_netif{};
std::unique_ptr<esp_modem::DCE> dce;

void link_state(bool connected, int error = 0) {
  online.store(connected);
  std::lock_guard lock(state_mutex);
  state.ppp_has_ip = connected;
  state.ppp_error = error;
  ++state.revision;
}
void on_event(void*, esp_event_base_t base, int32_t id, void* data) {
  if (base == IP_EVENT && id == IP_EVENT_PPP_GOT_IP) {
    const auto* event = static_cast<ip_event_got_ip_t*>(data);
    if (event && event->esp_netif == ppp_netif.load()) {
      link_state(true);
      ESP_LOGI(kTag, "PPP IPv4=" IPSTR, IP2STR(&event->ip_info.ip));
    }
  } else if (base == IP_EVENT && id == IP_EVENT_PPP_LOST_IP) {
    link_state(false);
  } else if (base == NETIF_PPP_STATUS && id > NETIF_PPP_ERRORNONE && id < NETIF_PP_PHASE_OFFSET) {
    link_state(false, static_cast<int>(id));
  }
}

void process_lines(std::string_view text) {
  while (!text.empty()) {
    const auto end = text.find('\n');
    auto line = text.substr(0, end);
    while (!line.empty() && (line.front() == '\r' || line.front() == ' '))
      line.remove_prefix(1);
    while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
      line.remove_suffix(1);
    if (spring::modem::is_urc(line))
      spring::modem::consume_urc(line);
    if (end == std::string_view::npos)
      break;
    text.remove_prefix(end + 1);
  }
}

Result command_locked(std::string_view command, std::uint32_t timeout, std::string& response) {
  constexpr auto kMaxResponseBytes = std::size_t{4096};
  response.clear();
  if (!dce || disconnected.load())
    return Result::transport_error;
  const auto result = dce->command(
      // EC600M's dedicated AT bulk port follows the historical engine's
      // CRLF framing. Keep this explicit because it is not a generic CDC
      // serial console.
      std::string{command} + "\r\n",
      [&](std::uint8_t* data, std::size_t size) {
        // esp_modem supplies the complete accumulated response on each callback.
        response.assign(reinterpret_cast<const char*>(data), std::min(size, kMaxResponseBytes));
        if (size > kMaxResponseBytes || response.find('\0') != std::string::npos)
          return esp_modem::command_result::FAIL;
        if (response.find("\nOK\r") != std::string::npos || response == "OK\r\n")
          return esp_modem::command_result::OK;
        if (response.find("\nERROR\r") != std::string::npos ||
            response.find("+CME ERROR:") != std::string::npos)
          return esp_modem::command_result::FAIL;
        return esp_modem::command_result::TIMEOUT;
      },
      timeout);
  if (response.find('\0') == std::string::npos)
    process_lines(response);
  ESP_LOGI(kTag, "AT command '%.*s' result=%d response_bytes=%u", static_cast<int>(command.size()),
           command.data(), static_cast<int>(result), static_cast<unsigned>(response.size()));

  if (result == esp_modem::command_result::OK)
    return Result::ok;
  if (result == esp_modem::command_result::TIMEOUT)
    return Result::timeout;
  return Result::rejected;
}

std::shared_ptr<esp_modem::DTE> create_dte() {
#if CONFIG_SPRING_MODEM_USE_UART
  esp_modem_dte_config_t config = ESP_MODEM_DTE_DEFAULT_CONFIG();
  config.uart_config.tx_io_num = 0;
  config.uart_config.rx_io_num = 1;
  config.uart_config.rts_io_num = UART_PIN_NO_CHANGE;
  config.uart_config.cts_io_num = UART_PIN_NO_CHANGE;
  return esp_modem::create_uart_dte(&config);
#else
  esp_modem_usb_term_config usb{};
  usb.vid = CONFIG_SPRING_MODEM_USB_VID;
  usb.pid = CONFIG_SPRING_MODEM_USB_PID;
  usb.interface_idx = CONFIG_SPRING_MODEM_USB_AT_INTERFACE;
  usb.secondary_interface_idx = CONFIG_SPRING_MODEM_USB_SECONDARY_INTERFACE;
  usb.timeout_ms = 5000;
  usb.install_usb_host = true;
  const esp_modem_dte_config_t config = ESP_MODEM_DTE_DEFAULT_USB_CONFIG(usb);
  return esp_modem::create_usb_dte(&config);
#endif
}

void modem_task(void*) {
  ESP_LOGI(kTag,
           "modem task started; EC600M dedicated AT bulk port VID=0x%04x PID=0x%04x interface=%d secondary=%d",
           CONFIG_SPRING_MODEM_USB_VID, CONFIG_SPRING_MODEM_USB_PID,
           CONFIG_SPRING_MODEM_USB_AT_INTERFACE, CONFIG_SPRING_MODEM_USB_SECONDARY_INTERFACE);
  const auto netif_result = esp_netif_init();
  const auto event_result = esp_event_loop_create_default();
  if (netif_result != ESP_OK || (event_result != ESP_OK && event_result != ESP_ERR_INVALID_STATE)) {
    ESP_LOGE(kTag, "network runtime initialization failed");
    started.store(false);
    vTaskDelete(nullptr);
    return;
  }
  esp_netif_config_t config = ESP_NETIF_DEFAULT_PPP();
  auto* netif = esp_netif_new(&config);
  ppp_netif.store(netif);
  if (!netif ||
      esp_event_handler_register(IP_EVENT, IP_EVENT_PPP_GOT_IP, on_event, nullptr) != ESP_OK ||
      esp_event_handler_register(IP_EVENT, IP_EVENT_PPP_LOST_IP, on_event, nullptr) != ESP_OK ||
      esp_event_handler_register(NETIF_PPP_STATUS, ESP_EVENT_ANY_ID, on_event, nullptr) != ESP_OK) {
    ESP_LOGE(kTag, "PPP initialization failed");
    vTaskDelete(nullptr);
    return;
  }
  while (true) {
    {
      std::lock_guard lock(transaction_mutex);
      if (disconnected.exchange(false)) {
        link_state(false);
        at_ready.store(false);
        dce.reset();
      }
      if (!dce) {
        ESP_LOGI(kTag, "waiting for EC600M dedicated AT port");
        auto dte = create_dte();
        if (dte) {
          ESP_LOGI(kTag, "EC600M dedicated AT port opened");
          dte->set_error_cb([](esp_modem::terminal_error error) {
            at_ready.store(false);
            link_state(false, static_cast<int>(error) + 100);
            disconnected.store(true);
          });
          const esp_modem_dce_config_t dce_config =
              ESP_MODEM_DCE_DEFAULT_CONFIG(CONFIG_SPRING_MODEM_APN);
          dce = esp_modem::create_generic_dce(&dce_config, std::move(dte), netif);
          if (dce) {
            dce->set_urc([](std::uint8_t* data, std::size_t size) {
              process_lines({reinterpret_cast<const char*>(data), size});
              return esp_modem::command_result::TIMEOUT;
            });
          } else {
            ESP_LOGE(kTag, "unable to create DCE on EC600M AT port");
          }
        }
      }
      if (dce && !online.load()) {
        if (dce->get_mode() == esp_modem::modem_mode::DATA_MODE)
          (void)dce->set_mode(esp_modem::modem_mode::COMMAND_MODE);
        std::string response;
        at_ready.store(command_locked("AT", 1500, response) == Result::ok);
        if (at_ready.load()) {
          if (command_locked("ATI", 1500, response) == Result::ok)
            ESP_LOGI(kTag, "modem identity: %s", response.c_str());
          (void)command_locked("ATE0", 1500, response);
          (void)command_locked("AT+CEREG=2", 1500, response);
          (void)command_locked("AT+CEREG?", 1500, response);
          (void)command_locked("AT+CGATT?", 1500, response);
          if (spring::modem::snapshot().registered &&
              !dce->set_mode(esp_modem::modem_mode::DATA_MODE))
            link_state(false, -1);
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(30'000));
  }
}

spring::network::TransportResult transport_error(spring::network::TransportError error) {
  spring::network::TransportResult result{};
  result.response.error = error;
  return result;
}

class CellularTransport final : public spring::network::HttpTransport {
public:
  bool available() const noexcept override { return online.load() && !disconnected.load(); }
  spring::network::TransportResult
  perform(const spring::network::Request& request,
          spring::network::CancellationToken cancellation) override {
    const auto began = std::chrono::steady_clock::now();
    std::unique_lock lock(transaction_mutex, std::defer_lock);
    while (!lock.try_lock_for(std::chrono::milliseconds{20})) {
      if (cancellation.cancelled())
        return transport_error(spring::network::TransportError::cancelled);
      if (std::chrono::steady_clock::now() - began >= request.options.timeout)
        return transport_error(spring::network::TransportError::timeout);
    }
    if (!available())
      return transport_error(spring::network::TransportError::unavailable);
    auto remaining = request;
    remaining.options.timeout -= std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - began);
    return spring::http::perform(remaining, std::move(cancellation), ppp_netif.load());
  }
};
CellularTransport cellular;
} // namespace

void spring::modem::start() {
  if (started.exchange(true))
    return;
  ESP_LOGI(kTag, "starting modem service; EC600M control uses the configured AT port");
  if (xTaskCreate(modem_task, "modem", 8192, nullptr, 4, nullptr) != pdPASS) {
    started.store(false);
    ESP_LOGE(kTag, "modem task allocation failed");
  } else {
    ESP_LOGI(kTag, "modem task created");
  }
}
Result spring::modem::execute(std::string_view command, std::uint32_t timeout) {
  std::string response;
  return execute_capture(command, timeout, response);
}
Result spring::modem::execute_capture(std::string_view command, std::uint32_t timeout,
                                      std::string& response) {
  if (command.empty() || timeout == 0)
    return Result::rejected;
  std::unique_lock lock(transaction_mutex, std::defer_lock);
  if (!lock.try_lock_for(std::chrono::milliseconds{timeout}))
    return Result::timeout;
  if (!dce || disconnected.load() || !at_ready.load())
    return Result::transport_error;
#if CONFIG_SPRING_MODEM_USE_UART
  constexpr auto single_port = true;
#else
  constexpr auto single_port = CONFIG_SPRING_MODEM_USB_SECONDARY_INTERFACE < 0;
#endif
  const auto resume = single_port && dce->get_mode() == esp_modem::modem_mode::DATA_MODE;
  if (resume && dce->pause_netif(true) != esp_modem::command_result::OK) {
    disconnected.store(true);
    link_state(false, -2);
    return Result::transport_error;
  }
  const auto result = command_locked(command, timeout, response);
  if (resume && dce->pause_netif(false) != esp_modem::command_result::OK) {
    link_state(false, -3);
    disconnected.store(true);
  }
  return result;
}
void spring::modem::set_location(double latitude, double longitude) {
  std::lock_guard lock(state_mutex);
  state.latitude = latitude;
  state.longitude = longitude;
  state.location_valid = true;
  ++state.revision;
}
Snapshot spring::modem::snapshot() {
  std::lock_guard lock(state_mutex);
  return state;
}
bool spring::modem::data_link_available() { return cellular.available(); }
esp_netif_obj* spring::modem::data_netif() { return ppp_netif.load(); }
spring::network::HttpTransport& spring::modem::transport() { return cellular; }
void spring::modem::consume_urc(std::string_view line) {
  std::lock_guard lock(state_mutex);
  if (line.starts_with("+CEREG:")) {
    int first{};
    int second{};
    const auto text = std::string{line};
    const auto count = std::sscanf(text.c_str(), "+CEREG: %d,%d", &first, &second);
    if (count > 0) {
      const auto status = count == 2 ? second : first;
      state.registered = status == 1 || status == 5;
    }
  } else if (line.starts_with("+CGATT:")) {
    state.data_attached = line.find('1') != std::string_view::npos;
  } else if (line.starts_with("VOICE CALL: BEGIN"))
    state.call_active = true;
  else if (line.starts_with("VOICE CALL: END"))
    state.call_active = false;
  else if (line.starts_with("+CSQ:")) {
    const auto text = std::string{line};
    (void)std::sscanf(text.c_str(), "+CSQ: %d", &state.signal_quality);
  } else
    return;
  ++state.revision;
}
bool spring::modem::is_urc(std::string_view line) {
  return line.starts_with("+CEREG:") || line.starts_with("+CGATT:") || line.starts_with("+CSQ:") ||
         line.starts_with("+CLIP:") || line.starts_with("VOICE CALL:") ||
         line.starts_with("+CMTI:");
}
bool spring::modem::is_final_ok(std::string_view line) { return line == "OK"; }
bool spring::modem::is_final_error(std::string_view line) {
  return line == "ERROR" || line.starts_with("+CME ERROR:");
}
