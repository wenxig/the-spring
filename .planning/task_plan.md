# EC600X 后现代工程化重构

## Goal

为 `packages/ec600x-firmware` 建立现代 Python 工程化边界，同时保持 QuecPython 运行时兼容，并提供可重复的构建、检查、打包和部署准备流程。

## Phases

### Phase 1: 现状与边界
**Status:** complete

- 已确认固件运行时是 QuecPython，不可直接使用 CPython-only 的 uv/dishka/pydantic。
- 已确认根 `package.json` 被用户清空脚本，本任务不恢复或覆盖。

### Phase 2: 固件架构
**Status:** complete

- 引入显式设备依赖边界和应用生命周期。
- 保持 `/usr/_main.py`、`main.run()` 兼容。
- 使用平台协议和依赖注入支持宿主机替身测试。

### Phase 3: 工程化命令
**Status:** complete

- 在固件包内增加构建、静态检查、产物打包和部署准备命令。
- 生成源码校验清单，避免上传不完整。

### Phase 4: 验证
**Status:** complete

- 运行编译检查和可用的 lint/test。
- 检查 git diff，确认不触及用户已有删除和未跟踪 skill。

### Phase 5: 严格质量门禁
**Status:** complete

- 使用项目内 QuecPython API 类型桩启用 Pyright strict，覆盖 `src`、`tools` 和 `tests`。
- 使用锁定的 `uv` 开发依赖统一执行格式化、lint、类型检查和测试。
- 保留设备运行时边界：类型检查依赖只存在于宿主机，固件不上传 `typings/`、`uv` 或第三方包。
- 完成完整门禁、编译检查、打包和 SHA-256 产物校验。

## Next Step

向用户说明严格检查配置、产物位置、验证结果，以及需要 QPYcom/实际模组完成的物理部署步骤。

## Errors Encountered

| Error | Attempt | Resolution |
|---|---:|---|
| `scripts/firmware.mjs` 不存在 | 1 | 改为在固件包内新增独立工具，不修改用户清空的根脚本 |
| 打包器复制 `src/__pycache__` 目录 | 1 | 过滤非文件路径后重新构建 |
| 严格 Pyright 首轮报告 248 个错误 | 1 | 固化 QuecPython API 类型桩并为固件、工具和测试补齐类型契约 |
| 严格 Pyright 第二轮报告 89 个错误 | 2 | 修复平台协议、回调参数收窄、串口/打包工具类型和可选成员访问 |
