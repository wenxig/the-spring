# 技术决策记录

- ESP-IDF 构建基线锁定为项目当前的 v6.0.2，目标为 ESP32-P4。
- EC600MCNLE 默认使用 `espressif/esp_modem_usb_dte` 1.3.1、USB Host CDC-ACM、`esp_modem` 1.4.3 和 PPP；UART1/GPIO0-GPIO1 作为 Kconfig 兼容路径。
- 网络公共 API 使用拥有型二进制 body、重复 header 列表、异步回调、请求取消和 Wi-Fi 优先/蜂窝单次 fallback。
- HTTP transport 绑定具体 `esp_netif`，统一 TLS、重定向、超时和响应收集。
- 天气服务与网络框架分离，启动立即刷新，之后每 10 分钟轮询；刷新失败保留上一份有效快照或 fallback。
- OTA 保留双槽回滚配置；服务器部署方式不属于网络框架。
