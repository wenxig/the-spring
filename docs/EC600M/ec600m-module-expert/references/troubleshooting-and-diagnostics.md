# EC600M 故障排查与错误码速查手册

本文档系统汇总 Quectel EC600M 系列（含 EC600M-CN 等）LTE Cat 1 bis 模组在硬件调试、固件开发与网络联调中最常见的故障现象、排查诊断流程以及全套错误代码速查表（CME ERROR, CMS ERROR, TCP/IP ERROR, MQTT ERROR）。

---

## 1. 硬件常见故障与排查流程

### 1.1 模块完全不上电 / 无法开机
- **现象**：拉低 PWRKEY 后，`VDD_EXT` 引脚没有 1.8V 输出，主串口没有输出 `RDY`。
- **排查步骤**：
  1. **测 VBAT 电压**：使用示波器或万用表测量模块 `VBAT_BB` / `VBAT_RF` 引脚，确认是否在 `3.4 V ~ 4.5 V`（推荐 3.8 V）。
  2. **查 PWRKEY 拉低时序**：确认 PWRKEY 是否在 VBAT 稳定后拉低持续了 **$\ge 700\ \text{ms}$**，且拉低时的对地电平低于 `0.45 V`。
  3. **查 RESET_N 引脚**：确认 `RESET_N` 没有被外部电路意外拉低（正常工作状态下应为高电平 1.8V 或悬空）。
  4. **排查电源短路/反接**：检查 VBAT 对地阻抗是否正常，有无焊接桥连。

### 1.2 搜网阶段异常重启或关机 (开机后自动掉电)
- **现象**：模块开机输出 `RDY`，但几秒后或在执行 `AT+QIACT` / 搜网时瞬间重启。
- **根本原因**：**供电能力不足导致 VBAT 产生严重瞬间跌落（Voltage Drop）**。
- **排查与解决**：
  1. 用示波器抓取搜网瞬间的 VBAT 跌落波形。如果波形跌落到 **$3.3\ \text{V}$ 以下**，基带芯片将触发低压欠压保护（Under-voltage lockout）而关机。
  2. 检查供电电源输出能力是否达到持续 **2.0 A**。
  3. 检查 VBAT 走线宽度是否 $< 2.0\ \text{mm}$，过孔数量是否过少。
  4. 在靠近模块 VBAT 引脚处补焊 $100\ \mu\text{F} \sim 470\ \mu\text{F}$ 低 ESR 钽电容。

### 1.3 休眠电流偏大（无法进入深睡眠）
- **现象**：配置 `AT+QSCLK=1` 并拉高 DTR 后，待机电流仍然高达 10 mA ~ 25 mA（正常应 $< 2.0\ \text{mA}$）。
- **排查步骤**：
  1. **USB 接口供电倒灌**：检查 USB VBUS 是否仍有 5V 供电。若 USB 连着电脑，模块会维持 USB 枚举状态而无法休眠。
  2. **I/O 漏电排查**：检查主控与模块之间的 TXD/RXD/GPIO 引脚。若主控为 3.3V 且未断电，可能通过内部 ESD 二极管向模块 1.8V 域漏电。
  3. **检查网络侧 DRX**：通过 `AT+CEDRXS?` 查询是否被基站拒绝或配置了过短的寻呼周期。

---

## 2. 核心网络与通信错误排查

### 2.1 SIM 卡不识别 (+CME ERROR: 10)
1. 检查 SIM 卡座引脚供电 `USIM_VDD` 在开机瞬间是否有输出（若无插卡，模块尝试供电识别几次后会关闭该电源）。
2. 检查 `USIM_DATA` 上拉电阻是否存在。
3. 检查 SIM 卡引脚上的 TVS 管结电容是否过大（要求 $\le 15\ \text{pF}$），是否存在静电击穿短路。
4. 检查 `AT+QSIMDET` 热插拔配置是否与实际卡座开关类型匹配。

### 2.2 附着失败 / 注册被拒绝 (+CEREG: 0,3)
1. 确认天线是否焊接良好，特性阻抗是否为 $50\ \Omega$。
2. 通过 `AT+CSQ` 查看信号是否 $\ge 12$。
3. 确认 SIM 卡是否欠费、停机、被锁卡，或是否属于定向物联网专用卡（APN 不对会导致鉴权被拒）。
4. 尝试执行 `AT+CFUN=0` 延时 2 秒后 `AT+CFUN=1` 重新搜网。

---

## 3. TCP/IP 协议栈错误代码速查表 (QICSGP / QIOPEN)

在执行 TCP/IP 指令返回 `+CME ERROR: <err>` 或 `+QIOPEN: <connectID>,<err>` 时，对应含义如下：

