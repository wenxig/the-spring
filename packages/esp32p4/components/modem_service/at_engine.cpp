#include "at_engine.hpp"

#include "cxx_include/esp_modem_api.hpp"
#include "cxx_include/esp_modem_dte.hpp"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_modem_config.h"
#include "esp_netif.h"
#include "esp_netif_ppp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "http_transport.hpp"
#include "sdkconfig.h"
#include "usb/cdc_acm_host.h"
#include "usb/usb_host.h"

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

TaskHandle_t usb_host_task_handle{};
std::once_flag usb_host_init_once;
esp_err_t usb_host_init_result{ESP_FAIL};

void usb_host_task(void*) {
  while (true) {
    std::uint32_t event_flags{};
    usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
    if ((event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) != 0)
      usb_host_device_free_all();
  }
}

bool install_usb_host() {
  std::call_once(usb_host_init_once, [] {
    usb_host_config_t host_config{};
    host_config.skip_phy_setup = false;
    host_config.intr_flags = ESP_INTR_FLAG_LEVEL1;
    auto err = usb_host_install(&host_config);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
      ESP_LOGE(kTag, "USB host install failed: %s", esp_err_to_name(err));
      usb_host_init_result = err;
      return;
    }
    if (xTaskCreate(usb_host_task, "usb_host", 4096, nullptr, 10, &usb_host_task_handle) !=
        pdPASS) {
      ESP_LOGE(kTag, "USB host task creation failed");
      usb_host_init_result = ESP_ERR_NO_MEM;
      return;
    }

    cdc_acm_host_driver_config_t driver_config{};
    driver_config.driver_task_stack_size = 4096;
    driver_config.driver_task_priority = 10;
    driver_config.xCoreID = 0;
    err = cdc_acm_host_install(&driver_config);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
      ESP_LOGE(kTag, "CDC-ACM host install failed: %s", esp_err_to_name(err));
      usb_host_init_result = err;
      return;
    }
    usb_host_init_result = ESP_OK;
    ESP_LOGI(kTag, "historical CDC-ACM AT engine installed");
  });
  return usb_host_init_result == ESP_OK;
}

#if !CONFIG_SPRING_MODEM_USE_UART
class HistoricalUsbTerminal final : public esp_modem::Terminal {
public:
  HistoricalUsbTerminal() {
    if (!install_usb_host())
      return;

    cdc_acm_host_device_config_t device_config{};
    device_config.connection_timeout_ms = 5000;
    device_config.out_buffer_size = 512;
    device_config.in_buffer_size = 2048;
    device_config.event_cb = &on_event;
    device_config.data_cb = &on_rx;
    device_config.user_arg = this;
    const auto err =
        cdc_acm_host_open(CONFIG_SPRING_MODEM_USB_VID, CONFIG_SPRING_MODEM_USB_PID,
                          CONFIG_SPRING_MODEM_USB_AT_INTERFACE, &device_config, &cdc_device_);
    if (err != ESP_OK) {
      ESP_LOGE(kTag, "EC600M dedicated AT port open failed: %s", esp_err_to_name(err));
      return;
    }

    cdc_acm_line_coding_t line_coding{};
    line_coding.dwDTERate = 115200;
    line_coding.bCharFormat = 0;
    line_coding.bParityType = 0;
    line_coding.bDataBits = 8;
    const auto coding_err = cdc_acm_host_line_coding_set(cdc_device_, &line_coding);
    const auto control_err = cdc_acm_host_set_control_line_state(cdc_device_, true, true);
    ESP_LOGI(kTag, "EC600M dedicated AT port opened on interface %d (coding=%s control=%s)",
             CONFIG_SPRING_MODEM_USB_AT_INTERFACE, esp_err_to_name(coding_err),
             esp_err_to_name(control_err));
  }

  ~HistoricalUsbTerminal() override {
    if (cdc_device_ != nullptr) {
      cdc_acm_host_close(cdc_device_);
      cdc_device_ = nullptr;
    }
  }

  [[nodiscard]] bool ready() const { return cdc_device_ != nullptr; }

  int write(std::uint8_t* data, std::size_t len) override {
    if (cdc_device_ == nullptr || data == nullptr || len == 0)
      return -1;
    ESP_LOG_BUFFER_HEXDUMP(kTag, data, len, ESP_LOG_DEBUG);
    const auto err = cdc_acm_host_data_tx_blocking(cdc_device_, data, len, 1000);
    if (err != ESP_OK) {
      ESP_LOGE(kTag, "AT TX failed: %s", esp_err_to_name(err));
      return -1;
    }
    return static_cast<int>(len);
  }

  int read(std::uint8_t*, std::size_t) override { return -1; }
  void start() override {}
  void stop() override {}

private:
  static bool on_rx(const std::uint8_t* data, std::size_t len, void* user_arg) {
    auto* terminal = static_cast<HistoricalUsbTerminal*>(user_arg);
    ESP_LOG_BUFFER_HEXDUMP(kTag, data, len, ESP_LOG_DEBUG);
    if (terminal == nullptr || terminal->on_read == nullptr) {
      ESP_LOGW(kTag, "AT RX dropped: DTE callback is not installed");
      return true;
    }
    return terminal->on_read(const_cast<std::uint8_t*>(data), len);
  }

  static void on_event(const cdc_acm_host_dev_event_data_t* event, void* user_arg) {
    auto* terminal = static_cast<HistoricalUsbTerminal*>(user_arg);
    if (terminal == nullptr || event == nullptr)
      return;
    if (event->type == CDC_ACM_HOST_DEVICE_DISCONNECTED) {
      terminal->cdc_device_ = nullptr;
      if (terminal->on_error != nullptr)
        terminal->on_error(esp_modem::terminal_error::DEVICE_GONE);
    } else if (event->type == CDC_ACM_HOST_ERROR && terminal->on_error != nullptr) {
      terminal->on_error(esp_modem::terminal_error::UNEXPECTED_CONTROL_FLOW);
    }
  }

  cdc_acm_dev_hdl_t cdc_device_{};
};
#endif

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
  esp_modem_dte_config_t config = ESP_MODEM_DTE_DEFAULT_CONFIG();
  auto terminal = std::make_unique<HistoricalUsbTerminal>();
  if (!terminal->ready())
    return nullptr;
  return std::make_shared<esp_modem::DTE>(&config, std::move(terminal));
#endif
}

void modem_task(void*) {
  ESP_LOGI(kTag,
           "modem task started; EC600M dedicated AT bulk port VID=0x%04x PID=0x%04x interface=%d "
           "secondary=%d",
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
