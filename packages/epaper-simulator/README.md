# 电子纸主机仿真

仿真器与 ESP-IDF 固件共同编译 `components/ui_core`，输出 SSD1683 所需的 400×300、1-bit framebuffer。PBM 使用 Netpbm `P4` 格式，文件头之后每行 50 字节，黑色像素按 MSB-first 排列。

```sh
cmake -S packages/epaper-simulator -B packages/epaper-simulator/build -G Ninja
cmake --build packages/epaper-simulator/build
ctest --test-dir packages/epaper-simulator/build --output-on-failure
packages/epaper-simulator/build/epaper_simulator /tmp/clock.pbm
```
