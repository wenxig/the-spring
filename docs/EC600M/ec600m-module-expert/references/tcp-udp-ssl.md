# EC600M TCP/UDP Socket 与 SSL/TLS 安全通信手册

本文档详细总结 Quectel EC600M 系列（含 EC600M-CN 等）LTE Cat 1 bis 模组的内置 TCP/IP 协议栈开发指南。涵盖 TCP Client/Server、UDP Client/Server、三种数据传输访问模式（Buffer Access / Direct Push / Transparent 透传）、多路 Socket 并发复用，以及基于 SSL/TLS 3.0/TLS 1.2 的加密安全连接配置。

---

## 1. TCP/IP 架构与多链路能力

EC600M 内部集成了完整的 TCP/IP 协议栈：
- **PDP 上下文复用**：支持 `1 ~ 3` 个独立的 PDP Context（`<contextID>`）。
- **并发连接数**：支持最大 **12 路** 并发 Socket 连接（`<connectID>`：`0 ~ 11`）。
- **传输模式**：
  1. **Buffer Access Mode（缓存访问模式，推荐）**：数据存放在模块内部缓冲区，通过 AT 指令写入与读取，流程清晰可控。
  2. **Direct Push Mode（直推模式）**：下行数据收到后直接作为 URC 从串口输出。
  3. **Transparent Access Mode（透传模式）**：串口与远程 Socket 建立点对点透明管道，无需 AT 指令封装。

---

## 2. TCP 客户端 (Client) 开发全流程

### 2.1 Buffer Access 模式开发序列

```text
// 1. 激活 PDP 上下文 (Context 1)
AT+QIACT=1
OK

// 2. 建立 TCP 客户端连接
// 语法: AT+QIOPEN=<contextID>,<connectID>,"TCP","<remote_ip>",<remote_port>[,<local_port>[,<access_mode>]]
// <access_mode>: 0=Buffer, 1=Direct Push, 2=Transparent
AT+QIOPEN=1,0,"TCP","220.180.239.212",8055,0,0
OK

+QIOPEN: 0,0       // 异步 URC 上报: connectID=0, err=0 (0表示连接成功建立)

// 3. 查询当前 Socket 连接状态 (QISTATE)
AT+QISTATE=1,0
+QISTATE: 0,"TCP","220.180.239.212",8055,49152,2,1,0,0,"uart1"
OK
// 状态代码: 2=Connected (已连接), 3=Closing, 4=Closed

// 4. 发送数据 (两种方式)
// 方式 A: 定长发送 (例如发送 10 字节)
AT+QISEND=0,10
> 1234567890       // 输入 10 字节后模块立刻发送
SEND OK

// 方式 B: 变长发送 (输入数据后以 Ctrl+Z 结束，Hex 0x1A)
AT+QISEND=0
> Hello World!<0x1A>
SEND OK

// 5. 接收下行数据
// 当模块收到服务器下行数据时，主串口触发 URC:
+QIURC: "recv",0,15   // 提示 connectID=0 接收到了 15 字节数据

// 主控调用 AT+QIRD 读取数据 (最大读取 1500 字节)
AT+QIRD=0,1500
+QIRD: 15
Quectel_Test_OK
OK

// 6. 关闭连接
AT+QICLOSE=0
OK
+QIURC: "closed",0   // 提示连接已彻底关闭
```

### 2.2 Direct Push 模式开发序列
在建立连接时将 `<access_mode>` 设为 `1`：
```text
AT+QIOPEN=1,1,"TCP","220.180.239.212",8055,0,1
OK

+QIOPEN: 1,0       // 连接成功

// 当有数据到达时，直接由串口吐出：
+QIURC: "recv",1,12
Hello Direct
```

### 2.3 Transparent 透传模式与切回 AT 模式
在建立连接时将 `<access_mode>` 设为 `2`：
```text
AT+QIOPEN=1,2,"TCP","220.180.239.212",8055,0,2
OK

CONNECT             // 收到 CONNECT 标志进入透明透传模式
// 此时串口输入的所有字节直接转发给服务器，收到的网络数据直接输出到串口
```
- **退出透传切回 AT 命令模式**：
  1. 暂停发送数据至少 **1 秒**；
  2. 连续输入 `+++`（且在 1 秒内完成输入）；
  3. 输入完毕后继续等待 **1 秒** 不发送任何字符；
  4. 模块返回 `OK`，表示成功切回 AT 命令模式。
- **重新进入透传模式**：发送 `AT+QISWTMD=2,2`。

---

## 3. UDP 客户端与服务端开发

