# EC600X 后现代工程化重构

## Goal

为 `packages/ec600x-firmware` 建立现代 Python 工程化边界，同时保持 QuecPython 运行时兼容，并提供可重复的构建、检查、打包和部署准备流程。

## Phases

### Phase 1: 现状与边界
**Status:** complete

- 已确认固件运行时是 QuecPython，不可直接使用 CPython-only 的 uv/dishka/pydantic。
- 已确认根 `package.json` 被用户清空脚本，本任务不恢复或覆盖。

### Phase 2: 固件架构
**Status:** in_progress

- 引入显式设备依赖边界和应用生命周期。
- 保持 `/usr/_main.py`、`main.run()` 兼容。

### Phase 3: 工程化命令
**Status:** complete

- 在固件包内增加构建、静态检查、产物打包和部署准备命令。
- 生成源码校验清单，避免上传不完整。

### Phase 4: 验证
**Status:** complete

- 运行编译检查和可用的 lint/test。
- 检查 git diff，确认不触及用户已有删除和未跟踪 skill。

## Next Step

向用户说明产物位置、验证结果，以及需要 QPYcom/实际模组完成的物理部署步骤。

## Errors Encountered

| Error | Attempt | Resolution |
|---|---:|---|
| `scripts/firmware.mjs` 不存在 | 1 | 改为在固件包内新增独立工具，不修改用户清空的根脚本 |
| 打包器复制 `src/__pycache__` 目录 | 1 | 过滤非文件路径后重新构建 |
