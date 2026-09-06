# 开发板 Skills 规划与设计规范

本文档遵循 `AGENTS.md` 规范，规划为项目中使用的两块核心物理开发板编写专用 Agent Skills 并存入项目目录 `.agents/skills/`：

1. **微雪 ESP32-P4-WIFI6-DEV-KIT** (`waveshare-esp32-p4-dev-kit`)
   - 对应硬件：微雪 ESP32-P4-WIFI6-DEV-KIT 开发板
   - 官方文档：https://docs.waveshare.net/ESP32-P4-WIFI6-DEV-KIT
   - 角色定位：主控板，统领传感器、EC600X、SPI 水墨屏、多媒体（音频 ES8311、MIPI-DSI/CSI、USB OTG、百兆以太网、SDIO Wi-Fi 6 C6 协处理器）
   - 软件框架：ESP-IDF v5.4/v5.5+、ESP32-P4 Platform、C++26 / Clang、FreeRTOS

2. **移远 EC600X-EVB (EC600M)** (`quectel-ec600x-evb`)
   - 对应硬件：移远 EC600X-EVB 开发板（内置 EC600M 模组）
   - 官方文档：https://developer.quectel.com/doc/quecpython/Dev_board_guide/zh/ec600x-evb.html
   - 角色定位：移动通信子系统（LTE Cat 1 bis、VoLTE、4G 数据网络传输、板载外设 AHT20/光敏/MIC/喇叭）
   - 软件框架：AT 指令 / PPP 拨号 / QuecPython 双栈开发支持，与 ESP32-P4 通过 UART/USB 接口协同

## 目录结构设计

```
.agents/skills/
├── waveshare-esp32-p4-dev-kit/
│   ├── SKILL.md
│   └── references/
│       ├── hardware-specs-and-pinout.md
│       ├── peripherals-and-interfaces.md
│       └── esp-idf-development.md
└── quectel-ec600x-evb/
    ├── SKILL.md
    └── references/
        ├── evb-hardware-and-pinout.md
        ├── at-communication-guide.md
        └── quecpython-development.md
```

## 交付与审查
- 完成 Skill 定义与参考文档。
- 确认符合 frontmatter 规范（name、description）。
- 运行 `vp check` 验证格式与配置完整性。
