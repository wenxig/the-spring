# Findings

- 实拍：明日天气位于 x208 左侧区域，日期列 x277 右侧空白；时间为 1970-01-01。
- 当前 declarative_ui.cpp 调用手工 Frame 绘制；XML 未接入构建。
- 默认字体文件 assets/HYWenHei-65W-3.ttf 为约 3.1 MB。
- storage_service 已使用 cJSON；manifest、lock 和架构文档仍存在 SQLite。
- Frame 中 set bit 表示黑色；保留已经实测工作的 SSD1683 传输约定。
- ESP-IDF 6.0.2 中 `esp_crt_bundle.h` 由 `mbedtls` 在 `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE` 下提供，不能作为独立组件写入 `REQUIRES`。
- ESP-Hosted 3.0.7 依赖解析正常，锁文件现已包含 `esp_hosted`、`esp_wifi_remote`、`wifi_remote_over_eppp` 与 `eppp_link`。
- 生成配置确认 P4 主机使用 SDIO slot 1、4-bit、40 MHz，GPIO CMD/CLK/D0..D3=19/18/14/15/16/17，C6 reset=54；板卡配置保持 `P4_DEV_BOARD_NONE`，这些值来自项目采用的内部 SDIO 方案，需以实机握手日志最终确认。
- 实机日志确认上述 SDIO 引脚与总线参数可用：`Card init success`、`slave chip id: 0x0d (esp32c6)`，Wi-Fi RPC 正常工作。
- C6 上报 `coprocessor=0.0.0`、`major version mismatch`、缺少 `SDIO_MODE TLV`，随后进入 compatible streaming mode；当前 Wi-Fi 已成功联网，但应将 C6 协处理器固件版本作为后续维护项核对。
- 真实板启动后电子纸路径未出现 BUSY 超时、SPI 初始化失败或刷新失败日志；应用输出首帧 checksum `12695988`，随后动态局部帧 `8f2b8917`。
- 实拍局刷出现花屏，根因风险集中在当前代码使用未经 QYEG0420BNS830 实屏验证的 `0x26` 旧帧 RAM、局部窗口和 `0xFF` 更新模式；本型号专属 LUT/波形资料尚未取得。
- 已保留工作正常的屏幕极性约定：帧数据发送前执行 `~source[...]`。本次修复仅切换刷新策略，不改变极性、坐标或 XML/UI 内容。
- 复烧日志显示 `present area=... mode=full partial_count=0 limit=20`，证明 dirty area 仍由 UI 产生，但物理电子纸更新统一走整刷；后续重新开启局刷必须先取得匹配 LUT 并进行实屏残影、花屏和断电恢复验证。
