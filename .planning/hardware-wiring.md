# 四板免焊接线规划

## 预期提交

1. `fix(hardware): 校正主控排针及板载资源资料`：核验官方引脚图与原理图，修正 ESP 技能和 AGENTS 中的位置定义。
2. `docs(hardware): 完成四板免焊接线方案并更新屏幕技能`：保存完整接线、供电、材料和上电验证方案，将用户提供的驱动板八针定义录入技能。

## 进度

- 已读取 EC、QYE、ESP、8PB 技能及相关硬件参考。
- 已运行 `vp install`，工作区初始干净。
- 官方 ESP 引脚图与原理图已确认 P6 脚号、板载复用；修正资料已提交为 `e4aa530`。
- 用户于 2026-09-10 提供屏幕驱动板从左到右针序：BUSY/RES/D/C/CS/SCK/SDI/GND/3.3V。
- 验证：资料交叉检查、GPIO 唯一性核验、技能校验、`vp check`、`vp test`、提交前暂存区审查。

## 交付与验证

- 完整接线方案：`packages/hardware/README.md`，20 根线、16 个独立 GPIO、两路 USB 供电。
- 屏幕技能补充用户实物定义与 P6 映射，按键技能及项目入口同步引用。
- `vp check`：三个既有文件 `package.json`、`pnpm-workspace.yaml`、`skills-lock.json` 格式检查失败。
- `vp test`：当前配置下无测试文件，退出码 1。
- ESP、屏幕、按键技能校验通过；20 个 P6 端点、16 个 GPIO 的唯一性、映射、保留资源和相对链接检查通过；`vp lint` 与 `git diff --check` 通过。
- 实物上电测试待按方案验收步骤执行。
