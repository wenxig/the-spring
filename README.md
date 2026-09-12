# The Spring - 四月天


- [四板免焊接线方案](packages/hardware/README.md)

## 常用 Vite Task

使用 `vp run <task>` 执行根目录任务。主机仿真和固件任务共享 `ui_core` 与汉仪文黑字库。

原生仿真使用 GCC C++26 规则，依赖由系统工具链和 ESP-IDF Component Manager 管理；项目不再引入 vcpkg。

```sh
vp run font:generate
vp run simulator:test
vp run simulator:render
vp run simulator:render:fallback
vp run simulator:patterns
vp run embedded:verify
vp run embedded:verify-buffer-only
vp run firmware:build
vp run firmware:flash
vp run firmware:monitor
vp run board:capture
```

硬件任务默认使用 `/dev/cu.usbmodem141101`。端口变化时通过 `ESP32_PORT` 覆盖，例如：

```sh
ESP32_PORT=/dev/cu.usbmodem142101 vp run firmware:flash
ESP32_PORT=/dev/cu.usbmodem142101 vp run firmware:monitor
```

仿真产物写入 `.artifacts/epaper/`，开发板帧采集写入 `.artifacts/board/`。
