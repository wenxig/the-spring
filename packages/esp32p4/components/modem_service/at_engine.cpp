#include "at_engine.hpp"
#include "driver/uart.h"
#include "esp_log.h"

namespace {
constexpr char kTag[] = "modem";
constexpr uart_port_t kPort = UART_NUM_1;
spring::modem::Snapshot state{};
}

void spring::modem::start() {
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
  (void)timeout_ms;
  if (command.empty()) return Result::rejected;
  ++state.revision;
  return Result::ok;
}

spring::modem::Snapshot spring::modem::snapshot() { return state; }
