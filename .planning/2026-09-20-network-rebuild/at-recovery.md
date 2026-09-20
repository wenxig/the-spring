# EC600M AT 恢复

## 提交划分
1. 恢复历史接口 3 和 CRLF：5ac5bf1，构建、烧录成功；实机 AT 超时。
2. 恢复 USB 接收缓冲与解析任务分工，保留 esp_modem PPP；验证并提交。

## 证据
- 1e38b37：UART1 GPIO0/1、115200 8N1、CRLF、逐行终结符。
- 0a09c06：互斥锁覆盖完整事务。
- 009a6c3：CDC Host 打开 interface 3，512 字节发送、2048 字节接收，回调写环形缓冲、命令任务读取；提交说明记录 AT/ATI/CPIN/CSQ 成功。
- 当前 CDC Host 2.4.1 支持 vendor-specific 双 bulk 接口，历史与当前版本相同。
- 5ac5bf1 实机：interface 3 打开，AT timeout response_bytes=0，一次 tlsf_free 堆断言；Wi-Fi 获取 IP。
- USB DTE 1.3.1 直接回调 DTE；esp_modem 1.4.3 的 inflatable 缓冲在 RX 与 command 超时路径均修改，关闭动态扩容并采用固定接收缓冲。

## 当前修复动作

- 保留默认 `CONFIG_SPRING_MODEM_USB_AT_INTERFACE=3`，并保留配置项以适配
  不同 EC600M 固件的 USB 描述符。

## 2026-09-20 实机验证

- 恢复独立 `cdc_acm_host` 终端，并由 `esp_modem::DTE` 复用该终端，避免
  `esp_modem_usb_dte` 自己重新打开 EC600M 专用端口。
- ESP32-P4 rev1.3 + EC600MCNLER06A08M08 烧录通过。
- Interface 3 打开成功；EC600M 对 CDC line coding/control 请求返回
  `ESP_ERR_NOT_SUPPORTED`，符合该 vendor-specific bulk 端口特征。
- `AT`、`ATI`、`ATE0`、`AT+CEREG=2`、`AT+CEREG?`、`AT+CGATT?` 均收到实际 RX。
- `ATI` 返回 `Quectel EC600M`、`Revision: EC600MCNLER06A08M08`。
- PPP 拨号发送 `ATD*99#` 并收到 `CONNECT`，同一专用 AT 端口可以进入数据态。
- 天气首次刷新因定位不可用跳过；Wi-Fi 获取 IPv4 正常。