### 3.1 UDP Client 发送与接收
```text
// 1. 创建 UDP Client (本地端口由系统动态分配)
AT+QIOPEN=1,3,"UDP","220.180.239.212",8056,0,0
OK
+QIOPEN: 3,0

// 2. 发送 UDP 数据报文
AT+QISEND=3,6
> UDP123
SEND OK

// 3. 接收 UDP 数据报文
+QIURC: "recv",3,8
AT+QIRD=3,1500
+QIRD: 8,"220.180.239.212",8056
AckData1
OK
```

### 3.2 UDP Service (固定本地监听端口)
```text
// 创建本地 UDP 监听服务 (ServiceType="UDP SERVICE", 本地端口=5000)
AT+QIOPEN=1,4,"UDP SERVICE","127.0.0.1",0,5000,0
OK
+QIOPEN: 4,0

// 收到远端客户端发来的 UDP 包
+QIURC: "recv",4,10
AT+QIRD=4,1500
+QIRD: 10,"112.25.66.88",6001
HelloServer
OK

// 向指定远端客户端回复数据 (AT+QISENDEX)
AT+QISEND=4,5,"112.25.66.88",6001
> Reply
SEND OK
```

---

## 4. SSL / TLS 安全加密配置 (AT+QSSLCFG)

EC600M 支持 SSL 3.0、TLS 1.0、TLS 1.1 和 **TLS 1.2** 协议，支持多种加密套件（AES, RSA, ECC, PSK 等）。

### 4.1 SSL 上下文参数配置
EC600M 支持配置最多 6 个独立的 SSL 上下文（`<sslctxID>`：`0 ~ 5`）。

```text
// 1. 配置 SSL 协议版本为 TLS 1.2
// 语法: AT+QSSLCFG="sslversion",<sslctxID>,<sslversion> (4=TLS 1.2, 5=All)
AT+QSSLCFG="sslversion",0,4
OK

// 2. 配置加密套件 (0xFFFF 代表支持所有标准 Cipher Suite)
AT+QSSLCFG="ciphersuite",0,0xFFFF
OK

// 3. 配置证书校验级别 (AT+QSSLCFG="seclevel")
// 0: 不校验证书 (用于测试或无 CA 场景)
// 1: 校验服务器端根证书 (单向认证，最常用)
// 2: 双向认证 (同时校验服务器根证书与客户端证书/私钥)
AT+QSSLCFG="seclevel",0,1
OK

// 4. 忽略证书有效日期校验 (防止设备无本地 RTC 时时钟不同步导致校验失败)
AT+QSSLCFG="ignorelocaltime",0,1
OK
```

### 4.2 证书上传与关联 (UFS 文件系统)
在进行单向或双向认证时，证书文件需先通过 `AT+QFUPL` 写入 UFS 文件系统：
```text
// 1. 上传 CA 根证书到 UFS (命名为 "cacert.pem")
AT+QFUPL="cacert.pem",1250
CONNECT
<写入 1250 字节证书内容>
OK

// 2. 关联 CA 证书到 SSL 上下文 0
AT+QSSLCFG="cacert",0,"cacert.pem"
OK

// 3. 若为双向认证，还需上传并关联客户端证书与私钥
AT+QSSLCFG="clientcert",0,"clientcert.pem"
OK
AT+QSSLCFG="clientkey",0,"clientkey.pem"
OK
```

### 4.3 基于 SSL 的 TCP/IP 与 Socket 安全连接
将 Socket 的 Service Type 指定为 `"TCP"` 并在打开前将 Socket 绑定到 SSL 上下文：
```text
// 将 ConnectID 0 绑定到 SSL 上下文 0
AT+QSSLCFG="sslctxid",0,0
OK

// 发起基于 SSL 的安全 TCP 连接 (如连接 443 端口)
AT+QIOPEN=1,0,"TCP","test.quectel.com",443,0,0
OK
+QIOPEN: 0,0
```

---

## 5. 常用网络诊断工具 (PING & NTP)

### 5.1 网络延迟与连通性检测 (AT+QPING)
```text
// 向指定 IP 或域名发送 4 次 Ping 包
AT+QPING=1,"www.baidu.com",32,4
OK

+QPING: 0,"180.101.50.242",32,65,255
+QPING: 0,"180.101.50.242",32,58,255
+QPING: 0,"180.101.50.242",32,62,255
+QPING: 0,"180.101.50.242",32,59,255
+QPING: 0,4,4,0,58,65,61   // 汇总: 0=成功, 发4收4丢0, 最短58ms, 最长65ms, 平均61ms
```

### 5.2 网络时间同步 (AT+QNTP)
```text
// 配置 NTP 服务器并执行对时 (Context 1, "ntp.aliyun.com", 端口 123)
AT+QNTP=1,"ntp.aliyun.com",123
OK

+QNTP: 0,"2026/09/04,13:25:30+32"  // 对时成功 (+32 代表东八区)
```
- 对时成功后，可通过 `AT+CCLK?` 查询模组内置实时时钟（RTC）。
