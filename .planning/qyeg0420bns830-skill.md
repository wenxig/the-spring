# QYEG0420BNS830 skill 计划

## 提交划分

1. `docs(plan): 记录水墨屏技能实施计划`
   - 记录资料来源、证据边界、交付文件与验证方式。
2. `feat(skill): 新增 QYEG0420BNS830 开发技能`
   - 在 `.agents/skills/qyeg0420bns830/` 新增技能入口、硬件参考、驱动流程与来源说明。
3. `docs(hardware): 明确 QYEG0420BNS830 水墨屏约束`
   - 更新根目录 `AGENTS.md`，明确项目采用的准确型号、已确认参数、控制边界与接线前置条件。

## 资料与证据边界

- 一级来源：奇耘商品页及其列出的原厂 PDF、Raspberry Pi 示例、STM32 示例。
- 二级来源：Solomon Systech SSD1683 Rev 1.0 数据手册。
- 三级来源：Waveshare 4.2 英寸 SSD1683 同类模块文档和公开驱动，仅用于交叉验证控制器命令、刷新状态机和电子纸维护经验。
- 同类模块的转接板供电、排针编号、BUSY 极性与 LUT 不视为 QYEG0420BNS830 的既定参数。
- 原厂 PDF 或实物标签无法确认的连接器定义保留为待核对项，不推断 ESP32-P4 GPIO 分配。

## 技能结构

- `SKILL.md`：适用场景、资料优先级、工作流、强制约束与参考路由。
- `references/hardware.md`：屏幕参数、控制器能力、信号语义、帧缓冲和电气边界。
- `references/driver-flow.md`：初始化、全刷、局刷、休眠、超时与测试策略。
- `references/sources.md`：来源 URL、证据用途、访问状态和交叉验证说明。

## 验证

- 使用 skill-creator 的 `quick_validate.py` 验证技能结构。
- 运行 `vp check` 与 `vp test`。
- 提交前检查暂存区，仅纳入当前提交对应文件。