| 错误代码 (Error Code) | 错误定义 (Macro) | 详细原因与排查方案 |
| :--- | :--- | :--- |
| **550** | Unknown error | 未知内部错误，通常重试或重启模组可恢复。 |
| **551** | Operation blocked | 操作被阻塞。前一个网络动作正在执行中，等待其完成。 |
| **552** | Invalid parameters | 参数非法。检查 AT 指令入参格式、端口号范围或字符串引号。 |
| **553** | Memory not enough | 内存不足。关闭闲置的 Socket 或释放 UFS 文件缓冲区。 |
| **554** | Create socket failed | 创建套接字失败。已达到最大 12 路连接限制，需先关闭废弃连接。 |
| **555** | Operation not supported | 操作不支持。当前模式不支持此指令。 |
| **556** | Socket bind failed | Socket 端口绑定失败。本地端口已被占用。 |
| **557** | Socket listen failed | Socket 监听失败。 |
| **558** | Socket write failed | 发送数据写入缓冲区失败。通常表示对端已断开或缓存溢出。 |
| **559** | Socket read failed | 读取缓冲区数据失败。 |
| **560** | Socket connect failed | 连接远端服务器失败。检查目标 IP、端口是否开放，防火墙是否拦截。 |
| **561** | Socket already connected | 该 connectID 已经处于连接状态，不可重复建立。 |
| **562** | Socket closed | 套接字已被远端或本地关闭。 |
| **563** | DNS parse failed | **域名解析失败 (DNS 错误)**。检查 PDP 是否正常激活、APN 是否正确、DNS 服务器是否可达。 |
| **564** | PDP context deactivated | **PDP 上下文已被去激活 (掉线)**。需重新执行 `AT+QIACT=1`。 |
| **565** | PDP context activate failed | PDP 上下文激活失败。检查 APN 配置及 SIM 卡状态。 |

---

## 4. MQTT 协议栈错误代码速查表 (AT+QMT...)

在执行 MQTT 指令时，URC 返回 `+QMTOPEN: <id>,<err>`、`+QMTCONN: <id>,<res>,<ret_code>` 或发布订阅错误：

### 4.1 QMTOPEN 网络建立错误码 (`+QMTOPEN: <id>,<err>`)
- `-1`：打开网络连接失败（无法解析域名或 TCP 握手被拒）。
- `0`：成功建立网络连接。
- `1`：参数错误。
- `2`：标识符已被占用（需先 `AT+QMTCLOSE`）。
- `3`：正在激活 PDP 或网络通信中。
- `4`：网络断开连接。
- `5`：SSL/TLS 握手失败（检查证书格式、有效时间或 `ignorelocaltime` 配置）。

### 4.2 QMTCONN 连接应答返回码 (`+QMTCONN: <id>,<res>,<ret_code>`)
当 `<res>` = 0 时，`<ret_code>` 代表 MQTT Broker 返回的 ConnAck 状态码：
- `0`：**Connection Accepted (连接成功)**。
- `1`：Connection Refused: Unacceptable protocol version（不支持的协议版本，尝试切换 `version` 为 4 即 v3.1.1）。
- `2`：Connection Refused: Identifier rejected（ClientID 格式非法或已被占用）。
- `3`：Connection Refused: Server unavailable（服务器服务不可用）。
- `4`：Connection Refused: Bad user name or password（**用户名或密码错误**，检查三元组签名或 Token）。
- `5`：Connection Refused: Not authorized（**未授权连接**，ACL 权限不足）。

---

## 5. 常见 3GPP 标准 CME / CMS 错误码速查

| 错误代码 | 错误名称 | 常见原因 |
| :--- | :--- | :--- |
| `+CME ERROR: 3` | Operation not allowed | 操作不允许（例如在未激活 PDP 时尝试建立连接）。 |
| `+CME ERROR: 10` | SIM not inserted | 未检测到 SIM 卡。 |
| `+CME ERROR: 11` | SIM PIN required | 需要输入 SIM PIN 码（`AT+CPIN=xxxx`）。 |
| `+CME ERROR: 12` | SIM PUK required | SIM 卡已被锁死，需要运营商 PUK 码解锁。 |
| `+CME ERROR: 13` | SIM failure | SIM 卡硬件故障。 |
| `+CME ERROR: 14` | SIM busy | SIM 卡正忙，稍后重试。 |
| `+CME ERROR: 30` | No network service | 当前无可用蜂窝网络信号。 |
| `+CME ERROR: 100` | Unknown | 未知基带错误。 |
| `+CMS ERROR: 302` | Operation not allowed | 短信操作不允许。 |
| `+CMS ERROR: 500` | Unknown error | 短信中心服务异常。 |
