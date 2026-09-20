#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define UART_PORT_NUM      UART_NUM_1
#define UART_BAUD_RATE     115200
#define UART_BUF_SIZE      2048

static const char *TAG = "uart_test";

typedef struct {
    const char *name;
    gpio_num_t tx_pin;
    gpio_num_t rx_pin;
} uart_config_mode_t;

void test_uart_mode(const uart_config_mode_t *mode, int num_tests)
{
    ESP_LOGI(TAG, "\n╔═══════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   测试模式: %-24s ║", mode->name);
    ESP_LOGI(TAG, "╚═══════════════════════════════════════╝");
    ESP_LOGI(TAG, "TX = GPIO%d, RX = GPIO%d", mode->tx_pin, mode->rx_pin);
    ESP_LOGI(TAG, "");
    
    // 先卸载旧驱动
    uart_driver_delete(UART_PORT_NUM);
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // 配置 UART
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, 
                                  mode->tx_pin,
                                  mode->rx_pin,
                                  UART_PIN_NO_CHANGE, 
                                  UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE, UART_BUF_SIZE, 0, NULL, 0));
    
    ESP_LOGI(TAG, "🚀 开始发送 AT 命令...\n");
    
    int success_count = 0;
    
    for (int i = 1; i <= num_tests; i++) {
        ESP_LOGI(TAG, "=== 测试 %d/%d ===", i, num_tests);
        
        // 清空接收缓冲区
        uart_flush_input(UART_PORT_NUM);
        vTaskDelay(pdMS_TO_TICKS(100));
        
        // 发送 AT 命令
        const char *at_cmd = "AT\r\n";
        int sent = uart_write_bytes(UART_PORT_NUM, at_cmd, strlen(at_cmd));
        ESP_LOGI(TAG, "发送: AT (已发送 %d 字节)", sent);
        
        // 等待足够长的时间让 EC600X 响应（AT 命令通常 <100ms）
        vTaskDelay(pdMS_TO_TICKS(500));
        
        // 读取响应
        uint8_t rx_buffer[128] = {0};
        int received = uart_read_bytes(UART_PORT_NUM, rx_buffer, sizeof(rx_buffer) - 1, pdMS_TO_TICKS(500));
        
        if (received > 0) {
            rx_buffer[received] = 0;
            
            // 打印可见字符和十六进制
            ESP_LOGI(TAG, "✅ 收到 %d 字节:", received);
            ESP_LOGI(TAG, "   ASCII: [%s]", rx_buffer);
            
            // 打印十六进制
            printf("   HEX:   [");
            for (int j = 0; j < received; j++) {
                printf("%02X ", rx_buffer[j]);
            }
            printf("]\n");
            
            // 检查是否包含 "OK"
            if (strstr((char*)rx_buffer, "OK") != NULL) {
                ESP_LOGI(TAG, "🎉 收到 OK 响应！");
                success_count++;
            } else {
                ESP_LOGW(TAG, "⚠️  响应不包含 OK");
            }
        } else {
            ESP_LOGE(TAG, "❌ 未收到响应 (超时)");
        }
        
        ESP_LOGI(TAG, "");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    ESP_LOGI(TAG, "本模式测试完成: %d/%d 成功\n", success_count, num_tests);
}

void app_main(void)
{
    ESP_LOGI(TAG, "\n╔═══════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   ESP32 ↔ EC600X UART 通信测试        ║");
    ESP_LOGI(TAG, "╚═══════════════════════════════════════╝\n");
    
    ESP_LOGI(TAG, "🔌 接线确认:");
    ESP_LOGI(TAG, "   E1: ESP32 GPIO0 ↔ EC600X J5-7 (RX0)");
    ESP_LOGI(TAG, "   E2: ESP32 GPIO1 ↔ EC600X J5-6 (TX0)");
    ESP_LOGI(TAG, "   E3: ESP32 GND   ↔ EC600X J5-1 (GND)");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "⚠️  请确认 EC600X 已开机（LED 亮起）");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "等待 5 秒...\n");
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    // 定义两种配置模式
    uart_config_mode_t modes[] = {
        {
            .name = "配置1: TX=GPIO0, RX=GPIO1",
            .tx_pin = GPIO_NUM_0,
            .rx_pin = GPIO_NUM_1
        },
        {
            .name = "配置2: TX=GPIO1, RX=GPIO0",
            .tx_pin = GPIO_NUM_1,
            .rx_pin = GPIO_NUM_0
        }
    };
    
    // 依次测试两种模式，每个模式发送 5 次 AT
    for (int i = 0; i < 2; i++) {
        ESP_LOGI(TAG, "\n>>> 开始测试模式 %d/2 <<<", i + 1);
        test_uart_mode(&modes[i], 5);
        
        if (i < 1) {
            ESP_LOGI(TAG, "\n⏱️  5 秒后切换到下一个模式...\n");
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
    
    ESP_LOGI(TAG, "\n╔═══════════════════════════════════════╗");
    ESP_LOGI(TAG, "║          测试完成                     ║");
    ESP_LOGI(TAG, "╚═══════════════════════════════════════╝");
    
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
