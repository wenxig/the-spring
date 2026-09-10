# EC600X Windows 平台测试计划

## 背景

- **问题**：macOS 不支持 EC600X USB 驱动，无法直接通过 USB 串口测试模组
- **当前状态**：ESP32-P4 通过 UART1 (GPIO0/GPIO1) 连接 EC600X，发送 AT 命令但收到 0 字节响应
- **目标**：在 Windows 上验证 EC600X 硬件、波特率、AT 响应，排查 ESP32 ↔ EC600X 通信问题

---

## Windows 环境准备

### 1. 安装 Quectel USB 驱动

下载并安装 **Quectel USB 串口驱动**：
- 官方驱动页面：<https://developer.quectel.com/download>
- 搜索 "EC600X USB Driver" 或 "Quectel USB Serial Driver for Windows"
- 安装后，Windows 设备管理器应显示 EC600X 的多个 COM 口（通常是 COM3-COM7 范围）

典型 COM 口分配：
- **AT 命令口**：主串口，用于发送 AT 命令（通常是第一个 COM 口）
- **诊断口**：用于 QXDM 日志抓取
- **NMEA 口**：GPS 数据
- **Modem 口**：数据拨号

### 2. 安装串口调试工具

推荐工具（任选一个）：
- **QCOM (Quectel 官方工具)**：<https://developer.quectel.com/download>（搜索 "QCOM"）
- **串口调试助手** (Serial Port Utility)
- **Tera Term**：<https://ttssh2.osdn.jp/>
- **PuTTY**：<https://www.putty.org/>

---

## 测试步骤

### 步骤 1：验证 EC600X 硬件与驱动

1. **连接 EC600X-EVB 到 Windows**
   - 使用 Micro USB 线连接 EC600X-EVB 的 USB 口到 Windows PC
   - 确认 EC600X-EVB 电源开关在 **USB** 挡位

2. **开机 EC600X**
   - 长按 EC600X-EVB 板载 **PWRKEY** 按键 **1-2 秒**
   - 观察板上 LED 指示灯是否亮起（通常是蓝色或绿色）
   - 等待 10-15 秒，模组完全启动

3. **确认 Windows 识别 COM 口**
   - 打开 **设备管理器**（Win+X → 设备管理器）
   - 展开 **端口 (COM 和 LPT)**
   - 应该看到多个 **Quectel USB Serial Port (COMx)** 设备
   - 记录 **AT 命令口**的 COM 号（通常是第一个，例如 COM3）

### 步骤 2：AT 命令测试

1. **打开串口工具**（以 QCOM 为例）
   - 选择 AT 命令口（例如 COM3）
   - 波特率：**115200**
   - 数据位：8
   - 停止位：1
   - 校验位：无
   - 流控：无

2. **发送基础 AT 命令**
   ```
   AT                    # 基础握手，应返回 OK
   ATI                   # 模组信息，返回型号和固件版本
   AT+CPIN?              # SIM 卡状态，返回 +CPIN: READY 或 ERROR
   AT+CSQ                # 信号强度，返回 +CSQ: rssi,ber
   AT+CEREG?             # 网络注册状态
   AT+QGMR               # 固件版本
   ```

3. **记录结果**
   - ✅ 所有命令正常响应 → EC600X 硬件正常，波特率确认为 115200
   - ❌ 无响应或乱码 → 尝试其他波特率（9600、57600、921600）或检查 COM 口选择
   - ⚠️ 部分命令失败 → 可能是 SIM 卡未插入或固件问题

### 步骤 3：验证 UART 主串口（J5 排针）

如果 USB 测试正常，接下来验证 **J5 排针的主串口（TX0/RX0）**：

1. **断开 ESP32 连接**
   - 拔掉 ESP32 ↔ EC600X 的三根杜邦线（E1/E2/E3）

2. **使用 USB-UART 转接器连接 J5**
   - **TX** (转接器) → **J5-7 (RX0)** (EC600X 接收)
   - **RX** (转接器) → **J5-6 (TX0)** (EC600X 发送)
   - **GND** (转接器) → **J5-1 (GND)**

