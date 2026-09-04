---
name: waveshare-esp32-p4-wifi6
description: >
  微雪 (Waveshare) ESP32-P4-WIFI6 开发板全栈开发与硬件集成指南。涵盖 ESP32-P4NRW32 主控 (双核 RISC-V 360MHz + 32MB PSRAM + 32MB Flash)、ESP32-C6-MINI-1 协处理器 (Wi-Fi 6 + BLE 5.3)、40-Pin 扩展引脚映射 (树莓派 Pico 兼容接口)、ES8311 音频 Codec + 扬声器/麦克风电路、MIPI-DSI/CSI 接口、SDMMC TF 卡、ESP-IDF v5.4/v5.5 编译与烧录流程。适用于基于 ESP32-P4 的多媒体、底板扩展与物联网硬件开发。
---

# Waveshare ESP32-P4-WIFI6 硬件开发与软件指南

> 基于官方文档库：`https://docs.waveshare.net/ESP32-P4-WIFI6`
> 适用硬件：微雪 ESP32-P4-WIFI6 开发板（含 ESP32-P4NRW32 + ESP32-C6-MINI-1）
> 适用框架：ESP-IDF v5.4 / v5.5+，ESP-Brookesia，LVGL v9

---

## 核心硬件概览

| 模块 / 外设 | 芯片型号 / 规格 | 关键参数与总线连接 |
| :--- | :--- | :--- |
| **主控 SoC** | ESP32-P4NRW32 | 双核 32-bit RISC-V @ 360MHz (最高 400MHz)，内置 32MB PSRAM，板载 32MB NOR Flash，LP 协处理器 @ 40MHz |
| **无线协处理器** | ESP32-C6-MINI-1 | 2.4GHz Wi-Fi 6 (802.11ax/b/g/n) + BLE 5.3，通过内部 SDIO 总线与 P4 通信 |
| **音频 Codec** | ES8311 | 低功耗 I2S 音频解码芯片，集成 ADC/DAC，支持耳机/MIC 与差分输出 |
| **板载麦克风** | 模拟 MEMS MIC | 连接至 ES8311 模拟 MIC 输入端口 |
| **音频功放** | NS4150B | Class D 单声道音频功放，驱动 MX1.25 2P 接口（8Ω 2W 扬声器） |
| **MicroSD (TF) 槽** | 标准 TF 弹片插槽 | SDMMC 4-bit 总线驱动（支持 SD 3.0 高速协议） |
| **显示接口** | 15-Pin 1.0mm FPC | MIPI-DSI 2-lane（最高 1.5 Gbps/lane），支持 1080P/720P 屏幕 |
| **摄像头接口** | 15-Pin 1.0mm FPC | MIPI-CSI 2-lane，支持硬件 ISP 与 H.264 硬件编码 |
| **USB 接口** | Type-C + 4-Pin 1.25mm | Type-C 支持供电与 USB Serial/JTAG 调试；4-Pin 接口支持 USB 2.0 High-Speed OTG (480Mbps) |
| **扩展排针** | 2x20 40-Pin Header | 兼容树莓派 Pico HAT 尺寸，引出 27 个 GPIO、ADC、SPI、I2C、UART 与电源轨 |

---

## 关键引脚与外设映射表

### 1. 板载专用外设引脚（不可任意挪作他用）

- **SDMMC TF 卡接口（4-bit）：**
  - `CLK`: **GPIO43**
  - `CMD`: **GPIO44**
  - `D0`: **GPIO39**
  - `D1`: **GPIO40**
  - `D2`: **GPIO41**
  - `D3`: **GPIO42**
- **I2C 传感器与外设控制总线（常用默认）：**
  - `SCL`: **GPIO8**
  - `SDA`: **GPIO7**
  （用于控制 ES8311 Codec、MIPI 触摸屏芯片、CSI 摄像头模组配置）
- **ES8311 音频 I2S 总线：**
  - `MCLK`: **GPIO13**
  - `BCLK`: **GPIO12**
  - `WS (LRCK)`: **GPIO10**
  - `DOUT (DAC)`: **GPIO9**
  - `DIN (ADC)`: **GPIO11**
  - `PA_EN (功放使能)`: **GPIO53**
- **USB 2.0 High-Speed OTG 接口（4-Pin 1.25mm）：**
  - `DP`: **GPIO27**
  - `DM`: **GPIO26**
  - `ID`: 浮空 / 检测
