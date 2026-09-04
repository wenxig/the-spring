# EC600M AT 核心与网络管理参考手册

本文档详细梳理 Quectel EC600M 系列（含 EC600M-CN 等）LTE Cat 1 bis 模组的核心系统控制、网络注册管理、PDP 上下文激活、基站定位（QuecCell/QENG）、Wi-Fi 扫描（QWIFISCAN）以及 USB 端口与系统参数配置。

---

## 1. 基础系统与通信控制

### 1.1 串口自适应与波特率设置
- **自适应波特率 (Autobauding)**：出厂默认开启自适应，开机后主控连续发送 `AT\r\n`，模组自动同步波特率并回复 `OK`。
- **固定波特率配置**：
  ```text
  AT+IPR=115200       // 设置主串口固定波特率为 115200 bps
  OK
  AT&W                // 将当前参数保存到 NVRAM（掉电不丢失）
  OK
  ```
  - 支持波特率：`9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600` 等。

### 1.2 回显与字符编码
- `ATE0`：关闭命令行回显（推荐在嵌入式程序中使用，大幅简化串口解析）。
- `ATE1`：开启回显（调试使用）。
- `AT+CSCS="GSM"` 或 `AT+CSCS="UCS2"`：设置字符编码集。

### 1.3 射频模式与飞行模式 (AT+CFUN)
- `AT+CFUN=0`：最小功能模式（关闭射频发射与接收，降低功耗）。
- `AT+CFUN=1`：全功能模式（正常入网通信）。
- `AT+CFUN=4`：飞行模式（关闭 RF 射频收发）。
- **常用重注网技巧**：在长时间网络异常或基站附着失败时，可通过 `AT+CFUN=0` 延时 2 秒后再 `AT+CFUN=1` 触发模块完整重新搜网。

---

## 2. SIM 卡与网络注册管理

### 2.1 SIM 卡状态查询 (AT+CPIN)
```text
AT+CPIN?
+CPIN: READY        // SIM 卡正常就绪
OK
```
- 若返回 `+CME ERROR: 10`（SIM not inserted），需检查 SIM 卡座引脚、供电 `USIM_VDD` 及供电线路 ESD 滤波电容。

### 2.2 信号质量与射频参数 (AT+CSQ / AT+QENG)
```text
AT+CSQ
+CSQ: 26,99         // 参数1: RSSI (0~31, 99未知), 参数2: BER
OK
```
- **RSSI 等级参考**：
  - `0 ~ 9`：信号极差，掉线率极高。
  - `10 ~ 14`：信号弱，能通信但吞吐量可能受限。
  - `15 ~ 19`：信号良好。
  - `20 ~ 31`：信号优秀。

### 2.3 LTE EPS 网络附着与注册 (AT+CGATT / AT+CEREG)
```text
// 1. 查询分组业务附着状态
AT+CGATT?
+CGATT: 1           // 1=已附着 (Attached)
OK

// 2. 查询 LTE 专用注册状态
AT+CEREG?
+CEREG: 0,1         // 状态值: 1=本地注册成功, 5=漫游注册成功
OK
```
- **+CEREG 状态码速查**：
  - `0`：未注册，未在搜网。
  - `1`：已注册，本地网络 (Home network)。
  - `2`：未注册，正在搜网 (Searching)。
  - `3`：注册被拒绝 (Registration denied)。
  - `4`：未知状态。
  - `5`：已注册，处于漫游网络 (Roaming)。

---

## 3. PDP 上下文配置与激活 (QICSGP & QIACT)

在进行任何 TCP/UDP/MQTT/HTTP 数据通信前，必须先完成 PDP 上下文配置与激活。

### 3.1 配置 PDP 场景 (AT+QICSGP)
**语法**：`AT+QICSGP=<contextID>,<context_type>,<APN>[,<username>,<password>[,<auth_type>]]`
- `<contextID>`：PDP 上下文索引，范围 `1 ~ 3`（通常默认使用 `1`）。
- `<context_type>`：协议类型，`1` = IPv4，`2` = IPv4v6，`3` = IPv6。
- `<APN>`：运营商接入点名称（中国移动 `"CMNET"` / `"CMIOT"`，中国联通 `"UNINET"` / `"3GNET"`，中国电信 `"CTNET"` / `"CTIOT"`，私网 APN 填写专用名称）。
- `<auth_type>`：认证方式，`0` = None，`1` = PAP，`2` = CHAP，`3` = PAP or CHAP。

