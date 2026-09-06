# ESP32-P4 ESP-IDF 固件开发与构建指南

本文档指导在 ESP-IDF 下进行 ESP32-P4-WIFI6-DEV-KIT 的工程构建、固件编译与烧录调试。

---

## 1. 软件开发环境需求

- **ESP-IDF 版本**: 推荐 **v5.4** 或 **v5.5+**（ESP32-P4 正式支持版本，严禁使用 v5.3 及更早版本）
- **工具链**: RISC-V 交叉编译器 `riscv32-esp-elf`
- **代码规范**: 符合项目规范，优先使用 C++26 / Clang，推导使用 `auto`，OOP + RAII，依赖注入减少强耦合

---

## 2. 目标设定与工程配置

### 2.1 设定目标芯片
在工程根目录下执行：
```bash
idf.py set-target esp32p4
```

### 2.2 PSRAM 与芯片版本配置 (sdkconfig)
ESP32-P4NRW32 内置 32MB PSRAM，需要在 `sdkconfig` 中开启八线 PSRAM 支持：
- `CONFIG_SPIRAM=y`
- `CONFIG_SPIRAM_MODE_HEX=y` (或 Octal/Hex SPI 模式)
- `CONFIG_SPIRAM_SPEED_200M=y`
- `CONFIG_SPIRAM_USE_CAPS_ALLOC=y`

对于微雪 ESP32-P4-Platform 提供的 rev 3.0 / 3.1 兼容配置：
```bash
# 若有预设配置可合并导入
cat sdkconfig.defaults.esp32p4 >> sdkconfig
```

### 2.3 ESP32-C6 Wi-Fi 协处理器配置 (ESP-Hosted / SDIO)
ESP32-C6 充当无线网卡协处理器：
- 启用 SDIO 通信驱动与网络层适配。
- 支持 802.11ax Wi-Fi 6 目标唤醒时间 (TWT) 节能特性。

---

## 3. 编译、烧录与串口监视

### 3.1 正常编译与烧录
```bash
# 1. 编译
idf.py build

# 2. 烧录固件与开启监视器（以 Linux/macOS 为例）
# 开发板双 Type-C 接口：
# - Type-C UART (CH343P): 通常识别为 /dev/ttyUSB0 或 /dev/tty.usbserial-*
# - Type-C USB (Native): 通常识别为 /dev/ttyACM0 或 /dev/tty.usbmodem*
idf.py -p /dev/tty.usbserial-110 flash monitor
```

### 3.2 手动进入 Bootloader 模式
若芯片处于深度休眠或串口未正常响应自动复位：
1. 按住板载 **BOOT** 按键不放。
2. 短按一次 **RST** 按键。
3. 松开 **BOOT** 按键。
4. 执行烧录指令：`idf.py flash`。

---

## 4. 常见问题排查与避坑

1. **PSRAM 识别不足或死机**：
   - 检查芯片型号是否为 P4NRW32，确保 `sdkconfig` 中开启了正确的 PSRAM 时钟与引脚配置，避免错误配置为 Quad PSRAM。
2. **Wi-Fi 搜不到或通信失败**：
   - 确认 ESP32-C6 固件是否已正确烧录且 SDIO 握手正常。
3. **以太网无法获取 IP**：
   - 检查 PHY 芯片型号与 MDC/MDIO 引脚配置，确保时钟源（50MHz RMII 或 25MHz 晶振）匹配。
