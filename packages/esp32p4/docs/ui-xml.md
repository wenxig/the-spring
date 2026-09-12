# 声明式 UI 源文件

`ui_xml/` 是页面布局的唯一声明式来源，使用 LVGL 9.5 XML 约定描述 screen、component、layout 和常量。

当前仓库锁定的 `lvgl/lvgl` 9.5.0 开源组件提供 Flex/Grid 和 I1 软件渲染，但不包含 XML 解析器；XML 解析、预览和 C 代码生成由 LVGL Pro Editor/CLI 完成。生产固件应将 XML 生成的 `*_gen.c` 纳入 `ui_core`，固件运行时只链接普通 LVGL API。

## 生成流程

1. 在 LVGL Pro 中打开 `ui_xml/` 项目并校验 XML。
2. 以 400×300、I1、无动画配置导出 C 代码。
3. 将生成文件放入 `components/ui_core/generated/`，并在 `ui_core/CMakeLists.txt` 中加入源文件。
4. 将 `Snapshot` 映射到 LVGL Subjects；按键事件通过 `UiAction` 交给 `Router`。

生成代码不得手工修改；业务适配放在 `ui_core/ui_bindings.cpp`。显示层继续负责 I1 位序、SSD1683 反相、脏矩形和整刷阈值。

## 暂存兼容层

在生成器接入前，现有 `Router::render` 仍作为 buffer-only 仿真后端，保证电子纸图案测试和板端帧采集链路可用。迁移页面时逐页替换该后端，直到所有 screen 都由生成的 LVGL UI 驱动。
