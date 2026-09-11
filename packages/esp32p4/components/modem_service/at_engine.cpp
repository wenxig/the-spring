#include "at_engine.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include <string>
#include <algorithm>

namespace {
constexpr char kTag[] = "modem";
constexpr uart_port_t kUart = UART_NUM_1;
constexpr std::size_t kUartBufferSize = 4096;

spring::modem::Snapshot state{};
SemaphoreHandle_t command_lock = nullptr;
bool uart_ready = false;

bool read_byte(std::uint8_t& byte, std::uint32_t timeout_ms) {
  return uart_read_bytes(kUart, &byte, 1, pdMS_TO_TICKS(timeout_ms)) == 1;
}

spring::modem::Result transact(std::string_view command, std::string_view payload,
                               std::uint32_t timeout_ms, std::string* response) {
  if (command.empty() || !uart_ready) return spring::modem::Result::transport_error;
  if (command_lock == nullptr || xSemaphoreTake(command_lock, pdMS_TO_TICKS(timeout_ms)) != pdTRUE)
    return spring::modem::Result::timeout;
  if (response != nullptr) response->clear();
  uart_flush_input(kUart);
  std::string wire{command};
  wire.append("\r\n");
  ESP_LOGI(kTag, "TX: %.*s", static_cast<int>(command.size()), command.data());
  if (uart_write_bytes(kUart, wire.data(), wire.size()) < 0 ||
      uart_wait_tx_done(kUart, pdMS_TO_TICKS(1000)) != ESP_OK) {
    xSemaphoreGive(command_lock);
    return spring::modem::Result::transport_error;
  }
  const auto deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
  std::string line;
  std::uint8_t byte{};
  bool payload_sent = payload.empty();
  bool final_ok_seen = false;
  bool async_result_seen = false;
  const auto asynchronous = command.starts_with("AT+QHTTPGET") || command.starts_with("AT+QNTP");
  while (xTaskGetTickCount() < deadline) {
    if (!read_byte(byte, 50)) continue;
    if (byte == '\n') {
      ESP_LOGI(kTag, "RX line: %s", line.c_str());
      if (line == "CONNECT" && !payload_sent) {
        if (uart_write_bytes(kUart, payload.data(), payload.size()) < 0 ||
            uart_wait_tx_done(kUart, pdMS_TO_TICKS(1000)) != ESP_OK) {
          xSemaphoreGive(command_lock);
          return spring::modem::Result::transport_error;
        }
        payload_sent = true;
      }
      if (response != nullptr && line != "OK") {
        response->append(line);
        response->push_back('\n');
      }
      if (line.starts_with("+QHTTPGET:") || line.starts_with("+QNTP:")) async_result_seen = true;
      if (spring::modem::is_urc(line)) spring::modem::consume_urc(line);
      if (asynchronous && final_ok_seen && async_result_seen) {
        xSemaphoreGive(command_lock);
        return spring::modem::Result::ok;
      }
      if (spring::modem::is_final_ok(line)) {
        final_ok_seen = true;
        if (!asynchronous || async_result_seen) {
          xSemaphoreGive(command_lock);
          return spring::modem::Result::ok;
        }
      }
      if (spring::modem::is_final_error(line)) {
        xSemaphoreGive(command_lock);
        return spring::modem::Result::rejected;
      }
      line.clear();
    } else if (byte != '\r') {
      line.push_back(static_cast<char>(byte));
    }
  }
  xSemaphoreGive(command_lock);
  return spring::modem::Result::timeout;
}

}  // namespace

void spring::modem::start() {
  command_lock = xSemaphoreCreateMutex();
  if (command_lock == nullptr) return;
  uart_config_t config{};
  config.baud_rate = 115200;
  config.data_bits = UART_DATA_8_BITS;
  config.parity = UART_PARITY_DISABLE;
  config.stop_bits = UART_STOP_BITS_1;
  config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  config.source_clk = UART_SCLK_DEFAULT;
  if (uart_param_config(kUart, &config) != ESP_OK ||
      uart_set_pin(kUart, 0, 1, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK ||
      uart_driver_install(kUart, kUartBufferSize, 0, 0, nullptr, 0) != ESP_OK) {
    ESP_LOGE(kTag, "EC600X UART1 initialization failed");
    return;
  }
  uart_ready = true;
  ESP_LOGI(kTag, "AT engine ready (UART1 TX=GPIO0 RX=GPIO1, 115200 8N1)");
}

spring::modem::Result spring::modem::execute(std::string_view command, std::uint32_t timeout_ms) {
  return transact(command, {}, timeout_ms, nullptr);
}

spring::modem::Result spring::modem::execute_capture(std::string_view command,
                                                      std::uint32_t timeout_ms, std::string& response) {
  return transact(command, {}, timeout_ms, &response);
}

spring::modem::Result spring::modem::execute_with_payload(std::string_view command,
                                                           std::string_view payload,
                                                           std::uint32_t timeout_ms, std::string& response) {
  return transact(command, payload, timeout_ms, &response);
}

spring::modem::HttpResponse spring::modem::http_get(std::string_view url) {
  if (url.empty()) return {.status = -1};
  std::string response;
  const auto url_command = "AT+QHTTPURL=" + std::to_string(url.size()) + ",10";
  if (execute_with_payload(url_command, url, 15000, response) != Result::ok) return {.status = -1};
  response.clear();
  if (execute_capture("AT+QHTTPGET=60", 70000, response) != Result::ok) return {.status = -1};
  const auto status_start = response.find("+QHTTPGET:");
  int status{};
  if (status_start == std::string::npos ||
      std::sscanf(response.c_str() + status_start, "+QHTTPGET: %d", &status) != 1) return {.status = -1};
  if (status != 200) return {.status = status};
  response.clear();
  if (execute_capture("AT+QHTTPREAD=60", 70000, response) != Result::ok) return {.status = -1};
  const auto begin = response.find('{');
  const auto end = response.rfind('}');
  if (begin == std::string::npos || end < begin) return {.status = -1};
  return {.status = 200, .body = response.substr(begin, end - begin + 1)};
}

void spring::modem::set_location(double latitude, double longitude) {
  state.latitude = latitude;
  state.longitude = longitude;
  state.location_valid = true;
  ++state.revision;
}

spring::modem::Snapshot spring::modem::snapshot() { return state; }

void spring::modem::consume_urc(std::string_view line) {
  if (line.starts_with("+CEREG:")) {
    const auto comma = line.find(',');
    const auto status = comma == std::string_view::npos ? line.substr(7) : line.substr(comma + 1);
    state.registered = status.starts_with("1") || status.starts_with("5");
  } else if (line.starts_with("+CGATT:")) {
    state.data_attached = line.find('1') != std::string_view::npos;
  } else if (line.starts_with("VOICE CALL: BEGIN")) {
    state.call_active = true;
  } else if (line.starts_with("VOICE CALL: END")) {
    state.call_active = false;
  }
  ++state.revision;
}

bool spring::modem::is_urc(std::string_view line) {
  return line.starts_with("+CEREG:") || line.starts_with("+CSQ:") ||
         line.starts_with("+CLIP:") || line.starts_with("VOICE CALL:") ||
         line.starts_with("+CMTI:") || line.starts_with("+QHTTPGET:") ||
         line.starts_with("+QNTP:");
}

bool spring::modem::is_final_ok(std::string_view line) { return line == "OK"; }

bool spring::modem::is_final_error(std::string_view line) {
  return line == "ERROR" || line.starts_with("+CME ERROR:");
}
