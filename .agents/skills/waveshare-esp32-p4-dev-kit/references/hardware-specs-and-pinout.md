# ESP32-P4-Module-DEV-KIT 硬件规格与引脚映射

本文档基于微雪官方文档（https://docs.waveshare.net/ESP32-P4-Module-DEV-KIT）和原理图设计；引脚编号以官方页面的“引脚定义”图为准。

---

## 1. 核心系统规格

| 模块 | 关键技术规格 |
| :--- | :--- |
| **SoC** | **ESP32-P4NRW32** (双核 32-bit RISC-V @ 360MHz，DSP/FPU/SIMD 扩展；LP 单核 RISC-V @ 40MHz) |
| **内存与存储** | 封装内叠封 **32MB PSRAM**，板载 **16MB Nor Flash**，768KB HP L2MEM，32KB LP SRAM，8KB TCM |
| **无线协处理器** | **ESP32-C6**，提供 2.4GHz Wi-Fi 6 (802.11ax/b/g/n) 与 Bluetooth 5 (LE) |
| **板载以太网** | 100Mbps 百兆 RJ45 以太网口（支持外接 PoE 模块供电） |
| **USB 接口** | 1x Type-C UART（CH343P USB 转串口调试与供电）；1x Type-C USB（ESP32-P4 原生 USB）；1x Type-A USB 2.0 OTG High Speed（支持 Host/Device 跳线切换） |
| **多媒体与音视频** | MIPI-DSI (2-lane) 高清屏接口、MIPI-CSI (2-lane) 摄像头接口、板载 ES8311 音频 Codec、NS4150B 功放、3.5mm 耳机插孔、贴片麦克风、MX1.25 喇叭接口 |
| **存储扩展** | MicroSD (TF) 卡槽，采用 SDIO 3.0 (4-bit SDMMC) |

---

## 2. 板载专用引脚与硬件占用

开发板上部分 GPIO 已连接到板载芯片和外设，设计底板或外挂模块时需注意避免冲突：

### 2.1 SDIO 3.0 MicroSD 卡槽 (4-bit SDMMC)
- **CLK**: `GPIO43`
- **CMD**: `GPIO44`
- **D0**: `GPIO39`
- **D1**: `GPIO40`
- **D2**: `GPIO41`
- **D3**: `GPIO42`

### 2.2 音频编解码 (ES8311 + NS4150B 功放)
- **MCLK**: `GPIO13`
- **BCLK**: `GPIO12`
- **WS (LRCK)**: `GPIO10`
- **DOUT (DAC)**: `GPIO9`
- **DIN (ADC)**: `GPIO11`
- **PA_EN (功放控制)**: `GPIO53`

### 2.3 板载 I2C 传感器与控制总线 (I2C0)
- **SDA**: `GPIO7`
- **SCL**: `GPIO8`
- *注：用于控制 ES8311、MIPI-DSI 触摸芯片、MIPI-CSI 摄像头模组配置*

### 2.4 USB、调试与其他共享资源

- CH343P 调试 UART0：TX=`GPIO37`、RX=`GPIO38`，连接器 P6 也引出这两个网络。
- 原生 USB 1.1 Type-C：D−=`GPIO24`、D+=`GPIO25`。
- USB High-Speed：模组专用 DP/DM，经 USB 切换芯片连接 USB 接口。
- `GPIO26`、`GPIO27` 在官方原理图中作为扩展 GPIO 引出。
- `GPIO32`、`GPIO33` 接 J9 扩展口，具有板载 2.2 kΩ 上拉。
- `GPIO45` 接 MicroSD 电源 MOS 控制网络；`GPIO53` 接功放控制网络。
- `GPIO34`～`GPIO38` 涉及芯片启动配置，分配外设时需核对启动采样条件；`GPIO35` 还接 BOOT 与以太网网络。
- ESP32-C6 通过模组内部 SDIO 连接；P6 引出的 `GPIO20` 可按原理图用作扩展 GPIO。

## 3. P6 40 针扩展排针

依据 [官方引脚图](https://www.waveshare.com/w/upload/3/3b/ESP32-P4-Module-DEV-KIT-details-intro.jpg) 与 [官方原理图](https://files.waveshare.com/wiki/ESP32-P4-Module-DEV-KIT/ESP32-P4-Module-DEV-KIT.pdf) 第 1 页核验，日期 2026-09-10。

观察元件面，将 40 针排针放在板子右侧，C6 UART 接口位于上方，USB HOST/DEVICE 跳帽位于下方。从上往下数 20 行，左列靠主控、右列靠板边。P6 左列为偶数脚，右列为奇数脚；行号与 GPIO 编号分别记录。

| 从上数行号 | 左列 P6 脚号 | 左列信号 | 右列 P6 脚号 | 右列信号 |
| ---: | ---: | --- | ---: | --- |
| 1 | 2 | 3V3 | 1 | 5V |
| 2 | 4 | GPIO7 / SDA | 3 | 5V |
| 3 | 6 | GPIO8 / SCL | 5 | GND |
| 4 | 8 | GPIO23 | 7 | GPIO37 / 调试 TX |
| 5 | 10 | GND | 9 | GPIO38 / 调试 RX |
| 6 | 12 | GPIO21 | 11 | GPIO22 |
| 7 | 14 | GPIO20 | 13 | GND |
| 8 | 16 | GPIO6 | 15 | GPIO5 |
| 9 | 18 | 3V3 | 17 | GPIO4 |
| 10 | 20 | GPIO3 | 19 | GND |
| 11 | 22 | GPIO2 | 21 | GPIO1 |
| 12 | 24 | GPIO0 | 23 | GPIO36 |
| 13 | 26 | GND | 25 | GPIO32 |
| 14 | 28 | GPIO24 / USB D− | 27 | GPIO25 / USB D+ |
| 15 | 30 | GPIO33 | 29 | GND |
| 16 | 32 | GPIO26 | 31 | GPIO54 |
| 17 | 34 | GPIO48 | 33 | GND |
| 18 | 36 | GPIO53 / 功放控制 | 35 | GPIO46 |
| 19 | 38 | GPIO47 | 37 | GPIO27 |
| 20 | 40 | GND | 39 | GPIO45 / SD 电源控制 |

接线采用本表和实物丝印共同定位。P6 的 3V3 为板载稳压输出，屏幕电流预算需包含刷新峰值与主板同时运行负载。
