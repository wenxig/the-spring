---
name: quectel-ec600x-evb
description: 移远 (Quectel) EC600X-EVB 与项目实装 EC600MCNLE 的硬件集成指南。用于核对 J5/J6 排针、供电与电平、板载音频，以及 ESP32-P4 通过 UART/AT 指令管理蜂窝网络、数据连接、VoLTE 和 URC 的实现。
---

# 移远 EC600X-EVB 开发板指南

> 官方文档：<https://developer.quectel.com/doc/quecpython/Dev_board_guide/zh/ec600x-evb.html>
> 支持模组：**EC600N-CN、EC600U-CN、EC600M-CN**（LTE Cat 1 bis）
> 项目实装：移远 **EC600MCNLE**
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
| **项目开发模式** | ESP32-P4 通过 UART 发送 **AT 指令**并解析响应与 URC；EC600MCNLE 保持 modem 固件运行 |

---

## 模块化参考资料索引

| 主题 | 文档路径 | 核心要点 |
| :--- | :--- | :--- |
| **EVB 硬件架构与引脚映射** | [`evb-hardware-and-pinout.md`](references/evb-hardware-and-pinout.md) | J5/J6 18-Pin 排针完整映射、开机时序、板载电源切换与电平隔离避坑 |
| **EC600MCNLE AT 指令网络接入指南** | [`ec600mcnle-at-commands.md`](references/ec600mcnle-at-commands.md) | PDP 激活流程、TCP/UDP Socket、MQTT 物联网通信、低功耗休眠模式 (QSCLK) |

---

## 快速上手与工程实践

### 1. 项目控制边界
- 网络注册、数据拨号、信号查询、基站定位信息和 VoLTE 控制由 ESP32-P4 的 C++ 服务封装。
- 不为 EC600MCNLE 新增 QuecPython 应用、Raw REPL 上传流程或依赖 QuecPython 运行时的部署脚本。
- AT 服务负责超时、重试、命令响应匹配、URC 分发和连接状态机。

### 2. 与 ESP32-P4 互联
- **物理接线 (J5)**:
  - `J5 Pin 6 (TX0 / UART0)` -> ESP32-P4 对应 UART RX
  - `J5 Pin 7 (RX0 / UART0)` <- ESP32-P4 对应 UART TX
  - `J5 Pin 1/2 (GND)` <-> ESP32-P4 GND
- **控制引脚**:
  - 官方 J5/J6 排针未引出 **PWRKEY、RESET_N、MAIN_RI**；项目需要按所装模组硬件手册另行设计连接路径。
  - ESP32-P4 GPIO 通过匹配的晶体管/MOS 控制电路下拉 **PWRKEY**，通过匹配的复位电路控制 **RESET_N**，并通过匹配的输入电路监听 **MAIN_RI**；时序和电平按所装模组手册执行。
- **通信协议**:
  - 采用 EC600MCNLE 当前固件支持的 3GPP 与移远扩展 AT 指令；具体命令和 URC 以对应版本的 AT 手册为准。
  - 最小链路验证依次执行 `AT`、`AT+CPIN?`、`AT+CEREG?`、`AT+CSQ`，再进入数据拨号或 VoLTE 流程。
