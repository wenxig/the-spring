# Progress

- 完成实拍与当前代码核对；保留用户正在进行的技能、硬件文档及测试包删除改动。
- 网络阶段首次 `idf.py reconfigure` 已完成依赖解析，生成 ESP-Hosted 3.0.7、ESP Wi-Fi Remote 1.6.4 及其 EPPP 依赖；初次失败原因为错误声明 `esp_crt_bundle` 组件，已改为 `mbedtls` 并启用证书包默认配置。
- 已把残留的 `managed_components/nopnop2002__sqlite3` 移出工程至 `/tmp/spring-sqlite.tmOIqR`，保持可恢复。
- ESP32-P4 固件已在 ESP-IDF 6.0.2 下成功构建，产物 `build/the_spring_esp32p4.bin`，应用占用 0x4a17f0 / 0x500000（剩余 7%）。
- 仿真器新构建目录 `packages/epaper-simulator/build-net` 的 7 个 CTest 全部通过；clang-format dry-run 通过。clang-tidy 因 GCC RISC-V 专用参数无法由 Apple/LLVM Clang 解析而失败。
- 真实板 `/dev/cu.usbmodem141101` 已烧录并启动：ESP32-P4 v1.3、16 MB Flash、32 MB PSRAM，应用提交 `4d395bb-dirty`。
- ESP32-C6 通过 SDIO 4-bit/40 MHz 握手成功，Wi-Fi 连接 `dlsflfl` 获得 `192.168.50.127`，Wi-Fi NTP 校时成功；应用随后完成电子纸首帧与动态帧渲染日志。
- 板上未检测到可用 SD host，cJSON 网络路径不受影响；存储挂载错误已记录为无 SD 卡/主控资源占用的硬件状态。
- 针对实拍反馈的局刷花屏，新增 `CONFIG_SPRING_DISPLAY_ENABLE_PARTIAL_REFRESH`，默认关闭；关闭时即使上层提交 dirty area，SSD1683 也走已验证的整帧更新，保持面板两块 RAM 与波形状态一致。
- 使用最新固件重新烧录 `/dev/cu.usbmodem141101` 并运行串口监控：启动和动态更新均为 `mode=full`、`partial_count=0`，未出现 `BUSY timeout` 或 `refresh failed`；C6 SDIO 握手、Wi-Fi `dlsflfl`、NTP 校时均正常。
