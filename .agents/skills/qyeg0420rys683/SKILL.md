---
name: qyeg0420rys683
description: 齐运 (Qiyun) / 佳显 4.2 英寸四色 (黑/白/黄/红) 电子墨水屏 (QYEG0420RYS683F38) 全栈开发与硬件集成指南。涵盖 400x300 分辨率、2-bit/pixel 显存映射 (30KB Framebuffer)、24-Pin FPC 排线定义与 DESPI-C02 转接板 8-Pin 对接、SPI 通信协议与寄存器时序 (PSR/BTST/PON/DRF/POF/DSLP)、全屏刷新与休眠防烧屏时序、ESP32-P4 (ESP-IDF) 与 STM32/树莓派驱动实现以及取模与抖动算法。
---

# QYEG0420RYS683 4.2寸四色水墨屏开发指南

> **模组型号**: `QYEG0420RYS683F38`  
> **规格尺寸**: 4.2 英寸，400 × 300 分辨率 (119 DPI)，四色 (黑/白/黄/红)  
> **显存容量**: 30,000 字节 (2-bit per pixel，每个字节容纳 4 个像素)  
> **通信接口**: 4-Wire SPI (Mode 0: CPOL=0, CPHA=0, 典型频率 4~10 MHz)  
> **核心特性**: 双稳态断电保持、超低功耗 (深度休眠 $\le 5\ \mu\text{A}$)、全刷耗时约 15~20 秒  

---

## 核心规格与色彩定义速查

### 1. 2-Bit 像素与颜色编码映射

| 颜色名称 | 2-bit 色码 | 16进制 | 纯色全屏填充字节 (4像素打包) | 视觉效果与 RGB |
| :---: | :---: | :---: | :---: | :--- |
| **黑色 (Black)** | `0b00` | `0x0` | **`0x00`** (`0b00000000`) | 经典墨黑 (`#000000`) |
| **白色 (White)** | `0b01` | `0x1` | **`0x55`** (`0b01010101`) | 底色纯白 (`#FFFFFF`)，**清屏使用** |
| **黄色 (Yellow)**| `0b10` | `0x2` | **`0xAA`** (`0b10101010`) | 亮黄/金黄 (`#FFD700`) |
| **红色 (Red)**   | `0b11` | `0x3` | **`0xFF`** (`0b11111111`) | 鲜艳正红 (`#FF0000`) |

> **⚠️ 关键避坑 1：全白清屏值**  
> 传统黑白屏全白通常是 `0xFF`，**但本四色屏全白是 `0x55`！** 若误写入 `0xFF`，整屏将被刷为**鲜红色**！

> **⚠️ 关键避坑 2：BUSY 忙引脚电平极性**  
> 该芯片的 BUSY 输出为 **低电平有效 (LOW = BUSY / 刷新中)**，**高电平 (HIGH = IDLE / 空闲可通信)**。发送指令前必须等待 BUSY 为高电平。

---

## 模块化参考资料索引

| 专题指南 | 文件路径 | 核心内容与设计要点 |
| :--- | :--- | :--- |
| **硬件电气与引脚定义** | [`hardware-specs-and-pinout.md`](references/hardware-specs-and-pinout.md) | 24-Pin FPC 原始引脚定义 (GDR/RESE/VSH/VGH/VGL/VCOM)、DESPI-C02 转接板 8-Pin 对接、电气极限参数与防烧屏策略 |
| **驱动协议与寄存器时序** | [`protocol-and-registers.md`](references/protocol-and-registers.md) | 4-Wire SPI 时序、完整寄存器指令集 (PSR, PWR, PON, BTST, DTM, DRF, POF, DSLP)、硬件复位与完整初始化序列 |
| **四色显存与图形取模算法**| [`color-mapping-and-graphics.md`](references/color-mapping-and-graphics.md) | 30KB Framebuffer 排布、SetPixel 坐标公式、Image2Lcd 取模参数 (4灰度/水平扫描/MSB优先)、Python 抖动转换脚本 |
| **ESP32-P4 与单片机驱动** | [`esp-idf-and-mcu-driver.md`](references/esp-idf-and-mcu-driver.md) | 基于 ESP-IDF (ESP32-P4) 的完整 SPI 驱动源码 (C/C++)、DMA 传输分片、刷新与进入休眠完整 API |

---

## 快速上手：标准驱动调用时序

```c
#include "qyeg0420.h"

// 静态显存定义 (30 KB)
static uint8_t s_framebuffer[QYEG0420_BUFFER_SIZE];

void app_main(void) {
    // 1. 初始化 SPI 与控制 GPIO
    qyeg0420_config_t config = {
        .pin_mosi = 20,
        .pin_sclk = 21,
        .pin_cs   = 22,
        .pin_dc   = 23,
        .pin_rst  = 24,
        .pin_busy = 25,
        .spi_host = SPI2_HOST,
    };
    qyeg0420_init(&config);

    // 2. 硬件复位与下发屏幕上电寄存器初始化序列
    qyeg0420_hardware_init();

    // 3. 绘制内容 (例如底色刷白，并画红/黄两色装饰点)
    qyeg0420_buffer_clear(s_framebuffer, QYEG_COLOR_WHITE); // 填入 0x55
    qyeg0420_buffer_draw_pixel(s_framebuffer, 100, 100, QYEG_COLOR_RED);
    qyeg0420_buffer_draw_pixel(s_framebuffer, 101, 100, QYEG_COLOR_YELLOW);

    // 4. 发送图像并触发全屏刷新 (耗时约 15~20 秒)，刷新完成后自动切断高压并进入 Deep Sleep
    qyeg0420_display_and_sleep(s_framebuffer);

    // 5. 此时主控可进入低功耗休眠，水墨屏依靠双稳态特性永久保持画面显示
}
```

---

## 硬件设计与开发排错速查 (Troubleshooting)

1. **屏幕刷新无反应或全屏杂乱条纹**:
   - 检查 `BS1` 引脚：必须接地 (GND) 才能工作在 4 线 SPI 模式。
   - 检查 SPI Mode：必须配置为 Mode 0 (CPOL=0, CPHA=0)。
   - 检查分辨率寄存器 `0x61`: 是否准确写入 `0x01, 0x90, 0x01, 0x2C` (400×300)。
2. **微控制器死循环或挂起在 `wait_idle`**:
   - 检查 BUSY 检测逻辑：是否错误判断为了 `while (gpio_get_level(BUSY) == 1)`。必须是 `gpio_get_level == 0` 时等待。
   - 检查是否有接触不良导致 BUSY 引脚悬空（建议在 MCU 输入端开启弱上拉）。
3. **颜色发暗或混色严重**:
   - 四色水墨屏对工作温度极为敏感 (最佳工作温度 15°C ~ 35°C)。低温 (<10°C) 时微胶囊粘度增加，颗粒迁移速度减慢，导致黄色或红色显色偏淡。
   - 必须执行官方完整的软启动与优化序列 (`0x06`, `0xE9`, `0xEF`, `0xF6`, `0xE0`, `0xE6`)。
4. **长期上电保护 (防止物理烧屏)**:
   - 绝对禁止让驱动 IC 长期停留在高压输出状态。每次执行完 `0x12 (DRF)` 刷新完成后，**必须立即执行 `0x02 (POF)` 和 `0x07 (DSLP 0xA5)`** 关闭升压回路并进入休眠。
