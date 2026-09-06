# QYEG0420RYS683 在 ESP32-P4 (ESP-IDF) 与主流 MCU 平台驱动实现

## 1. ESP32-P4 硬件连线与引脚分配推荐

在本项目中，ESP32-P4 作为主控，具有大量高速 GPIO 与专用 SPI 控制器（如 SPI2 / SPI3）。

针对 8-Pin 转接板（如 DESPI-C02）连接到 ESP32-P4 的推荐引脚分配：

| 信号名称 | 转接板排针 | ESP32-P4 推荐引脚 | 功能类型 | 说明 |
| :---: | :---: | :---: | :---: | :--- |
| **VCC** | 3.3V | 3.3V Power Pin | 供电轨 | 需稳定提供 50mA 动态供电 |
| **GND** | GND | GND | 地 | 参考地 |
| **DIN** | MOSI | GPIO 20 | SPI MOSI | 硬件 SPI 数据线 |
| **CLK** | SCK | GPIO 21 | SPI SCLK | 硬件 SPI 时钟线 (4~10 MHz) |
| **CS** | CS# | GPIO 22 | GPIO Output | SPI 硬件/软件片选 |
| **DC** | D/C# | GPIO 23 | GPIO Output | 数据/命令模式切换 |
| **RST** | RES# | GPIO 24 | GPIO Output | 硬件复位输出 |
| **BUSY**| BUSY | GPIO 25 | GPIO Input | 忙检测输入 (**注意: LOW=Busy, HIGH=Ready**) |

---

## 2. ESP-IDF (C/C++) 完整驱动实现

### 2.1 头文件 `qyeg0420.h`

```c
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QYEG0420_WIDTH       400
#define QYEG0420_HEIGHT      300
#define QYEG0420_BUFFER_SIZE (QYEG0420_WIDTH * QYEG0420_HEIGHT / 4) // 30,000 bytes

typedef enum {
    QYEG_COLOR_BLACK  = 0x00, // 00
    QYEG_COLOR_WHITE  = 0x01, // 01
    QYEG_COLOR_YELLOW = 0x02, // 10
    QYEG_COLOR_RED    = 0x03  // 11
} qyeg_color_t;

typedef struct {
    int pin_mosi;
    int pin_sclk;
    int pin_cs;
    int pin_dc;
    int pin_rst;
    int pin_busy;
    int spi_host; // e.g. SPI2_HOST
} qyeg0420_config_t;

/**
 * @brief 初始化水墨屏 GPIO 与 SPI 外设
 */
esp_err_t qyeg0420_init(const qyeg0420_config_t *config);

/**
 * @brief 硬件复位并下发上电初始化配置序列
 */
void qyeg0420_hardware_init(void);

/**
 * @brief 等待水墨屏内部空闲 (BUSY 引脚拉高)
 * @param timeout_ms 超时时间 (毫秒)，全刷时建议设置为 30000ms
 */
bool qyeg0420_wait_idle(uint32_t timeout_ms);

/**
 * @brief 传输全屏数据并启动物理电泳刷新，随后自动进入 Deep Sleep
 * @param buffer 30000 字节的图像帧缓冲
 */
void qyeg0420_display_and_sleep(const uint8_t *buffer);

/**
 * @brief 在内存缓冲区中清屏或填充纯色
 */
void qyeg0420_buffer_clear(uint8_t *buffer, qyeg_color_t color);

/**
 * @brief 在内存缓冲区中绘制一个点
 */
void qyeg0420_buffer_draw_pixel(uint8_t *buffer, int x, int y, qyeg_color_t color);

#ifdef __cplusplus
}
#endif
```

### 2.2 实现文件 `qyeg0420.c`

