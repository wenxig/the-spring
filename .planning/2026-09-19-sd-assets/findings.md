# Findings

- 实装为 Waveshare ESP32-P4-Module-DEV-KIT，SDMMC 使用 GPIO43/44/39/40/41/42，GPIO45 为卡电源控制。
- dependencies.lock 固定 ESP-IDF 6.0.2、LVGL 9.5.0；受管组件 manifest 的 IDF 下界覆盖当前版本。
- app_main 在首次 render 前 mount_sdcard 与 ui_assets_ready。
- 固件使用 S:/fonts 与 S:/icons，主机仿真继续内嵌资源。
- 当前 macOS /Volumes 仅有 Macintosh HD，串口 /dev/cu.usbmodem141101 可见。
- 现有未提交修改含 UI 重构及独立硬件/技能文件变更，需要核对提交依赖。
