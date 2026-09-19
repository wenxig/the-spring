# 实拍问题修复

## Goal
XML 组件驱动 400×300 黑白 UI；默认 HYWenHei TTF 解析缩放；Ionicons 5 原始天气矢量；正确日期与高考区间；cJSON 存储依赖一致。

## Phases / 预期提交
1. 清理存储依赖与文档，重新解析 IDF 依赖。Status: complete
2. 实现共享 XML 组件渲染、运行时 TTF 字体、Ionicons 图标与天气列布局。Status: complete
3. 修复日期有效性、6月7日至9日状态及天气数据语义，增加边界验证。Status: complete
4. 仿真视觉验证、静态检查、固件构建、烧录和日志核验。Status: complete
5. 板载 ESP32-C6 Wi-Fi 优先链路、蜂窝回退与真实板验证。Status: complete (C6 firmware metadata warning tracked)

## Next Step
已完成 `/dev/cu.usbmodem141101` 烧录与启动日志核验；后续维护项为更新 C6 协处理器固件元数据并复测兼容性告警。

## Errors
初次技能合并读取输出截断；已拆分重读。
仿真配置缺少 LV_USE_MATRIX，已补齐；ThorVG c++26 编译需要显式包含 cstdlib。
首次 IDF 重配置把 `esp_crt_bundle` 当作独立组件；已改成 `mbedtls` 并启用证书包。
clang-tidy 无法解析 ESP-IDF GCC 专用架构参数（`esp-base`、`xesploop` 等）；保留失败证据，IDF GCC 构建已通过。
