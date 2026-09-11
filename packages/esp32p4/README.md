# the-spring ESP32-P4 核心固件

面向 Waveshare ESP32-P4-Module-DEV-KIT 的 ESP-IDF 子项目。FreeRTOS 负责任务调度，LVGL 9 负责 UI，SSD1683 电子纸由 `esp_epaper` 驱动，EC600MCNLE 通过 UART1/GPIO0-GPIO1 由 `esp_modem` 协调 AT 命令。

## 当前硬件映射

- 电子纸：BUSY/RES/D-C/CS/SCK/SDI = GPIO22/21/6/5/2/3
- EC600X UART1：TX/RX = GPIO0/GPIO1
- 按键 S1-S8：GPIO4/20/23/26/27/46/47/48，active-low 上拉

按键语义、数字时钟和休眠行为见 `docs/ui-input-clock.md`。

## 构建

默认显示后端为 `buffer_only`：开发板可以连接电脑和按键，但 P6 不连接电子纸，固件仍会完成 UI 路由和 framebuffer 渲染，不会等待 BUSY 或访问屏幕总线。接入实体屏幕后，将启动参数切换为 `Backend::epaper`，再启用 SSD1683 传输。

无屏幕验收时观察 USB 串口日志中的 `ui route=... dirty=... frame=...`：路由名确认页面切换，`dirty` 确认首帧全屏和后续局部区域，`frame` 是 framebuffer 的 FNV-1a 校验值，可用于重复运行比对。buffer-only 配置还会输出二进制帧包，使用 `packages/epaper-simulator/capture_frame.py` 可从 USB 捕获文件生成 PBM；实体屏模式不会输出该帧包。

```sh
idf.py set-target esp32p4
idf.py build
```

## 无屏幕验收

P6 暂未接屏时保持 `CONFIG_SPRING_DISPLAY_BUFFER_ONLY=y`，连接开发板 USB 后烧录
固件。主机端执行：

```sh
python3 packages/epaper-simulator/verify.py
python3 packages/epaper-simulator/capture_frame.py /tmp/spring.pbm --port /dev/cu.usbmodemXXXX --baud 115200
```

第二条命令收到第一帧后退出。生成的 PBM 可用 ImageMagick、预览工具或其他
Netpbm 工具查看；帧包校验失败时工具会返回错误。

串口启动日志会明确打印 `buffer-only` 或 `epaper`。接屏前使用：

```sh
idf.py -C packages/esp32p4 menuconfig
```

在 `the-spring display` 中确认 SPI 频率、BUSY 极性和局刷阈值，再构建烧录。

接入屏幕后，将配置切换为 `CONFIG_SPRING_DISPLAY_BUFFER_ONLY=n` 并重新构建。首次
上电先使用 `pattern_test` 生成的全白、全黑、棋盘格和四角图案，记录 BUSY 极性、
画面方向和刷新波形，再启用业务页面。

组件版本和待讨论技术决策见 `docs/decisions.md`。
