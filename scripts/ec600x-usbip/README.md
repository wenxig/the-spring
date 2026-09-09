# EC600M USB/IP 持久在线架构

macOS 不提供 EC600M 的 Quectel 串口驱动。当前 Rust USB/IP 主机可以枚举设备并让 Linux `option` 驱动创建 ttyUSB，但 EC600M 的复合 USB 控制/批量 URB 在转发后仍不稳定，不能作为烧录链路。

稳定方案按优先级：

1. Ubuntu/Windows 物理主机直连开发板，使用 QuecOpen/QFlash 等官方工具。
2. macOS 使用带 USB passthrough 的 Ubuntu 虚拟机（UTM/QEMU），在虚拟机内运行标准 Linux `usbipd`/QPYcom；Docker Desktop 不提供该 USB passthrough。
3. 当前 Rust USB/IP 仅用于诊断，不用于写入固件。

烧录完成后设备独立运行，macOS 网络服务保持停用，容器或虚拟机可以关闭。
