# Findings

- `packages/ec600x-firmware/src` 当前直接导入 `checkNet`、`net`、`sms`、`voiceCall`、`machine`，这些模块只存在于 QuecPython。
- `main.py` 和 `_main.py` 都创建 WDT，职责重复；应用层应接收外部 watchdog 或由启动层统一管理。
- 当前服务已经是独立类，但构造函数无法注入平台模块，导致宿主机无法进行有效的单元测试。
- QuecPython 兼容性优先于 CPython 生态依赖：不把 `uv`、`pydantic`、`dishka` 等运行时依赖上传到模组。
- 根 `package.json` 的 scripts 被用户改为空对象；只添加子包脚本，避免覆盖用户决定。
- 移远官方 QPYcom 工具仅支持 Windows 与 Ubuntu (18/22/24)，无 macOS 原生构建；官方 VSCode 插件同样标注仅支持 Windows。
- 移远官方在 macOS 下推荐且开源维护的跨平台 GUI 方案是 `thonny-quecpython` (通过 pip/uv 安装 Thonny 插件)。
- 移远模组在 macOS 下连接后暴露标准串口及 MicroPython REPL，可编写基于 pyserial 的 CLI 传输脚本或通过 Thonny 界面操作。