- **ESP32-C6 协处理器接口：**
  - 内部专用 SDIO 总线与握手控制引脚（GPIO14~GPIO20 区域）。

### 2. 40-Pin 扩展排针引脚布局（Pico 兼容接口）

40-Pin 排针按左右各 20 引脚对称排列：

| 引脚编号 | 左侧定义 (Pin 1~20) | 引脚编号 | 右侧定义 (Pin 21~40) |
| :--- | :--- | :--- | :--- |
| **1** | **GPIO0** (UART0 TX / PWM / ADC) | **40** | **VBUS / VSYS** (5V 输入电源) |
| **2** | **GPIO1** (UART0 RX / PWM / ADC) | **39** | **VSYS** (系统 5V 轨) |
| **3** | **GND** | **38** | **GND** |
| **4** | **GPIO2** (通用 GPIO / SPI0 SCK) | **37** | **3V3_EN** (3.3V DCDC 使能) |
| **5** | **GPIO3** (通用 GPIO / SPI0 MOSI) | **36** | **3V3_OUT** (3.3V 电源输出) |
| **6** | **GPIO4** (通用 GPIO / SPI0 MISO) | **35** | **ADC_VREF** (ADC 参考电压) |
| **7** | **GPIO5** (通用 GPIO / SPI0 CS) | **34** | **GPIO25** (ADC 通道 / GPIO) |
| **8** | **GND** | **33** | **GND** |
| **9** | **GPIO6** (通用 GPIO / I2C1 SDA) | **32** | **GPIO24** (ADC 通道 / GPIO) |
| **10** | **GPIO7** (I2C0 SDA / 通用 GPIO) | **31** | **GPIO23** (ADC 通道 / GPIO) |
| **11** | **GPIO8** (I2C0 SCL / 通用 GPIO) | **30** | **GPIO22** (ADC 通道 / GPIO) |
| **12** | **GPIO9** (I2S DOUT / 通用 GPIO) | **29** | **GPIO21** (通用 GPIO) |
| **13** | **GND** | **28** | **GND** |
| **14** | **GPIO10** (I2S WS / 通用 GPIO) | **27** | **GPIO20** (通用 GPIO) |
| **15** | **GPIO11** (I2S DIN / 通用 GPIO) | **26** | **GPIO19** (通用 GPIO) |
| **16** | **GPIO12** (I2S BCLK / 通用 GPIO) | **25** | **GPIO18** (通用 GPIO) |
| **17** | **GPIO13** (I2S MCLK / 通用 GPIO) | **24** | **GPIO17** (通用 GPIO) |
| **18** | **GND** | **23** | **GND** |
| **19** | **GPIO14** (通用 GPIO) | **22** | **GPIO16** (通用 GPIO) |
| **20** | **GPIO15** (通用 GPIO) | **21** | **GPIO32** (通用 GPIO) |

---

## 软件开发与构建流程 (ESP-IDF)

### 1. 环境准备与分支选择
- 推荐使用 **ESP-IDF v5.4** 或 **v5.5+**（ESP32-P4 正式支持版本）。
- 官方支持仓库：`https://github.com/waveshareteam/ESP32-P4-Platform`

### 2. 构建与目标设定
```bash
# 1. 设置目标芯片为 ESP32-P4
idf.py set-target esp32p4

# 2. 检查芯片版本与 SDKCONFIG 适配 (ESP32-P4 rev 3.0 / 3.1+)
# 若编译报错提示芯片版本不兼容，合并对应版本配置：
cp sdkconfig.defaults.esp32p4_rev_v3_1 sdkconfig

# 3. 编译工程
idf.py build

# 4. 烧录固件与监控日志
idf.py -p /dev/ttyACM0 flash monitor
```

### 3. 进入 Bootloader 烧录模式
若 USB CDC / JTAG 无法自动复位下载：
1. 按住板载 **BOOT** 按键。
2. 短按一次 **RST (Reset)** 按键。
3. 松开 **BOOT** 按键，设备进入 ROM Download 模式。
4. 执行 `idf.py flash` 烧录。

---

## 参考文档导航

- 详细硬件电路与供电规格：`references/hardware-architecture.md`
- 40-Pin 排针与底板设计指南：`references/pinout-and-expansion.md`
- 软件配置与多媒体框架 (ESP-Brookesia/LVGL/ES8311)：`references/software-and-multimedia.md`
- 故障排查与 FAQ：`references/troubleshooting-and-faq.md`
