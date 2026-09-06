# ec600x-firmware

移远 **EC600X-EVB** (搭载 **EC600M-CN** LTE Cat 1 bis 模组) QuecPython 蜂窝网络、VoLTE 语音与短信固件工程。

## 功能核心

- **蜂窝网络连接管理 (`services/network.py`)**：基站注网就绪检测、数据通道激活与信号强度 (CSQ) 监控。
- **VoLTE 语音通话 (`services/volte.py`)**：来电异步监听、呼出拨号、接听、挂断、音频通道与音量控制。
- **短信业务系统 (`services/sms.py`)**：中英文/UCS2 编码短信发送、来信异步事件回调与自动存储防满清理。
- **系统看门狗与异常防护 (`_main.py`)**：开机自动启动、WDT 定时喂狗与未捕获异常持久化记录 (`/usr/crash.log`)。

## 目录结构

```text
ec600x-firmware/
├── package.json          # Monorepo 子包标识
├── pyproject.toml        # Python 配置
├── README.md             # 部署说明与接口文档
├── tools/pack.py         # 宿主机产物构建与校验工具
├── tests/                # 使用注入平台替身的宿主机测试
└── src/
    ├── _main.py          # 系统自启入口（看门狗初始化、崩溃日志兜底守护）
    ├── main.py           # QuecPython 兼容入口
    ├── application.py    # 应用生命周期与依赖组合根
    ├── qpy_platform.py   # QuecPython 平台模块边界
    └── services/         # 蜂窝通信与电信核心服务模块
        ├── __init__.py
        ├── network.py    # 网络注册与 CSQ 监控服务
        ├── sms.py        # 短信服务
        └── volte.py      # VoLTE 通话服务
```

## 宿主机检查与构建

这些命令只在开发机执行，`uv` 可选；固件运行时不携带第三方依赖。

```bash
cd packages/ec600x-firmware
python3 -m compileall -q src
python3 -m pytest
python3 tools/pack.py build
python3 tools/pack.py verify
```

构建后把 `dist/ec600x-firmware.zip` 解压得到的文件上传到模组 `/usr`。`dist/SHA256SUMS` 用于在上传前确认归档未损坏。

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