```text
// 示例：配置 Context 1 为移动通用 APN，无认证
AT+QICSGP=1,1,"CMNET","","",1
OK
```

### 3.2 激活与去激活 PDP 上下文 (AT+QIACT / AT+QIDEACT)
```text
// 1. 激活 Context 1
AT+QIACT=1
OK

// 2. 查询已激活的 IP 地址
AT+QIACT?
+QIACT: 1,1,1,"10.145.23.68"
OK

// 3. 去激活 Context 1（断开网络）
AT+QIDEACT=1
OK
```

---

## 4. 基站信息与 QuecCell 定位服务

EC600M 提供强大的工程模式基站查询与基站定位功能。

### 4.1 工程模式小区信息查询 (AT+QENG)
```text
// 查询服务小区详细信息
AT+QENG="servingcell"
+QENG: "servingcell","NOCONN","LTE","FDD",460,00,1A2B,3C4D5E,1825,3,5,5,B3,-95,-12,-65,15,45
OK

// 查询邻区信息 (Neighbor Cells)
AT+QENG="neighbourcell"
+QENG: "neighbourcell intra","LTE",1825,123,-12,-96,-66,0,-,-,-,-,-
+QENG: "neighbourcell inter","LTE",38950,456,-14,-102,-72,0,-,-,-,-,-
OK
```
- **核心字段解析**：`MCC` (460=中国), `MNC` (00/02/07/08 移动, 01 联通, 11 电信), `LAC/TAC` (1A2B), `CellID` (3C4D5E), `Band` (B3), `RSRP` (-95 dBm), `RSRQ` (-12 dB), `SINR` (15 dB)。

### 4.2 QuecCell 基站定位 (AT+QCELL)
支持利用当前基站及邻区信息向云端服务器请求经纬度坐标：
```text
AT+QCELL=1,"CMNET"
+QCELL: 121.365421,31.225890,550
OK
```

---

## 5. Wi-Fi Scan 室内定位辅助扫描 (AT+QWIFISCAN)

EC600M 支持 Wi-Fi Scan 硬件扫描功能（仅扫描周围 2.4GHz Wi-Fi AP 的 BSSID 和 RSSI，不支持连接 Wi-Fi）。常用于室内外混合定位。

### 5.1 配置与触发扫描
```text
// 1. 配置扫描超时与最大热点数量
AT+QWIFISCAN=1,10,20       // 模式1, 超时10秒, 最多扫描20个AP
OK

// 2. 模块上报周围 Wi-Fi 列表
+QWIFISCAN: "aa:bb:cc:dd:ee:01",-65,1
+QWIFISCAN: "aa:bb:cc:dd:ee:02",-78,6
+QWIFISCAN: "aa:bb:cc:dd:ee:03",-85,11
OK
```
- 返回参数：`MAC 地址 (BSSID)`, `信号强度 (RSSI)`, `信道 (Channel)`。
- 主控可将此列表打包上传至百度/高德/腾讯云定位 API，实现米级精度的室内无 GPS 定位。

---

## 6. USB 描述符与端口配置 (AT+QCFG="usbdesc")

EC600M 的 USB 接口支持多种复合设备端口配置（CDC-ACM, ECM, RNDIS 等）。

### 6.1 常用 USB 端口模式配置
```text
// 查询当前 USB 描述符配置
AT+QCFG="usbdesc"
+QCFG: "usbdesc",0x2C7C,0x6008,1,1,1,1,1,0,0
OK
```
- **常用端口构成**：
  - `VID`: `0x2C7C` (Quectel)
  - `PID`: `0x6008` (标准 4 端口模式)
  - 虚拟串口映射：
    1. **Quectel USB AT Port**：用于发送 AT 指令与数据通信；
    2. **Quectel USB Modem Port**：用于 PPP 拨号与数据通信；
    3. **Quectel USB NMEA Port**：用于输出 GPS/GNSS 定位报文（如有内置/外接）；
    4. **Quectel USB DM/Diag Port**：用于底层抓包、调试诊断与固件升级。

### 6.2 网卡模式切换 (ECM / RNDIS)
- 在 Linux / Android 主机平台下，可将模组配置为免拨号 USB 网卡模式：
  - `AT+QCFG="usbnet",1`：配置为 **ECM** 模式（标准 Linux CDC-ECM 网卡）。
  - `AT+QCFG="usbnet",3`：配置为 **RNDIS** 模式（Windows / Android 常用网卡驱动）。
  - 配置完成后需执行 `AT+CFUN=1,1` 重启模组生效。
