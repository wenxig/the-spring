# the-spring ESP32-P4 核心固件

面向 Waveshare ESP32-P4-Module-DEV-KIT 的 ESP-IDF 子项目。FreeRTOS 负责任务调度，LVGL 9 负责 UI，SSD1683 电子纸由 `esp_epaper` 驱动，EC600MCNLE 通过 UART1/GPIO0-GPIO1 由 `esp_modem` 协调 AT 命令。

## 当前硬件映射

- 电子纸：BUSY/RES/D-C/CS/SCK/SDI = GPIO22/21/6/5/2/3
- EC600X UART1：TX/RX = GPIO0/GPIO1
- 按键 S1-S8：GPIO4/20/23/26/27/46/47/48，active-low 上拉

按键语义、数字时钟和休眠行为见 `docs/ui-input-clock.md`。

## 构建

默认显示后端为 `buffer_only`：开发板可以连接电脑和按键，但 P6 不连接电子纸，固件仍会完成 UI 路由和 framebuffer 渲染，不会等待 BUSY 或访问屏幕总线。接入实体屏幕后，将启动参数切换为 `Backend::epaper`，再启用 SSD1683 传输。

```sh
idf.py set-target esp32p4
idf.py build
```

组件版本和待讨论技术决策见 `docs/decisions.md`。
