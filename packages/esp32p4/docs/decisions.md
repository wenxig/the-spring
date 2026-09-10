# 技术决策记录

## 已确定

- ESP-IDF：首版以 5.5 为基线，使用 ESP-IDF 原生 FreeRTOS、GPIO、SPI、UART 驱动。
- OTA：采用 `otadata + ota_0 + ota_1` 双槽和应用回滚；服务器部署方式暂不实现，固件只保留 HTTPS OTA 客户端边界。
- UI：LVGL 9；UI 任务独占 LVGL，显示 flush 通过电子纸适配层串行化。
- Modem：`esp_modem` 封装 UART 和 AT 通道，应用层维护注册、拨号、URC 状态机。
- 电子纸：`esp_epaper` 负责 SSD1683 时序；400x300、单色 15,000 字节帧缓冲。若其 managed component 与目标 IDF/LVGL 适配不稳定，将在项目内封装同一接口并锁定已验证版本。
- 网络：上层只依赖 `NetworkClient` 的 GET/POST 接口；底层按 Wi-Fi 优先、蜂窝网络回退自动选择链路，链路断开时指数退避并恢复请求队列。
- 数据：SQLite 使用 SD 卡单连接存储服务，WAL、短事务和参数绑定；网络请求由独立服务执行，避免 UI/驱动任务阻塞。

## 待讨论

- Wi-Fi/C6 协处理器与蜂窝网络的凭据、优先级和计费策略。
- NVS 加密、安全启动和证书存储策略。
- MQTT 是否作为长期连接能力加入上层 GET/POST 之外的消息接口。
