# ESP32-P4 外设与扩展接口开发指南

本文档指导在 ESP32-P4-WIFI6-DEV-KIT 上外接外设、总线扩展与硬件互联开发。

---

## 1. 外部通信与传感器连接

在本项目架构中，ESP32-P4 作为核心主控，需要挂载：
1. **SPI 水墨屏 (E-Paper)**
2. **移远 EC600X 开发板**
3. **各类 I2C / ADC 传感器**

### 1.1 SPI 水墨屏驱动引脚建议
水墨屏通常需要 4 线或 3 线 SPI 接口加控制引脚（BUSY, RST, DC, CS）：
- **SCLK**: `GPIO2` (40-Pin Pin 4)
- **MOSI (DIN)**: `GPIO3` (40-Pin Pin 5)
- **CS**: `GPIO5` (40-Pin Pin 7)
- **DC (Data/Command)**: `GPIO6` (40-Pin Pin 9)
- **RST (Reset)**: `GPIO21` (40-Pin Pin 29)
- **BUSY**: `GPIO22` (40-Pin Pin 30，配置为输入中断)

### 1.2 EC600X 通信引脚规划
EC600X 与 ESP32-P4 的通信推荐走硬件 UART：
- **ESP32-P4 TX -> EC600X RX**: 使用 `GPIO0` (Pin 1) 或配置独立串口引脚如 `GPIO24`
- **ESP32-P4 RX <- EC600X TX**: 使用 `GPIO1` (Pin 2) 或配置独立串口引脚如 `GPIO25`
- **PWRKEY 唤醒/开机引脚**: 使用 `GPIO17` (Pin 24) 通过晶体管/MOS 驱动拉低 700ms~1000ms
- **RESET_N 紧急复位**: 使用 `GPIO18` (Pin 25)
- **MAIN_RI 唤醒中断**: 使用 `GPIO19` (Pin 26，配置外部双沿中断)

> **电平注意**：EC600X 开发板对外排针通常已处理为 3.3V 接口，但若直接连模组裸口（1.8V），必须严格串联双向电平转换芯片（如 TXS0108E）。

---

## 2. 音频系统开发 (ES8311 + NS4150B)

ESP32-P4-WIFI6-DEV-KIT 集成了 ES8311 低功耗音频编解码芯片与 NS4150B 功放，适用于语音对讲、提示音播放与麦克风拾音。

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
