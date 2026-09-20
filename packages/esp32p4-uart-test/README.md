# EC600X UART 最小测试项目

最小化测试项目，专注于 ESP32-P4 ↔ EC600X UART 通信调试。

## 功能

- 仅测试 UART1 (GPIO0 TX / GPIO1 RX) 与 EC600X 的 AT 命令通信
- 发送 3 次 `AT` 命令，记录响应
- 无其他外设、无 SD 卡、无 WiFi

## 构建与烧录

```bash
cd /Users/wenxig/Documents/the-spring/packages/esp32p4-uart-test
idf.py set-target esp32p4
idf.py build
idf.py -p /dev/cu.usbmodem141101 flash monitor
```

## 预期结果

**成功：**
```
I (xxx) uart_test: --- 测试 1/3 ---
I (xxx) uart_test: 发送: AT (写入 4 字节)
I (xxx) uart_test: ✅ 收到 6 字节:
AT
OK
```

**失败：**
```
I (xxx) uart_test: --- 测试 1/3 ---
I (xxx) uart_test: 发送: AT (写入 4 字节)
E (xxx) uart_test: ❌ 超时，未收到任何数据
```

## 如果测试失败

1. **检查接线**
   - E1: GPIO0 (P6-24) → J5-7 (EC RX)
   - E2: GPIO1 (P6-21) → J5-6 (EC TX)
   - E3: GND (P6-26) → J5-1 (GND)

2. **确认 EC600X 已开机**
   - 长按 PWRKEY 1-2 秒
   - LED 指示灯应亮起

3. **尝试交换 TX/RX**
   - 修改 `main/main.c` 第 32 行：
   ```c
   // 原: uart_set_pin(UART_PORT_NUM, GPIO_NUM_0, GPIO_NUM_1, ...);
   // 改: uart_set_pin(UART_PORT_NUM, GPIO_NUM_1, GPIO_NUM_0, ...);
   ```

## 接线参考

见主项目：`../../packages/hardware/README.md`