```c
#include "qyeg0420.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"

static const char *TAG = "QYEG0420";

static spi_device_handle_t s_spi_dev = NULL;
static qyeg0420_config_t s_config;

static void epd_send_cmd(uint8_t cmd) {
    gpio_set_level(s_config.pin_dc, 0); // Command Mode
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    spi_device_polling_transmit(s_spi_dev, &t);
}

static void epd_send_data(uint8_t data) {
    gpio_set_level(s_config.pin_dc, 1); // Data Mode
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &data,
    };
    spi_device_polling_transmit(s_spi_dev, &t);
}

static void epd_send_data_bytes(const uint8_t *data, size_t len) {
    gpio_set_level(s_config.pin_dc, 1); // Data Mode
    // 分批使用 DMA 或轮询传输
    size_t chunk_size = 4096;
    for (size_t offset = 0; offset < len; offset += chunk_size) {
        size_t to_send = (len - offset < chunk_size) ? (len - offset) : chunk_size;
        spi_transaction_t t = {
            .length = to_send * 8,
            .tx_buffer = data + offset,
        };
        spi_device_polling_transmit(s_spi_dev, &t);
    }
}

bool qyeg0420_wait_idle(uint32_t timeout_ms) {
    uint32_t elapsed = 0;
    // 关键：LOW=BUSY, HIGH=IDLE
    while (gpio_get_level(s_config.pin_busy) == 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
        elapsed += 10;
        if (elapsed >= timeout_ms) {
            ESP_LOGE(TAG, "Wait busy timeout (%lu ms)!", (unsigned long)timeout_ms);
            return false;
        }
    }
    return true;
}

void qyeg0420_hardware_init(void) {
    // 硬件复位脉冲
    gpio_set_level(s_config.pin_rst, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(s_config.pin_rst, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(s_config.pin_rst, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
    qyeg0420_wait_idle(1000);

    // 软启动
    epd_send_cmd(0x06);
    epd_send_data(0x0F);
    epd_send_data(0x8B);
    epd_send_data(0x9C);
    epd_send_data(0x96);

    // 芯片内部寄存器初始化配置
    epd_send_cmd(0xE9);
    epd_send_data(0x01);

    epd_send_cmd(0xEF);
    epd_send_data(0x01);

    epd_send_cmd(0xF6);
    epd_send_data(0x15);

    epd_send_cmd(0xEF);
    epd_send_data(0x00);

    // 面板配置 (0x2F: FPC向下排线扫描方向)
    epd_send_cmd(0x00);
    epd_send_data(0x2F);
    epd_send_data(0x69);
    qyeg0420_wait_idle(1000);

    // 内部时钟 PLL
    epd_send_cmd(0x30);
    epd_send_data(0x08);

    // 分辨率: 400x300
    epd_send_cmd(0x61);
    epd_send_data(QYEG0420_WIDTH / 256);
    epd_send_data(QYEG0420_WIDTH % 256);
    epd_send_data(QYEG0420_HEIGHT / 256);
    epd_send_data(QYEG0420_HEIGHT % 256);

    epd_send_cmd(0x62);
    epd_send_data(0x64);
    epd_send_data(0x53);

    epd_send_cmd(0x65);
    epd_send_data(0x00);
    epd_send_data(0x00);
    epd_send_data(0x00);
    epd_send_data(0x00);

    // VCOM / CDI
    epd_send_cmd(0x50);
    epd_send_data(0x37);

    epd_send_cmd(0xE0);
    epd_send_data(0x02);

    epd_send_cmd(0xE6);
    epd_send_data(0x5C);

    epd_send_cmd(0xA5);
    qyeg0420_wait_idle(1000);
}

esp_err_t qyeg0420_init(const qyeg0420_config_t *config) {
    s_config = *config;

    // 配置 GPIO 输出
    gpio_config_t io_conf_out = {
        .pin_bit_mask = (1ULL << s_config.pin_dc) | (1ULL << s_config.pin_rst),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_conf_out);

    // 配置 BUSY 输入
    gpio_config_t io_conf_in = {
        .pin_bit_mask = (1ULL << s_config.pin_busy),
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE, // 弱上拉，防止空闲浮空
    };
    gpio_config(&io_conf_in);

    // 配置 SPI 总线
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = s_config.pin_mosi,
        .sclk_io_num = s_config.pin_sclk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    ESP_ERROR_CHECK(spi_bus_initialize((spi_host_device_t)s_config.spi_host, &buscfg, SPI_DMA_CH_AUTO));

    // 配置 SPI 设备 (Mode 0, 8MHz)
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 8 * 1000 * 1000,
        .mode = 0, // CPOL=0, CPHA=0
        .spics_io_num = s_config.pin_cs,
        .queue_size = 7,
    };
    return spi_bus_add_device((spi_host_device_t)s_config.spi_host, &devcfg, &s_spi_dev);
}

void qyeg0420_display_and_sleep(const uint8_t *buffer) {
    // 1. 发送显存数据
    epd_send_cmd(0x10);
    epd_send_data_bytes(buffer, QYEG0420_BUFFER_SIZE);

    // 2. 启动高压升压
    epd_send_cmd(0x04); // PON
    qyeg0420_wait_idle(1000);

    // 3. 启动全屏刷新 (耗时 15~20 秒)
    ESP_LOGI(TAG, "Display refreshing...");
    epd_send_cmd(0x12); // DRF
    epd_send_data(0x00);
    qyeg0420_wait_idle(30000); // 30s 超时等待
    ESP_LOGI(TAG, "Refresh completed.");

    // 4. 关闭高压
    epd_send_cmd(0x02); // POF
    epd_send_data(0x00);
    qyeg0420_wait_idle(1000);

    // 5. 深度休眠
    epd_send_cmd(0x07); // DSLP
    epd_send_data(0xA5);
    vTaskDelay(pdMS_TO_TICKS(200));
    ESP_LOGI(TAG, "Screen entered Deep Sleep.");
}

void qyeg0420_buffer_clear(uint8_t *buffer, qyeg_color_t color) {
    uint8_t byte_val = (color << 6) | (color << 4) | (color << 2) | color;
    memset(buffer, byte_val, QYEG0420_BUFFER_SIZE);
}

void qyeg0420_buffer_draw_pixel(uint8_t *buffer, int x, int y, qyeg_color_t color) {
    if (x < 0 || x >= QYEG0420_WIDTH || y < 0 || y >= QYEG0420_HEIGHT) return;
    uint32_t idx = y * (QYEG0420_WIDTH / 4) + (x / 4);
    uint8_t shift = (3 - (x % 4)) * 2;
    buffer[idx] &= ~(0x03 << shift);
    buffer[idx] |= ((color & 0x03) << shift);
}
```
