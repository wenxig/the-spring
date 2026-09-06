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
- 启用 Pyright strict，并在 `typings/` 固化项目使用的 QuecPython 与宿主机 API 类型契约。
- 将 Ruff 扩展为导入、命名、升级、bug 检测和简化规则；以 `uv.lock` 锁定开发工具依赖。
- 为固件、部署工具和测试补齐严格类型标注；类型契约仅在类型检查分支导入，保持 QuecPython 运行时兼容。
- `pnpm run check` 通过：格式检查、Ruff lint、Pyright（0 errors/0 warnings/0 informations）和 pytest（3 passed）。
- `python3 -m compileall -q src` 通过；构建并校验部署归档通过，最新 ZIP SHA-256 为 `eb904fe8b53e776b00cb9e0844259b80ed0f1be06f8b956a536ab52c2e5370d7`。
