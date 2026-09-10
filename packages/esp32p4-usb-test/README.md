# ESP32-P4 USB Host ↔ EC600X 测试

通过 USB CDC ACM Host 模式测试 ESP32-P4 与 EC600X 的通信。

## 硬件连接

```
ESP32-P4 开发板                EC600X-EVB
─────────────────              ────────────────
Type-A USB Host 口  <---USB线---> Micro USB 口
(宽的长方形母口)                  
```

## 注意事项

1. **供电**: 两块板独立供电（不要从 EC600X 给 ESP32 供电）
2. **EC600X 开机**: 长按 PWRKEY 1-2 秒，LED 亮起
3. **USB 数据线**: 必须是数据线，不是仅充电线

## VID/PID 说明

EC600X 的 USB VID/PID 可能因固件版本不同而变化：
- 默认配置: VID=0x2C7C, PID=0x6002
- 如果无法识别，在 Windows 设备管理器中查看实际 VID/PID

## 构建与烧录

```bash
cd /Users/wenxig/Documents/the-spring/packages/esp32p4-usb-test
idf.py set-target esp32p4
idf.py build flash monitor
```

## 预期结果

- 识别 USB 设备
- 打开 CDC-ACM 端口
- 发送 AT 命令
- 收到 "OK" 响应
