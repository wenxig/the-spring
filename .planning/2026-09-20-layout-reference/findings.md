# Findings

- main 为 57715bc，原 XML 分支为 codex/ui-lvgl95-gcc-cpp26 (87d962b)。
- 主目录日历改动位于 ui_core.cpp、ui_core.hpp、simulator main.cpp；完整保留。
- 当前工作位于独立工作树 clock-layout-reference，基于原 XML 分支；未修改 main 分支历史。
- 参考图实际为 1448×1086，等比缩到 400×300：日期分割 x≈270，天气横线 y≈174。
- 参考图天气四列等宽，日期竖线到天气横线结束；天气独立分隔线从 y≈194 开始。
- 保留 SD 字体与 SVG 加载，预览共用真实 LVGL 渲染器，数据为确定性样例。
- 所有参考画面用纯黑白像素，放大展示使用最近邻。
