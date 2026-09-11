# LVGL 9 与电子纸局部刷新

LVGL 只在 `UiTask` 中运行，使用单色 `LV_COLOR_FORMAT_I1` 绘制。显示服务维护 `committed_frame`（已确认写入面板的 400x300 帧）和 `pending_frame`，flush 回调把 LVGL dirty area 转换为字节对齐的脏矩形。

刷新策略：

- 首次启动、唤醒、掉电恢复或状态未知：整帧刷新，并提交完整基线。
- 局部区域：合并相邻 dirty area，按 8 像素水平字节边界扩展，比较新旧帧后只发送变化矩形。
- 局刷失败或 BUSY 超时：标记基线无效，下一次强制整帧；有限重试后上报显示故障。
- 局刷计数、温度、残影阈值由策略配置控制，达到阈值后自动整帧清理。
- 当前实现局刷累计 20 次后自动切换整刷；该数值是工程初始值，需结合面板版本、温度和残影实测调整。

SPI、BUSY、复位和高压时序全部由 `EpaperService` 串行拥有。LVGL flush 在刷新完成前保持 pending 状态，完成后调用 `lv_display_flush_ready`。这样 LVGL 的局部刷新语义与 SSD1683 的实际帧状态保持一致。
