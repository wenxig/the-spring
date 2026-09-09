# ESP32-P4 ESP-IDF 固件开发与构建指南

本文档指导在 ESP-IDF 下进行 ESP32-P4-Module-DEV-KIT 的工程构建、固件编译与烧录调试。

---

## 1. 软件开发环境需求

- **ESP-IDF 版本**: ESP32-P4 Platform 主要面向 **release/v5.4** 及后续版本维护，官方建议使用 **v5.5.4**
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

ESP32-P4 芯片版本需要使用与实物匹配的默认配置。对于微雪 `ESP32-P4-Platform` 仓库，配置文件位于 `config/` 目录：
- rev v3.1 及之后：`esp32p4_rev_v3_1.defaults`
- rev v3.0 及之后：`esp32p4_rev_v3_0.defaults`
- rev v0.x / v1.x 工程样品：`esp32p4_rev_pre_v3.defaults`

构建时将对应文件追加到 `SDKCONFIG_DEFAULTS`，例如：
```bash
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;../../../config/esp32p4_rev_v3_1.defaults" set-target esp32p4 build
```
切换芯片版本配置后，建议删除示例目录中的 `sdkconfig`，避免已有配置覆盖默认值。

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
   - 检查芯片型号是否为 P4NRW32，确保 `sdkconfig` 中开启了正确的 PSRAM 时钟与引脚配置。
2. **芯片版本配置不匹配**：
    - 若日志提示芯片版本范围与实际版本不符，按实物版本重新选择 `esp32p4_rev_*.defaults` 后重新构建，不要使用 `--force` 跳过检查。
3. **Wi-Fi 搜不到或通信失败**：
   - 确认 ESP32-C6 固件是否已正确烧录且 SDIO 握手正常。
4. **以太网无法获取 IP**：
   - 检查 PHY 芯片型号与 MDC/MDIO 引脚配置，确保时钟源（50MHz RMII 或 25MHz 晶振）匹配。
