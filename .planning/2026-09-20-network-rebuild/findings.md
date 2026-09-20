# 发现
- ESP-IDF 6.0.2；esp_modem 1.4.3。官方 USB DTE 为独立 esp_modem_usb_dte 组件，解析到 1.3.1，支持 ESP32-P4。
- esp_http_client_set_header 会覆盖同名字段；需要有序字段编码和覆盖测试。
- open() 包含发送请求头；连接事件标记保守的 fallback 边界。
- managed_components 原跟踪链接指向自身，已用生成目录替换，需清理跟踪链接以保证可复现构建。
