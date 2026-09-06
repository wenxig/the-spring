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
└── src/
    ├── _main.py          # 系统自启入口（看门狗初始化、崩溃日志兜底守护）
    ├── main.py           # 业务主逻辑（网络检查、短信/通话服务启动与主监控循环）
    └── services/         # 蜂窝通信与电信核心服务模块
        ├── __init__.py
        ├── network.py    # 网络注册与 CSQ 监控服务
        ├── sms.py        # 短信服务
        └── volte.py      # VoLTE 通话服务
```

## 烧录与部署流程

1. 使用 Type-C 数据线将 EC600X-EVB 接入电脑，电源拨动开关切换至 **USB** 挡。
2. 确保模组已烧录移远官方支持 **VoLTE** 的 QuecPython 固件版本。
3. 打开移远 **QPYcom** 工具，选择模组对应的 USB CDC 串口。
4. 将 `src/` 目录下的所有文件上传至模组的 `/usr` 存储根目录。
5. 重启模组，系统将自动执行 `/usr/_main.py` 并启动网络、SMS 和 VoLTE 监听。
