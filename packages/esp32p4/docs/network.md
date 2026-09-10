# 网络抽象与自动回退

应用层只使用 `NetworkClient` 的 `get`、`post` 和统一响应错误类型，不感知 Wi-Fi、蜂窝 UART 或具体 socket 实现。

链路管理器维护 Wi-Fi 与蜂窝两个传输适配器，默认 Wi-Fi 优先；Wi-Fi 未连接、认证失败或请求连续超时后切换蜂窝。恢复 Wi-Fi 后通过稳定窗口确认再切回。请求使用有限重试、指数退避和取消令牌，禁止在 LVGL、电子纸或按键任务中同步执行网络 I/O。

HTTPS 使用 ESP-TLS 根证书校验。蜂窝数据拨号由 `esp_modem` 管理，网络适配器向上提供相同的 HTTP 客户端语义。服务器端 OTA 当前只要求提供 HTTPS 固件 URL，设备侧后续接入 `esp_https_ota` 并依赖双槽回滚。
