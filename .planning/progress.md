# Progress

## 2026-09-06

- 检查仓库和 `packages/ec600x-firmware`。
- 识别到用户已有 docs 删除和 `.agents/skills/qyeg0420rys683` 未跟踪变更，均不触碰。
- 确认缺少 `scripts/firmware.mjs`，部署工具需要重新建立在固件包内。
- 宿主机编译通过，注入平台服务测试 3/3 通过。
- 首次打包因未过滤 `__pycache__` 目录失败，已修复过滤逻辑。
- Ruff 检查通过，pytest 3/3 通过，compileall 通过。
- 已生成并验证 `packages/ec600x-firmware/dist/ec600x-firmware.zip`，SHA-256 为 `74561674a3ff8b90873faae54d9b522771304c39cc3816c6dba26c1e8eb25db9`。
- 排查 QPYcom 跨平台情况：移远官方 QPYcom 仅支持 Windows 与 Ubuntu，无 macOS 版本；官方 VSCode 插件亦仅限 Windows。
- 通过 `uv tool install thonny --with thonny-quecpython` 为 macOS 安装了移远官方推荐的跨平台 QuecPython 图形工具（位于 `~/.local/bin/thonny`）。
- 针对命令行无头自动化需求，实现并集成了基于 MicroPython/QuecPython Raw REPL 协议的轻量 CLI 刷机工具 `packages/ec600x-firmware/tools/flash.py`。
- 在 `packages/ec600x-firmware/package.json` 中配置了 `pnpm run ports` 与 `pnpm run flash` 脚本，可无缝结合 `pnpm run build` 实现终端一键自动化部署。
- 更新 `packages/ec600x-firmware/README.md`，添加 CLI 自动部署命令与示例。
