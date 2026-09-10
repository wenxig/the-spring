# 技术决策记录

## 已确定

- ESP-IDF：首版以 5.5 为基线，使用 ESP-IDF 原生 FreeRTOS、GPIO、SPI、UART 驱动。
- UI：LVGL 9；UI 任务独占 LVGL，显示 flush 通过电子纸适配层串行化。
- Modem：`esp_modem` 封装 UART 和 AT 通道，应用层维护注册、拨号、URC 状态机。
- 电子纸：`esp_epaper` 负责 SSD1683 时序；400x300、单色 15,000 字节帧缓冲。

## 待讨论

- 是否启用 Wi-Fi/C6 协处理器及以太网。
- 是否首版加入 OTA 双分区、NVS 加密和安全启动。
- 数据协议（HTTP/MQTT）与本地存储（SD/NVS/LittleFS）。
- `esp_epaper` 与 LVGL 适配层采用 managed component 版本还是项目内 fork。
