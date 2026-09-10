# ESP32-P4 外设与扩展接口开发指南

本文档指导在 ESP32-P4-Module-DEV-KIT 上外接外设、总线扩展与硬件互联开发。

---

## 1. 外部通信与传感器连接

在本项目架构中，ESP32-P4 作为核心主控，需要挂载：
1. **SPI 水墨屏 (E-Paper)**
2. **移远 EC600X 开发板**
3. **各类 I2C / ADC 传感器**

### 1.1 SPI 水墨屏驱动引脚

项目 QYEG0420BNS830 驱动板采用四线 SPI：

| 信号 | GPIO | P6 脚号 | 位置 |
| --- | ---: | ---: | --- |
| SCK | 2 | 22 | 第 11 行左 |
| SDI / MOSI | 3 | 20 | 第 10 行左 |
| CS | 5 | 15 | 第 8 行右 |
| D/C | 6 | 16 | 第 8 行左 |
| RES | 21 | 12 | 第 6 行左 |
| BUSY | 22 | 11 | 第 6 行右 |

位置观察方向见 [排针表](hardware-specs-and-pinout.md)。驱动板八针顺序与电源定义见 [屏幕硬件参考](../../qyeg0420bns830/references/hardware.md)。

### 1.2 EC600X 通信引脚

- ESP32-P4 GPIO0（P6-24，第 12 行左）TX → EC600X J5-7 RX0。
- ESP32-P4 GPIO1（P6-21，第 11 行右）RX ← EC600X J5-6 TX0。
- ESP32-P4 P6-26（第 13 行左，GND）↔ EC600X J5-1 GND。
- 固件使用独立 UART1，并通过 GPIO matrix 映射到 GPIO0/1；UART0 调试保持 GPIO37/38。EC 端的 TX0/RX0 表示 EC 主串口。
- EC600X-EVB V3.2 的 J5 串口信号经板载电平转换处于 3.3V 域。两块开发板分别 USB 供电，公共信号地相连。
- EC 开关机与复位使用板载 PWRKEY/RESET；J5/J6 的可插接信号不包含 PWRKEY、RESET_N、MAIN_RI 和 PCM。

---

## 2. 音频系统开发 (ES8311 + NS4150B)

ESP32-P4-Module-DEV-KIT 集成了 ES8311 低功耗音频编解码芯片与 NS4150B 功放，适用于语音对讲、提示音播放与麦克风拾音。

### 2.1 硬件配置参数
- **I2C 控制总线**: I2C0 (`SDA=GPIO7`, `SCL=GPIO8`), 芯片 I2C 从机地址 `0x18` (或 `0x30` 写)
- **I2S 标准**: Philips 格式，16-bit / 24-bit，标准采样率 16kHz / 44.1kHz / 48kHz
- **功放使能**: `GPIO53` 输出高电平开启功放（静音时拉低以减少噪声与功耗）

### 2.2 ESP-ADF / ESP-IDF 驱动流程
1. 初始化 I2C 总线并检测 ES8311 设备。
2. 配置 ES8311 寄存器（时钟源设定为 MCLK，配置 DAC 增益与 ADC 前级偏置）。
3. 初始化 ESP32-P4 I2S 外设驱动（使用 `driver/i2s_std.h` 新版 API）。
4. 拉高 `GPIO53` 使能板载音频功放。

---

## 3. 摄像头与显示屏接口 (MIPI)

### 3.1 MIPI-CSI 摄像头
- 接口：15-Pin 1.0mm FPC 接口，2-lane MIPI-CSI
- 支持传感器：OV5647、SC2336 等
- 硬件 ISP：支持自动曝光 (AEC)、自动白平衡 (AWB)、自动对焦 (AF)、坏点校正与降噪
- 硬件编解码：配合 ESP32-P4 内置 H.264 编码器，可实现 1080P@30fps 流媒体采集

### 3.2 MIPI-DSI 显示屏
- 接口：15-Pin 1.0mm FPC 接口，2-lane MIPI-DSI，最高 1.5 Gbps/lane
- 支持分辨率：720P / 1080P，支持搭配微雪 3.5寸、4.3寸、5寸、7寸 DSI 触控屏
- 图形加速：利用 PPA (Pixel Processing Accelerator) 与 2D-DMA 进行图像旋转、缩放与色空间转换 (YUV420 to RGB565)