3. **在 Windows 上打开转接器的 COM 口**
   - 波特率：115200
   - 发送 `AT` 命令
   - 应该收到 `OK` 响应

4. **记录结果**
   - ✅ J5 主串口正常响应 → 硬件正常，问题在 ESP32 侧
   - ❌ J5 无响应 → EC600X 主串口故障或波特率配置问题

---

## 常见问题排查

### 问题 1：Windows 设备管理器无 COM 口

**原因**：
- 驱动未正确安装
- EC600X 未开机
- USB 线损坏或接触不良

**解决**：
1. 重新安装 Quectel USB 驱动
2. 确认已长按 PWRKEY 开机，LED 亮起
3. 更换 USB 线或 USB 接口

### 问题 2：AT 命令无响应

**原因**：
- 选错 COM 口（选择了诊断口而非 AT 命令口）
- 波特率不匹配
- EC600X 未完全启动

**解决**：
1. 逐个尝试所有 Quectel COM 口
2. 尝试常见波特率：9600、115200、921600
3. 等待 15-20 秒后再发送命令

### 问题 3：AT 命令响应乱码

**原因**：
- 波特率错误
- 数据位/停止位/校验位配置错误

**解决**：
1. 确认串口参数：115200 8N1
2. 关闭流控
3. 尝试其他波特率

---

## ESP32 ↔ EC600X 调试决策树

根据 Windows 测试结果，确定下一步：

### 情况 A：EC600X USB 正常，J5 主串口正常
**结论**：EC600X 硬件正常，问题在 ESP32 侧

**下一步**：
1. 检查 ESP32 接线是否接反（GPIO0 ↔ GPIO1）
2. 在 ESP32 代码中添加 UART loopback 测试
3. 使用逻辑分析仪检查 ESP32 UART 信号

### 情况 B：EC600X USB 正常，J5 主串口无响应
**结论**：EC600X 主串口故障或配置问题

**下一步**：
1. 通过 USB AT 命令检查串口配置：`AT+IPR?`（查询波特率）
2. 尝试设置波特率：`AT+IPR=115200`
3. 检查 EC600X 固件版本，可能需要升级

### 情况 C：EC600X USB 无响应
**结论**：EC600X 硬件故障或未开机

**下一步**：
1. 确认 PWRKEY 开机流程（长按 1-2 秒，观察 LED）
2. 检查 EC600X-EVB 供电（USB 挡位，5V 输入）
3. 联系硬件供应商或更换模组

---

## 测试结果记录模板

```markdown
## EC600X Windows 测试结果

**测试日期**：YYYY-MM-DD
**测试人员**：

### 1. 硬件识别
- [ ] Windows 设备管理器显示 Quectel COM 口
- [ ] AT 命令口编号：COM___
- [ ] 模组已开机（LED 亮起）

### 2. USB AT 命令测试
| 命令 | 响应 | 状态 |
|------|------|------|
| AT | | ✅ / ❌ |
| ATI | | ✅ / ❌ |
| AT+CPIN? | | ✅ / ❌ |
| AT+CSQ | | ✅ / ❌ |
| AT+CEREG? | | ✅ / ❌ |

### 3. J5 主串口测试（使用 USB-UART 转接器）
- [ ] 转接器 COM 口：COM___
- [ ] 波特率：115200
- [ ] AT 命令响应：✅ / ❌

### 4. 结论
- EC600X 硬件状态：正常 / 故障
- 确认波特率：_______
- 下一步行动：__________________
```

---

## 返回 macOS 后的操作

完成 Windows 测试后，根据结果在 macOS 上：

1. **如果 EC600X 正常**
   - 修复 ESP32 接线或代码
   - 重新测试 ESP32 ↔ EC600X 通信

2. **如果发现波特率不是 115200**
   - 更新 ESP32 代码中的波特率配置（`at_engine.cpp:20`）

3. **如果 J5 主串口故障**
   - 联系供应商更换模组
   - 或通过 USB-UART 转接器作为临时方案

---

## 相关文档

- [EC600X-EVB 硬件技能](../.agents/skills/quectel-ec600x-evb/SKILL.md)
- [接线方案](../packages/hardware/README.md)
- [ESP32 固件工程](../packages/esp32p4/)
