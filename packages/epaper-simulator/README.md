# 电子纸主机仿真

仿真器与 ESP-IDF 固件共同编译 `components/ui_core`，输出 SSD1683 所需的 400×300、1-bit framebuffer。PBM 使用 Netpbm `P4` 格式，文件头之后每行 50 字节，黑色像素按 MSB-first 排列。仿真目标只依赖 `ui_core`，不需要 ESP-IDF、GPIO、SPI、LVGL 或实体屏幕。

```sh
cmake -S packages/epaper-simulator -B packages/epaper-simulator/build -G Ninja
cmake --build packages/epaper-simulator/build
ctest --test-dir packages/epaper-simulator/build --output-on-failure
packages/epaper-simulator/build/epaper_simulator /tmp/clock.pbm
```

生成首次接屏校准图案：

```sh
packages/epaper-simulator/build/pattern_test /tmp/pattern_
```

输出 `white.pbm`、`black.pbm`、`checkerboard.pbm` 和 `corners_marked.pbm`。
# 无屏幕板端帧采集

开发板启用 `CONFIG_SPRING_DISPLAY_BUFFER_ONLY=y` 后，P6 可以保持断开。每次 UI
渲染会向 USB 控制台输出一个二进制帧包；包格式为小端序：`magic(u32)`、
`version(u16)`、`frame_id(u32)`、`payload_length(u32)`、`fnv1a(u32)`、
`payload(15000 bytes)`、`magic(u32)`。控制台中的普通日志可以与帧包混合存在。

保存串口输出后生成 PBM：

```sh
python3 capture_frame.py frame.pbm --input serial.log
```

也可以直接监听开发板 USB 串口，收到第一帧后退出并生成图片：

```sh
python3 capture_frame.py frame.pbm --port /dev/cu.usbmodemXXXX --baud 115200
```

工具只接受版本、长度、尾标记和 FNV-1a 全部正确的完整帧。

运行主机仿真、校准图案和无屏配置检查：

```sh
python3 packages/epaper-simulator/verify.py
```
