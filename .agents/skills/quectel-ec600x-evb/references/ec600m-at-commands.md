# EC600M 模组核心 AT 指令与网络接入开发指南

当 EC600X-EVB 作为纯蜂窝调制解调器（Modem）受控于外部主控芯片（如 ESP32-P4）时，通过 UART0 发送 AT 指令实现网络配置、Socket 数据透传与 MQTT 云端接入。

---

## 1. 串口配置与网络初始化时序

- **波特率**: 默认 `115200 8N1`（支持硬件流控 RTS/CTS，或通过 `AT+IPR` 动态调整）。
- **回显与错误报告格式**:
  ```text
  ATE0          // 关闭回显，减少串口解析负担
  AT+CMEE=2     // 开启详细文本错误报告（方便排错）
  ```

### 1.1 SIM 卡与网络驻网检查
```text
AT+CPIN?
// 期望响应: +CPIN: READY (若报 +CME ERROR: 10 表示未检测到 SIM 卡)

AT+CSQ
// 查询信号强度: +CSQ: <rssi>,<ber> (rssi 应在 15~31 之间良好)

AT+CREG?
// 检查 CS 域注册: +CREG: 0,1 (本地) 或 0,5 (漫游)

AT+CEREG?
// 检查 EPS/LTE 驻网状态: +CEREG: 0,1 或 0,5
```

---

## 2. 数据上下文 (PDP) 激活流程

```text
// 1. 配置 APN (以中国移动 CMMTM 为例，可按需留空自动适配)
AT+QICSGP=1,1,"CMIOT","","",1

// 2. 激活场景 1
AT+QIACT=1
// 响应: OK

// 3. 查询获取到的 IP 地址
AT+QIACT?
// 响应: +QIACT: 1,1,1,"10.xx.xx.xx"
```

---

## 3. TCP/UDP Socket 通信

```text
// 打开 TCP 连接 (contextID=1, connectID=0, "TCP", "ip", port)
AT+QIOPEN=1,0,"TCP","220.180.239.212",8009,0,1
// 响应:
// OK
// +QIOPEN: 0,0  (第二参数为 0 表示成功建连)

// 发送数据 (发送指定字节长度)
AT+QISEND=0,5
> Hello
// 响应: SEND OK

// 关闭连接
AT+QICLOSE=0
```

---

## 4. MQTT 客户端指令实战

```text
// 1. 配置 MQTT 会话参数 (clientId, username, password)
AT+QMTCFG="version",0,4  // 4 代表 MQTT 3.1.1
AT+QMTCFG="recv/mode",0,0,1 // 收到消息时上报 URC

// 2. 连接 MQTT 服务器
AT+QMTOPEN=0,"broker.emqx.io",1883
// 等待 URC: +QMTOPEN: 0,0

// 3. 登录 Broker
AT+QMTCONN=0,"esp32_p4_client","user","password"
// 等待 URC: +QMTCONN: 0,0,0

// 4. 订阅主题
AT+QMTSUB=0,1,"the-spring/cmd",1
// 等待 URC: +QMTSUB: 0,1,0,1

// 5. 发布主题数据
AT+QMTPUBEX=0,0,0,0,"the-spring/telemetry",13
> {"status":"ok"}
// 等待 URC: +QMTPUBEX: 0,0,0
```

---

## 5. 低功耗与休眠控制

- 启用睡眠模式：
  ```text
  AT+QSCLK=1
  ```
- 当外部主控将 `DTR` 引脚拉高且模组内部空闲时，模组自动进入低功耗休眠（电流降至 2mA 左右）。
- 主控需发送数据时，先将 `DTR` 拉低唤醒，等待 20ms 后正常发送 AT 指令。
- 外部网络来电或下行短信/数据时，模组通过 `MAIN_RI` 引脚拉低 120ms 通知主控唤醒。
