---
name: quecpython
description: 移远通信 (Quectel) QuecPython 嵌入式物联网开发全栈指南。深入覆盖蜂窝网络与数据拨号 (dataCall/net/checkNet/sim/sms/voiceCall/volte)、硬件外设控制 (machine Pin/UART/I2C/SPI/WDT, misc ADC/PWM)、低功耗电源管理 (pm/autosleep/wakelock)、物联网网络协议 (usocket/ussl/request/umqtt)、主流云平台对接 (阿里云 aLiYun/腾讯云 TXyun)、并发多任务架构 (_thread/Queue/sys_bus/uasyncio)、文件系统与 OTA 固件/应用热更新 (fota/app_fota)。适用于移远全系列 Cat 1 / Cat 4 / NB-IoT / 5G 模组开发。
---

# QuecPython 嵌入式全栈开发指南

QuecPython 是移远通信针对蜂窝物联网模组打造的 MicroPython 运行环境与生态，允许开发者直接在移远通信模组（如 EC600M、EC600N、EC200U、EC800M、EG915N 等）内部运行 Python 3 脚本，大幅精简外围主控 MCU 并缩短产品上市周期。

## 目录结构与快速导航

本 Skill 配备了针对 QuecPython 官方文档 (https://developer.quectel.com/doc/quecpython/) 核心体系的五部深度参考手册：

1. **蜂窝通信与网络服务**: `references/cellular-and-network-services.md`
   - 注网就绪与状态监听 (`checkNet`, `net`)
   - PDP 激活与 APN 拨号生命周期 (`dataCall`)
   - SIM 卡信息读取与状态管理 (`sim`)
   - 基站基准定位 (`cellLocator`)
2. **短信通信与 VoLTE 语音通话**: `references/sms-and-volte-telephony.md`
   - 短信存储位置与容量管理 (`sms.setSaveLoc`, `sms.getSaveLoc`)
   - 中英文短信发送与 PDU 编解码 (`sms.sendTextMsg`, `sms.decodePdu`)
   - 短信来信事件异步回调与处理 (`sms.setCallback`)
   - VoLTE 通话控制与事件状态机 (`voiceCall.callStart`, `voiceCall.callAnswer`, `voiceCall.setCallback`)
   - 音频输出通道与音量调节 (`voiceCall.setChannel`, `voiceCall.setVolume`)
   - 交互式 DTMF 按键识别与发送 (`voiceCall.dtmfSetCb`, `voiceCall.startDtmf`)
   - 双向通话录音与流数据读取 (`voiceCall.startRecord`, `voiceCall.startRecordStream`)
   - 工业级 TelephonyManager 电话与告警短信守护单例模式
3. **硬件外设与电源管理**: `references/peripherals-and-power-management.md`
   - GPIO 输入输出与外部中断 (`machine.Pin`, `machine.ExtInt`)
   - 串口多实例与异步回调 (`machine.UART`)
   - 硬件总线 (`machine.I2C`, `machine.SPI`)
   - 模拟量采集与调光驱动 (`misc.ADC`, `misc.PWM`)
   - 硬件看门狗容错 (`machine.WDT`)
   - 电源管理与自动休眠 (`pm.autosleep`, `pm.wakelock`)
4. **物联网网络协议与云平台**: `references/network-protocols-and-cloud.md`
   - 基础 TCP/UDP 及 TLS/SSL 加密套接字 (`usocket`, `ussl`)
   - 高级 HTTP/HTTPS 请求客户端 (`request`)
   - 工业级 MQTT 客户端与断线重连状态机 (`umqtt.MQTTClient`)
   - 阿里云与腾讯云 IoT 平台直连 SDK (`aLiYun`, `TXyun`)
5. **系统架构、多任务与工程实践**: `references/system-and-best-practices.md`
   - 多线程并发与堆栈分配控制 (`_thread`, `queue.Queue`)
   - 系统级事件发布与订阅总线 (`sys_bus`)
   - 协作式异步协程开发 (`uasyncio`)
   - 闪存文件系统与空间管理 (`uos`, `ql_fs`)
   - 应用热更新与整机固件升级 (`app_fota`, `fota`)
   - 工业级 `_main.py` 启动守护与崩溃兜底架构

---

## 核心架构原则与设计规约

### 1. 网络优先检查机制
在进行任何网络 Socket、HTTP 请求或 MQTT 连接前，必须通过 `checkNet.wait_network_connected()` 确认蜂窝基站注册和数据通道激活，切忌在开机后直接调用连接函数。

### 2. 内存敏感性与垃圾回收
- 模组堆内存宝贵，严格控制全局对象创建。
- 使用 `gc.collect()` 在批量网络收发或大对象解析后主动整理内存碎片。
- 在多线程环境下，务必通过 `_thread.stack_size(8192)` 控制线程栈大小，避免栈溢出导致模组静默重启。

### 3. 异步驱动与总线解耦
- 尽量避免长时阻塞的 `while True` 延时轮询，优先使用 UART 回调、Pin 外部中断、Timer 定时器或 `sys_bus` 事件驱动。
- 在复杂的跨业务协作中，使用 `sys_bus.subscribe` 和 `sys_bus.publish` 保持模块松耦合。

### 4. 低功耗场景下的唤醒锁管理
- 在电池供电设备中开启 `pm.autosleep(1)`。
- 在执行串口数据收发、ADC 采样或网络通信时，使用 `lock = pm.create_wakelock(...)` 保护业务执行；任务完成后务必 `lock.release()`，确保设备能够休眠进入微安级待机。

### 5. 故障保护与安全启动
- 生产环境脚本存放于 `/usr/_main.py`。
- 必须开启硬件看门狗 `machine.WDT` 并定期喂狗。
- 将未捕获异常记录于持久化日志文件 `/usr/crash.log`，以便于野外排错。
