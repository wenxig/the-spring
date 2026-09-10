#include "at_engine.hpp"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string>

namespace {
constexpr char kTag[] = "modem";
constexpr uart_port_t kPort = UART_NUM_1;
spring::modem::Snapshot state{};
SemaphoreHandle_t command_lock = nullptr;
}

void spring::modem::start() {
  command_lock = xSemaphoreCreateMutex();
  const uart_config_t config{.baud_rate = 115200,
                             .data_bits = UART_DATA_8_BITS,
                             .parity = UART_PARITY_DISABLE,
                             .stop_bits = UART_STOP_BITS_1,
                             .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                             .source_clk = UART_SCLK_DEFAULT};
  uart_param_config(kPort, &config);
  uart_set_pin(kPort, GPIO_NUM_0, GPIO_NUM_1, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  uart_driver_install(kPort, 4096, 4096, 16, nullptr, 0);
  ESP_LOGI(kTag, "AT engine ready");
}

spring::modem::Result spring::modem::execute(std::string_view command, std::uint32_t timeout_ms) {
  if (command.empty()) return Result::rejected;
  if (command_lock == nullptr || xSemaphoreTake(command_lock, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) {
    return Result::timeout;
  }
  std::string wire{command};
  wire.append("\r\n");
  if (uart_write_bytes(kPort, wire.data(), wire.size()) < 0) {
    xSemaphoreGive(command_lock);
    return Result::transport_error;
  }
  std::string line;
  const auto deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
  std::uint8_t byte = 0;
  while (xTaskGetTickCount() < deadline) {
    if (uart_read_bytes(kPort, &byte, 1, pdMS_TO_TICKS(50)) != 1) continue;
    if (byte == '\n') {
      if (is_urc(line)) consume_urc(line);
      if (is_final_ok(line)) {
        xSemaphoreGive(command_lock);
        return Result::ok;
      }
      if (is_final_error(line)) {
        xSemaphoreGive(command_lock);
        return Result::rejected;
      }
      line.clear();
    } else if (byte != '\r') {
      line.push_back(static_cast<char>(byte));
    }
  }
  xSemaphoreGive(command_lock);
  return Result::timeout;
}

spring::modem::Snapshot spring::modem::snapshot() { return state; }

void spring::modem::consume_urc(std::string_view line) {
  if (line.starts_with("+CEREG: 1") || line.starts_with("+CEREG: 5")) {
    state.registered = true;
  } else if (line.starts_with("+CEREG:")) {
    state.registered = false;
  } else if (line.starts_with("+CGATT: 1")) {
    state.data_attached = true;
  } else if (line.starts_with("+CGATT:")) {
    state.data_attached = false;
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
         line.starts_with("+CMTI:");
}

bool spring::modem::is_final_ok(std::string_view line) { return line == "OK"; }

bool spring::modem::is_final_error(std::string_view line) {
  return line == "ERROR" || line.starts_with("+CME ERROR:");
}
