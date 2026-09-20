# 声明式 UI 源文件

`ui_xml/` 是页面布局的唯一声明式来源，使用 LVGL 9.5 XML 约定描述 screen、component、layout 和常量。

当前仓库锁定的 `lvgl/lvgl` 9.5.0 开源组件提供 Flex/Grid 和 I1 软件渲染，但不包含 XML 解析器；XML 解析、预览和 C 代码生成由 LVGL Pro Editor/CLI 完成。生产固件应将 XML 生成的 `*_gen.c` 纳入 `ui_core`，固件运行时只链接普通 LVGL API。

## 生成流程

1. 在 LVGL Pro 中打开 `ui_xml/project.xml` 项目并校验 XML。
2. 以 400×300、I1、无动画配置导出 C 代码。
3. 将生成文件放入 `components/ui_core/generated/`，并在 `ui_core/CMakeLists.txt` 中加入源文件。
4. 将 `Snapshot` 映射到 LVGL Subjects；按键事件通过 `UiAction` 交给 `Router`。

命令行生成器可使用 `node lved-cli.js validate ui_xml/project.xml` 和
`node lved-cli.js generate ui_xml/project.xml`；生成结果放入
`components/ui_core/generated/` 后由 CMake 编译。

页面中的 `bind="snapshot.*"` 与 `bind="forecast.*"` 是动态数据绑定声明；生成代码不得手工修改，业务适配集中在 `ui_core/ui_bindings.cpp`（当前兼容实现为 `ui_components.cpp`）。显示层继续负责 I1 位序、SSD1683 反相、脏矩形和整刷阈值。

## 暂存兼容层

在生成器接入前，现有 `Router::render` 仍作为 buffer-only 仿真后端，保证电子纸图案测试和板端帧采集链路可用。迁移页面时逐页替换该后端，直到所有 screen 都由生成的 LVGL UI 驱动。
## 布局预览

默认 `reference` 按 400×300 面板实现参考比例：日期栏起点 x=270，天气横线 y=174，四张天气卡片等宽。`ClockHeader`、`DatePanel`、`ForecastCard` 共享动态绑定。日期的月份与日分别居中，定位文字通过 TinyTTF 字宽测量在 13–10 px 内适配，超长内容使用省略号。

`UI_LAYOUT_VARIANT` 可选 `reference`、`large-time`、`weather-focus`，变量来自 `ui_xml/variants`。预览命令生成三种布局的正常、长文本/负温度、等待数据、高考期间画面，结束后恢复默认布局：

```sh
uv run --with pillow python packages/epaper-simulator/render_previews.py --output .planning/2026-09-20-layout-reference/previews
```

PNG 与 PBM 均为 400×300 黑白双态。预览数据为确定性样例，板端使用服务快照；定位未获取时显示“位置待更新”。
