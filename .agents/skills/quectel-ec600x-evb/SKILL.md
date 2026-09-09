---
name: quectel-ec600x-evb
description: 移远 (Quectel) EC600X-EVB 开发板全栈开发指南。涵盖 EC600X 模组硬件架构、供电与时序、J5/J6 双排针定义、板载 AHT20 温湿度传感器 (I2C) 与 GT36528 光敏电阻 (ADC) 驱动、NS4160 音频功放、QuecPython 编程以及 AT 指令蜂窝通信协议栈 (PDP 激活、Socket、MQTT、低功耗休眠与 URC 事件处理)。
---

# 移远 EC600X-EVB 开发板指南

> 官方文档：<https://developer.quectel.com/doc/quecpython/Dev_board_guide/zh/ec600x-evb.html>
> 支持模组：**EC600N-CN、EC600U-CN、EC600M-CN**（LTE Cat 1 bis）
> 项目实装：移远 **EC600M-CN**
> 角色定位：项目中 4G/蜂窝网络、移动通信与环境传感采集板，受主控板 (ESP32-P4) 统领调度  

---

## 硬件核心规格

| 项目 | 技术指标与参数 |
| :--- | :--- |
| **蜂窝通信** | LTE Cat 1 bis，最大下行速率 10Mbps，最大上行速率 5Mbps，支持 VoLTE 与短信功能 |
| **供电方式** | Type-C 5V 接口或外部 DC 电源（通过板载电源拨动开关在 USB / DC 间切换） |
| **板载排针** | 2 组 18-Pin 2.54mm 排针 (J5 与 J6)，引出串口、I2C、SPI、ADC、PWM、喇叭及电源引脚 |
| **板载传感器** | **AHT20** 温湿度传感器（挂载于 I2C1 总线）、**GT36528** 光敏电阻（连接至 ADC0） |
| **音频系统** | **GMI6050P-66DB** 驻极体麦克风、**NS4160** 音频功率放大器、板载扬声器接线柱 |
| **接口与按键** | Micro SIM 自弹式卡座、SMA/IPEX 天线底座、PWRKEY 按键、RESET 按键、NET 网络指示灯 |
| **开发模式** | **QuecPython** 原生 Python 脚本独立运行模式，或 **AT 指令**受控调制解调器模式 |

---

## 模块化参考资料索引

| 主题 | 文档路径 | 核心要点 |
| :--- | :--- | :--- |
| **EVB 硬件架构与引脚映射** | [`evb-hardware-and-pinout.md`](references/evb-hardware-and-pinout.md) | J5/J6 18-Pin 排针完整映射、开机时序、板载电源切换与电平隔离避坑 |
| **QuecPython 基础与传感器驱动** | [`quecpython-guide.md`](references/quecpython-guide.md) | QPYcom 工具链、板载 AHT20 温湿度 I2C 驱动、GT36528 ADC 采样、音频播放 |
| **EC600X AT 指令网络接入指南** | [`ec600m-at-commands.md`](references/ec600m-at-commands.md) | PDP 激活流程、TCP/UDP Socket、MQTT 物联网通信、低功耗休眠模式 (QSCLK) |

---

## 快速上手与工程实践

### 1. 独立运行模式 (QuecPython)
- 使用 Type-C 数据线连接开发板，滑动开关拨至 **USB** 挡。
- 打开 **QPYcom** 工具，选择 USB CDC 串口进入 REPL 交互。
- 上传 `AHT20` 与光敏电阻采集脚本，快速验证环境采集与数据上报逻辑。

### 2. 作为外部通信模组与 ESP32-P4 互联 (AT 模式)
- **物理接线 (J5)**:
  - `J5 Pin 6 (TX0 / UART0)` -> ESP32-P4 对应 UART RX
  - `J5 Pin 7 (RX0 / UART0)` <- ESP32-P4 对应 UART TX
  - `J5 Pin 1/2 (GND)` <-> ESP32-P4 GND
- **控制引脚**:
  - 官方 J5/J6 排针未引出 **PWRKEY、RESET_N、MAIN_RI**；项目需要按所装模组硬件手册另行设计连接路径。
  - ESP32-P4 GPIO 通过匹配的晶体管/MOS 控制电路下拉 **PWRKEY**，通过匹配的复位电路控制 **RESET_N**，并通过匹配的输入电路监听 **MAIN_RI**；时序和电平按所装模组手册执行。
- **通信协议**:
  - 采用标准 3GPP 与移远标准扩展 AT 指令集，通过 `AT+QIACT` 激活网络、`AT+QMT*` 进行 MQTT 消息订阅与发布。
