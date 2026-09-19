# Findings

- 实拍：明日天气位于 x208 左侧区域，日期列 x277 右侧空白；时间为 1970-01-01。
- 当前 declarative_ui.cpp 调用手工 Frame 绘制；XML 未接入构建。
- 默认字体文件 assets/HYWenHei-65W-3.ttf 为约 3.1 MB。
- storage_service 已使用 cJSON；manifest、lock 和架构文档仍存在 SQLite。
- Frame 中 set bit 表示黑色；保留已经实测工作的 SSD1683 传输约定。
- ESP-IDF 6.0.2 中 `esp_crt_bundle.h` 由 `mbedtls` 在 `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE` 下提供，不能作为独立组件写入 `REQUIRES`。
- ESP-Hosted 3.0.7 依赖解析正常，锁文件现已包含 `esp_hosted`、`esp_wifi_remote`、`wifi_remote_over_eppp` 与 `eppp_link`。
- 生成配置确认 P4 主机使用 SDIO slot 1、4-bit、40 MHz，GPIO CMD/CLK/D0..D3=19/18/14/15/16/17，C6 reset=54；板卡配置保持 `P4_DEV_BOARD_NONE`，这些值来自项目采用的内部 SDIO 方案，需以实机握手日志最终确认。
