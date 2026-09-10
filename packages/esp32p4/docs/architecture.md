# 模块化系统架构

## 分层

- **BSP/Drivers**：GPIO、SPI、UART、SDMMC、RTC 和板级电源，仅暴露硬件能力。
- **System Services**：`InputService`、`DisplayService`、`StorageService`、`NetworkService`、`ModemService`、`ClockService`，负责生命周期、任务和资源所有权。
- **Application Runtime**：应用注册表、应用生命周期、事件总线和当前应用路由。
- **Apps**：数字时钟、网络状态、定位、通话、设置等产品功能，以应用接口访问系统服务。

## 应用模型

每个应用实现统一生命周期：`install`、`start`、`pause`、`resume`、`stop`，并声明名称、入口界面、需要订阅的事件和持久化命名空间。`AppManager` 同一时刻运行一个前台应用，可保留后台服务应用；S8 发送 `NavigateHome`，S7 发送 `SleepRequested`。

应用通过依赖注入获得 `SystemContext`，其中包含时钟、显示、输入、网络、存储、定位和通话接口。应用不能直接操作 GPIO、UART、SPI 或 SQLite 连接。

## 事件与数据

系统服务发布类型化事件，应用订阅后更新自身状态。跨模块事实通过统一快照和 revision 传递；UI 应用不复制 modem、网络或时间状态。StorageService 负责将选定事件持久化到 SQLite。

## 资源与并发

每个服务拥有自己的 FreeRTOS task、队列和同步对象；跨服务调用使用非阻塞消息或异步 future。DisplayService 独占 LVGL 和电子纸总线，ModemService 独占 AT 队列，StorageService 独占 SQLite 连接。

## 首版边界

首版不做应用权限、安全策略、进程隔离或沙箱。应用运行在同一固件地址空间，模块边界通过 C++ 接口、依赖方向、任务所有权和代码审查维护。
