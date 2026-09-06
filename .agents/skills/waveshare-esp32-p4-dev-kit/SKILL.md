---
name: waveshare-esp32-p4-dev-kit
description: 微雪 (Waveshare) ESP32-P4-WIFI6-DEV-KIT 开发板开发与硬件集成指南。涵盖 ESP32-P4NRW32 主控 (双核 RISC-V 360MHz + 32MB PSRAM + 16MB Flash)、ESP32-C6-MINI-1 协处理器 (Wi-Fi 6 + BLE 5.3)、40-Pin 扩展引脚映射、ES8311 音频电路、百兆以太网/PoE 接口、MIPI-DSI/CSI、USB OTG、SDMMC TF 卡以及 ESP-IDF 编译与系统集成流程。
---

# 微雪 ESP32-P4-WIFI6-DEV-KIT 开发板开发指南

> 官方文档：<https://docs.waveshare.net/ESP32-P4-WIFI6-DEV-KIT>  
> 适用硬件：微雪 ESP32-P4-WIFI6-DEV-KIT（A/B/C 套餐通用）  
> 角色定位：项目中主控制板，统领传感器、移远 EC600X 通信板及 SPI 水墨屏  

---

## 硬件核心规格

| 项目 | 技术指标与参数 |
| :--- | :--- |
| **主控 SoC** | **ESP32-P4NRW32**，双核 RISC-V @ 360MHz (HP)，内置 FPU/DSP 扩展；单核 RISC-V @ 40MHz (LP) |
| **内存与存储** | 封装内集成 **32MB PSRAM**，板载 **16MB NOR Flash**，768KB HP L2MEM，32KB LP SRAM |
| **无线协处理器**| **ESP32-C6-MINI-1**，支持 2.4GHz Wi-Fi 6 (802.11ax) + BLE 5.3，通过 SDIO 总线连接 |
| **网络接口** | 百兆 RJ45 以太网口（支持外接 PoE 模块供电） |
| **音频系统** | ES8311 音频 Codec + NS4150B Class D 功放，板载贴片麦克风、3.5mm 耳机接口与 MX1.25 喇叭座 |
| **显示与相机** | 15-Pin MIPI-DSI (2-lane，最高 1080P) + 15-Pin MIPI-CSI (2-lane，支持硬件 ISP 与 H.264 编码) |
| **USB 与调试** | Type-C UART 调试烧录口 (CH343P)、Type-C USB (ESP32-P4 原生)、Type-A USB 2.0 High Speed OTG (480Mbps) |
| **扩展接口** | 2x20 40-Pin 排针，引出 28 个可编程 GPIO，兼容树莓派 Pico 扩展形式 |
| **存储扩展** | 标准 MicroSD (TF) 弹片插槽，支持 4-bit SDIO 3.0 (SDMMC) |

---

## 模块化参考资料索引

| 主题 | 文档路径 | 核心要点 |
| :--- | :--- | :--- |
| **硬件规格与引脚映射** | [`hardware-specs-and-pinout.md`](references/hardware-specs-and-pinout.md) | SoC 架构、板载专用引脚（SDIO/音频/I2C/USB）避坑、40-Pin 扩展端子完整定义 |
| **外设与扩展接口开发** | [`peripherals-and-interfaces.md`](references/peripherals-and-interfaces.md) | SPI 水墨屏驱动引脚、EC600X 串口与电源控制互联、ES8311 音频系统、MIPI-DSI/CSI |
| **ESP-IDF 固件开发指南** | [`esp-idf-development.md`](references/esp-idf-development.md) | ESP-IDF v5.4/v5.5+ 编译流程、P4 芯片目标设定、PSRAM 配置、烧录与 Bootloader 救砖 |

---

## 快速上手与工程实践

### 1. 开发环境配置
- **ESP-IDF 版本**: 推荐 `v5.4` 或 `v5.5+`
- **目标芯片设置**:
  ```bash
  idf.py set-target esp32p4
  ```
- **配置 32MB PSRAM**:
  在 `menuconfig` 中启用 `Component config -> ESP PSRAM -> Support for external, SPI-connected RAM`，确保工作模式为 Octal/Hex 模式且速度配置正确。

### 2. 烧录与串口监视
- 板载配备两个 Type-C 接口：
  - **Type-C UART**：通过 CH343P 转接，用于下载固件和查看标准日志输出。
  - **Type-C USB**：直连 P4 原生 USB，可用于 USB DFU/CDC/JTAG 调试。
- 烧录命令：
  ```bash
  idf.py -p /dev/tty.usbserial-* flash monitor
  ```
- 若无法识别下载，按住 **BOOT** 键，点按 **RST** 键，松开 **BOOT** 键即可强制进入烧录模式。

### 3. 系统集成与总线规划
- **EC600X 模组连接**：通过 GPIO0/GPIO1 (UART0) 或分配独立 UART 引脚连接 EC600X 的 Main UART，搭配 GPIO 控制 PWRKEY 和检测 RI 信号。
- **SPI 水墨屏连接**：分配标准硬件 SPI 引脚（SCLK, MOSI, CS, DC, RST, BUSY），利用 DMA 缓冲区刷新墨水屏显存。
