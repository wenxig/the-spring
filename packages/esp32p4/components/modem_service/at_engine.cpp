#include "at_engine.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "usb/usb_host.h"
#include "usb/cdc_acm_host.h"
#include <string>
#include <cstring>

namespace {
constexpr char kTag[] = "modem";
constexpr std::uint16_t kVendorId = 0x2C7C;   // Quectel
constexpr std::uint16_t kProductId = 0x6002;  // EC600X
constexpr std::uint8_t kAtInterface = 3;      // AT 命令端口是 Interface 3

spring::modem::Snapshot state{};
SemaphoreHandle_t command_lock = nullptr;
SemaphoreHandle_t rx_sem = nullptr;
cdc_acm_dev_hdl_t cdc_dev = nullptr;

// 接收缓冲区
constexpr std::size_t kRxBufferSize = 2048;
char rx_buffer[kRxBufferSize];
std::size_t rx_write_pos = 0;
std::size_t rx_read_pos = 0;

// USB Host 库事件任务
void usb_host_task(void *arg) {
  while (true) {
    std::uint32_t event_flags;
    usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
  }
}

// CDC-ACM 数据接收回调
bool cdc_rx_callback(const std::uint8_t *data, std::size_t data_len, void *user_ctx) {
  if (data_len == 0) return true;
  
  // 将数据写入环形缓冲区
  for (std::size_t i = 0; i < data_len; ++i) {
    rx_buffer[rx_write_pos] = static_cast<char>(data[i]);
    rx_write_pos = (rx_write_pos + 1) % kRxBufferSize;
    
    // 如果缓冲区满，覆盖最旧的数据
    if (rx_write_pos == rx_read_pos) {
      rx_read_pos = (rx_read_pos + 1) % kRxBufferSize;
    }
  }
  
  // 通知有数据可读
  xSemaphoreGive(rx_sem);
  return true;
}

// 从环形缓冲区读取一个字节
bool read_byte(std::uint8_t &byte, std::uint32_t timeout_ms) {
  const auto deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
  
  while (xTaskGetTickCount() < deadline) {
    if (rx_read_pos != rx_write_pos) {
      byte = static_cast<std::uint8_t>(rx_buffer[rx_read_pos]);
      rx_read_pos = (rx_read_pos + 1) % kRxBufferSize;
      return true;
    }
    
    // 等待新数据到达
    xSemaphoreTake(rx_sem, pdMS_TO_TICKS(50));
  }
  
  return false;
}

}  // namespace

void spring::modem::start() {
  command_lock = xSemaphoreCreateMutex();
  rx_sem = xSemaphoreCreateBinary();
  
  // 1. 安装 USB Host 库
  usb_host_config_t host_config{};
  host_config.skip_phy_setup = false;
  host_config.intr_flags = ESP_INTR_FLAG_LEVEL1;
  ESP_ERROR_CHECK(usb_host_install(&host_config));
  ESP_LOGI(kTag, "USB Host library installed");
  
  // 创建 USB Host 任务
  xTaskCreate(usb_host_task, "usb_host", 4096, nullptr, 10, nullptr);
  
  // 2. 安装 CDC-ACM 驱动
  cdc_acm_host_driver_config_t driver_config{};
  driver_config.driver_task_stack_size = 4096;
  driver_config.driver_task_priority = 10;
  driver_config.xCoreID = 0;
  driver_config.new_dev_cb = nullptr;
  ESP_ERROR_CHECK(cdc_acm_host_install(&driver_config));
  ESP_LOGI(kTag, "CDC-ACM driver installed");
  
  // 等待设备枚举
  vTaskDelay(pdMS_TO_TICKS(2000));
  
  // 3. 打开 EC600X AT 命令端口 (Interface 3)
  ESP_LOGI(kTag, "Opening EC600X AT port (VID=0x%04X, PID=0x%04X, Interface=%d)...",
           kVendorId, kProductId, kAtInterface);
  
  cdc_acm_host_device_config_t dev_config{};
  dev_config.connection_timeout_ms = 5000;
  dev_config.out_buffer_size = 512;
  dev_config.in_buffer_size = 2048;
  dev_config.event_cb = nullptr;
  dev_config.data_cb = cdc_rx_callback;
  dev_config.user_arg = nullptr;
  
  esp_err_t err = cdc_acm_host_open(kVendorId, kProductId, kAtInterface, 
                                     &dev_config, &cdc_dev);
  if (err != ESP_OK) {
    ESP_LOGE(kTag, "Failed to open EC600X: %s", esp_err_to_name(err));
    ESP_LOGE(kTag, "Please ensure EC600X is connected via USB and powered on");
    return;
  }
  
  ESP_LOGI(kTag, "EC600X AT port opened successfully");
  
  // 4. 配置串口参数 (115200 8N1)
  cdc_acm_line_coding_t line_coding{};
  line_coding.dwDTERate = 115200;
  line_coding.bCharFormat = 0;
  line_coding.bParityType = 0;
  line_coding.bDataBits = 8;
  cdc_acm_host_line_coding_set(cdc_dev, &line_coding);
  cdc_acm_host_set_control_line_state(cdc_dev, true, true);
  
  ESP_LOGI(kTag, "AT engine ready (USB CDC-ACM Interface 3, 115200 8N1)");
  
  // 清空可能存在的初始数据
  vTaskDelay(pdMS_TO_TICKS(100));
  rx_read_pos = rx_write_pos;
}

spring::modem::Result spring::modem::execute(std::string_view command, std::uint32_t timeout_ms) {
  if (command.empty()) return Result::rejected;
  if (cdc_dev == nullptr) return Result::transport_error;
  
  if (command_lock == nullptr || xSemaphoreTake(command_lock, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) {
    return Result::timeout;
  }
  
  // 构建完整命令（加 \r\n）
  std::string wire{command};
  wire.append("\r\n");
  
  ESP_LOGI(kTag, "TX: %.*s", static_cast<int>(command.size()), command.data());
  
  // 发送命令
  esp_err_t err = cdc_acm_host_data_tx_blocking(cdc_dev, 
                                                 reinterpret_cast<const std::uint8_t*>(wire.data()),
                                                 wire.size(), 1000);
  if (err != ESP_OK) {
    ESP_LOGE(kTag, "USB write failed: %s", esp_err_to_name(err));
    xSemaphoreGive(command_lock);
    return Result::transport_error;
  }
  
  // 读取响应
  std::string line;
  const auto deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
  std::uint8_t byte = 0;
  int bytes_received = 0;
  
  while (xTaskGetTickCount() < deadline) {
    if (!read_byte(byte, 50)) continue;
    
    bytes_received++;
    
    if (byte == '\n') {
      // 收到完整行
      ESP_LOGI(kTag, "RX line: %s", line.c_str());
      
      if (is_urc(line)) {
        consume_urc(line);
      }
      
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
  
  ESP_LOGW(kTag, "Timeout after receiving %d bytes", bytes_received);
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
