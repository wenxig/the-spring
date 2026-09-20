# 模块化系统架构

## 分层

- **BSP/Drivers**：GPIO、SPI、UART、USB Host、SDMMC、RTC 和板级电源。
- **System Services**：`network_service`、`wifi_service`、`modem_service`、`weather_service`、`DisplayService`、`StorageService` 和 `ClockService`。
- **Application Runtime**：应用注册、事件分发、渲染与天气轮询任务。
- **Apps**：数字时钟、网络状态、定位、通话和设置。

`network_service` 只负责请求调度、取消、链路选择和统一响应。Wi-Fi、蜂窝硬件初始化分别由对应 service 持有；`weather_service` 负责天气 URL、JSON 解析、fallback、快照和 revision。`app_manager` 只读取天气快照，不依赖网络协议或 JSON。

## 启动顺序

1. modem service 创建 USB CDC-ACM/PPP 管理任务。
2. Wi-Fi service 初始化 ESP-Hosted C6 STA。
3. 注册 Wi-Fi 与蜂窝 HTTP transport。
4. 启动 network service。
5. 启动天气服务和 10 分钟轮询。
6. 启动 UI、输入和其他应用服务。

## 并发与所有权

网络回调由 network task 串行执行。modem service 使用事务 mutex 保护 AT 命令、PPP 模式切换和蜂窝 HTTP。天气快照由 mutex 保护，轮询任务使用 `xTaskDelayUntil` 保持固定节奏。Wi-Fi 与蜂窝的状态事件只更新 transport 可用性，不直接触发业务。
