# ec600x-firmware

移远 **EC600X-EVB** (搭载 **EC600M-CN** LTE Cat 1 bis 模组) QuecPython 4G 注册与基站定位测试固件。

## 功能核心

- **蜂窝网络连接管理 (`services/network.py`)**：基站注网就绪检测、数据通道激活与信号强度 (CSQ) 监控。
- **基站定位服务 (`services/location.py`)**：通过 cellLocator API 获取当前经纬度坐标，无需 GPS 模块。
- **VoLTE 语音通话 (`services/volte.py`)**：来电异步监听、呼出拨号、接听、挂断、音频通道与音量控制。
- **短信业务系统 (`services/sms.py`)**：中英文/UCS2 编码短信发送、来信异步事件回调与自动存储防满清理。
- **系统看门狗与异常防护 (`_main.py`)**：开机自动启动、WDT 定时喂狗与未捕获异常持久化记录 (`/usr/crash.log`)。

## 目录结构

```text
ec600x-firmware/
├── package.json          # Monorepo 子包标识
├── pyproject.toml        # Python、Ruff、Pyright 与 pytest 配置
├── README.md             # 部署说明与接口文档
├── tools/pack.py         # 宿主机产物构建与校验工具
├── typings/              # QuecPython 与宿主机 API 类型桩
├── tests/                # 使用注入平台替身的宿主机测试
└── src/
    ├── _main.py          # 系统自启入口（看门狗初始化、崩溃日志兜底守护）
    ├── main.py           # 4G 注册与定位测试主程序
    ├── application.py    # 应用生命周期与依赖组合根
    ├── qpy_platform.py   # QuecPython 平台模块边界
    ├── type_contracts.py # 类型协议与回调契约
    └── services/         # 蜂窝通信与电信核心服务模块
        ├── __init__.py
        ├── network.py    # 网络注册与 CSQ 监控服务
        ├── location.py   # 基站定位服务
        ├── sms.py        # 短信服务
        └── volte.py      # VoLTE 通话服务
```

## 宿主机检查与构建

这些命令只在开发机执行。`uv` 管理锁定的开发工具依赖，固件运行时不携带第三方依赖。

```bash
cd packages/ec600x-firmware
uv sync

# 完整质量门禁：格式、lint、严格类型检查、单元测试
pnpm run check

# 分项执行
pnpm run format
pnpm run lint
pnpm run typecheck
pnpm run test

# QuecPython 运行时语法兼容性
python3 -m compileall -q src

# 构建并校验部署产物
pnpm run build
pnpm run verify
```

Pyright 使用 `typeCheckingMode = "strict"`，并将 `src`、`tools`、`tests` 纳入检查范围；Ruff 使用包含错误、导入、命名、升级建议、bug 检测和简化规则的门禁。`typings/` 固化项目实际使用的 QuecPython API 契约，因此检查结果不依赖开发机上的 Thonny 或其他用户目录安装。

若只需要直接调用底层工具，也可以使用：

```bash
uv run pyright
uv run ruff check src tools tests
uv run ruff format --check src tools tests
uv run pytest
python3 tools/pack.py build
python3 tools/pack.py verify
```

构建后把 `dist/ec600x-firmware.zip` 解压得到的文件上传到模组 `/usr`。`dist/SHA256SUMS` 用于在上传前确认归档未损坏。

## 测试程序说明

当前 `main.py` 是 **4G 注册与基站定位测试程序**，烧录后会自动执行：

1. 等待 4G 网络注册（最多 60 秒）
2. 查询信号强度 (CSQ) 与基站信息
3. 调用 cellLocator 获取当前经纬度坐标
4. 每 60 秒报告一次信号与内存状态

测试输出示例：
```
========================================
  EC600X 4G Registration & Location Test
========================================
Free memory: 5242880 bytes

[Test 1/2] Testing 4G network registration...
[Net] Waiting for cellular network...
[Net] Cellular network ready.
[OK] Signal CSQ: 23
[OK] Cell info: {...}

[Test 2/2] Testing cell tower location...
[Location] Position: lat=39.908823, lng=116.397470, accuracy=550m
[OK] Position: lat=39.908823, lng=116.397470, accuracy=550m

========================================
All tests completed. Module will stay alive for monitoring.
Press Ctrl+C to exit.
========================================
```

## 烧录与部署流程

### 方案 1：命令行全自动部署 (推荐，全平台通用)

固件包内置了基于 MicroPython Raw REPL 通信协议的原生部署工具 `tools/flash.py`，无需打开任何 GUI 窗口，一条命令即可自动同步 `dist` 代码并重启模组：

```bash
cd packages/ec600x-firmware

# 1. 查看当前可用串口
pnpm run ports

# 2. 一键自动构建并烧录上传至模组 /usr (会自动探测 EC600X/Quectel 端口，也可指定 -p /dev/cu.xxx)
pnpm run build
pnpm run flash
# 或指定端口:
# pnpm run flash -- -p /dev/cu.usbmodem1101
```

### 方案 2：Thonny IDE (移远官方跨平台 GUI 方案)

1. 终端执行 `thonny` 打开 IDE。
2. 进入 **运行 (Run) -> 配置解释器 (Configure interpreter...)**，选择 **QuecPython device** 并选定模组串口。
3. 在左侧文件面板导航至 `packages/ec600x-firmware/dist/ec600x-firmware/`，全选右键点击 **上传至 /usr** 即可。

### 方案 3：QPYcom (Windows / Ubuntu)

若在 Windows 或 Ubuntu 环境下，可使用移远官方 **QPYcom** 图形工具连接串口后上传 `dist` 目录内的代码。
