# ESP32-P4-WIFI6-DEV-KIT 硬件规格与引脚映射

本文档基于微雪官方文档（https://docs.waveshare.net/ESP32-P4-WIFI6-DEV-KIT）和原理图设计。

---

## 1. 核心系统规格

| 模块 | 关键技术规格 |
| :--- | :--- |
| **SoC** | **ESP32-P4NRW32** (双核 32-bit RISC-V @ 360MHz，DSP/FPU/SIMD 扩展；LP 单核 RISC-V @ 40MHz) |
| **内存与存储** | 封装内叠封 **32MB PSRAM**，板载 **16MB Nor Flash**，768KB HP L2MEM，32KB LP SRAM，8KB TCM |
| **无线协处理器** | **ESP32-C6-MINI-1**，提供 2.4GHz Wi-Fi 6 (802.11ax/b/g/n) 与 Bluetooth 5 (LE) |
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

### 2.4 USB 2.0 High-Speed OTG (480Mbps)
- **DP**: `GPIO27`
- **DM**: `GPIO26`
- 可通过板载跳线帽在 Host / Device 之间切换控制

### 2.5 ESP32-C6 无线通信总线
- 内部通过专用 SDIO 总线及流控/中断引脚（GPIO14~GPIO20 区域内部专用）与 ESP32-P4 互联通信。

---

## 3. 2x20 40-Pin 扩展端子引脚定义

开发板引出 28 个可编程 GPIO，引脚排列兼容树莓派 Pico 扩展生态：

| 引脚编号 | 左侧定义 (Pin 1~20) | 引脚编号 | 右侧定义 (Pin 21~40) |
| :--- | :--- | :--- | :--- |
| **1** | **GPIO0** (UART0 TX / PWM / ADC) | **40** | **VBUS / VSYS** (5V 输入电源轨) |
| **2** | **GPIO1** (UART0 RX / PWM / ADC) | **39** | **VSYS** (系统 5V 轨) |
| **3** | **GND** | **38** | **GND** |
| **4** | **GPIO2** (通用 GPIO / SPI SCK) | **37** | **3V3_EN** (3.3V DCDC 降压使能) |
| **5** | **GPIO3** (通用 GPIO / SPI MOSI) | **36** | **3V3_OUT** (板载 3.3V 输出) |
| **6** | **GPIO4** (通用 GPIO / SPI MISO) | **35** | **ADC_VREF** (ADC 参考电压) |
| **7** | **GPIO5** (通用 GPIO / SPI CS) | **34** | **GPIO25** (ADC 通道 / 通用 GPIO) |
| **8** | **GND** | **33** | **GND** |
| **9** | **GPIO6** (通用 GPIO / I2C SDA) | **32** | **GPIO24** (ADC 通道 / 通用 GPIO) |
| **10** | **GPIO7** (I2C0 SDA / 通用 GPIO) | **31** | **GPIO23** (ADC 通道 / 通用 GPIO) |
| **11** | **GPIO8** (I2C0 SCL / 通用 GPIO) | **30** | **GPIO22** (ADC 通道 / 通用 GPIO) |
| **12** | **GPIO9** (I2S DOUT / 通用 GPIO) | **29** | **GPIO21** (通用 GPIO) |
| **13** | **GND** | **28** | **GND** |
| **14** | **GPIO10** (I2S WS / 通用 GPIO) | **27** | **GPIO20** (通用 GPIO) |
| **15** | **GPIO11** (I2S DIN / 通用 GPIO) | **26** | **GPIO19** (通用 GPIO) |
| **16** | **GPIO12** (I2S BCLK / 通用 GPIO) | **25** | **GPIO18** (通用 GPIO) |
| **17** | **GPIO13** (I2S MCLK / 通用 GPIO) | **24** | **GPIO17** (通用 GPIO) |
| **18** | **GND** | **23** | **GND** |
| **19** | **GPIO14** (通用 GPIO) | **22** | **GPIO16** (通用 GPIO) |
| **20** | **GPIO15** (通用 GPIO) | **21** | **GPIO32** (通用 GPIO) |
