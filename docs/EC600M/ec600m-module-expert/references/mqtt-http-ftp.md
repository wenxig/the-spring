# EC600M 物联网应用协议手册 (MQTT, HTTP, FTP)

本文档详细总结 Quectel EC600M 系列（含 EC600M-CN 等）LTE Cat 1 bis 模组的内置高层物联网应用协议开发指南。覆盖 MQTT 协议接入（阿里云、腾讯云、EMQX、AWS IoT）、HTTP(S) 请求与 RESTful API 调用、以及 FTP(S) 文件上传与下载全流程。

---

## 1. MQTT 协议开发全指南 (AT+QMT)

EC600M 模组内置 MQTT 协议栈，全面支持 MQTT v3.1 与 MQTT v3.1.1 规范，支持 QoS 0/1/2 消息等级、遗嘱消息（Will Message）、SSL/TLS 加密传输及断线重连。

### 1.1 MQTT 客户端基础配置 (AT+QMTCFG)
EC600M 支持最大 **6 路** 并发 MQTT 客户端实例（`<tcpconnectID>`：`0 ~ 5`）。

```text
// 1. 配置 PDP 上下文关联 (实例 0 关联 Context 1)
AT+QMTCFG="pdpcid",0,1
OK

// 2. 配置 MQTT 协议版本 (3=v3.1, 4=v3.1.1 默认)
AT+QMTCFG="version",0,4
OK

// 3. 配置心跳保活间隔 Keepalive (例如 60 秒)
AT+QMTCFG="keepalive",0,60
OK

// 4. 配置 Session 会话标志 (0=保留会话, 1=清理会话 Clean Session)
AT+QMTCFG="session",0,1
OK

// 5. 配置接收下行消息格式 (0=文本格式, 1=Hex 16进制格式)
AT+QMTCFG="recv/mode",0,0,1
OK

// 6. 配置 SSL/TLS 加密 (若连接 MQTTS 8883 端口，绑定 SSL 上下文 0)
AT+QMTCFG="ssl",0,1,0
OK
```

### 1.2 连接 Broker、订阅与发布完整流程

```text
// 步骤 1: 打开 MQTT 网络通道 (实例 0, 域名/IP, 端口)
AT+QMTOPEN=0,"broker.emqx.io",1883
OK

+QMTOPEN: 0,0       // URC: 实例0, 结果0(网络通道建立成功)

// 步骤 2: 发送 CONNECT 报文连接 Broker (ClientID, Username, Password)
AT+QMTCONN=0,"EC600M_Client_001","admin","public"
OK

+QMTCONN: 0,0,0     // URC: 实例0, 结果0(成功), 返回码0(Accepted)

// 步骤 3: 订阅主题 (实例 0, MsgID=1, Topic="device/control", QoS=1)
AT+QMTSUB=0,1,"device/control",1
OK

+QMTSUB: 0,1,0,1    // URC: 订阅成功，返回授权 QoS=1

// 步骤 4: 发布消息 (实例 0, MsgID=2, QoS=1, Retain=0, Topic="device/telemetry")
AT+QMTPUB=0,2,1,0,"device/telemetry"
> {"voltage":3.82,"temp":24.5}<Ctrl+Z>
OK

+QMTPUB: 0,2,0      // URC: 发布成功并收到 PUBACK

// 步骤 5: 接收 Broker 推送的下行消息
// 当云端向 "device/control" 发送消息时，串口输出 URC:
+QMTRECV: 0,0,"device/control",18,"{\"relay\":1,\"delay\":5}"

// 步骤 6: 取消订阅与断开连接
AT+QMTUNS=0,3,"device/control"
OK
+QMTUNS: 0,3,0

AT+QMTDISC=0
OK
+QMTDISC: 0,0

AT+QMTCLOSE=0
OK
```

### 1.3 阿里云 IoT 平台快速接入实战
阿里云物联网平台采用三元组认证（ProductKey, DeviceName, DeviceSecret）：
1. **生成 MQTT 认证参数**：
   - `ClientID`: `${DeviceName}|securemode=3,signmethod=hmacsha1|`
   - `Username`: `${DeviceName}&${ProductKey}`
   - `Password`: `hmacsha1(DeviceSecret, "clientId${DeviceName}deviceName${DeviceName}productKey${ProductKey}")`
2. **连接 AT 序列**：
   ```text
   AT+QMTOPEN=0,"${ProductKey}.iot-as-mqtt.cn-shanghai.aliyuncs.com",1883
   OK
   +QMTOPEN: 0,0

   AT+QMTCONN=0,"${ClientID}","${Username}","${Password}"
   OK
   +QMTCONN: 0,0,0
   ```

---

## 2. HTTP / HTTPS 协议开发指南 (AT+QHTTP)

EC600M 内置 HTTP(S) 协议栈，支持 GET, POST, PUT 等标准 HTTP 交互方法。

### 2.1 HTTP(S) 基础配置 (AT+QHTTPCFG)
```text
// 1. 关联 PDP 上下文
AT+QHTTPCFG="contextid",1
OK

// 2. 启用 HTTPS (绑定 SSL 上下文 0)
AT+QHTTPCFG="sslctxid",0
OK

// 3. 配置 HTTP 请求超时时间 (连接超时 30s, 传输超时 60s)
AT+QHTTPCFG="timeout",30,60
OK

// 4. 配置自定义 HTTP Header (开启自定义 Header 输入模式)
AT+QHTTPCFG="requestheader",1
OK
```

### 2.2 HTTP GET 请求全流程
```text
// 1. 设置请求的目标 URL
// 语法: AT+QHTTPURL=<url_length>,<input_timeout>
AT+QHTTPURL=28,5
CONNECT
http://api.weather.com/v1/now
OK

// 2. 发起 GET 请求 (响应超时 60s)
AT+QHTTPGET=60
OK

+QHTTPGET: 200,128,128  // URC: HTTP状态码200, 实际读取长度128, 总内容长度128

// 3. 读取服务器返回的数据到串口
AT+QHTTPREAD=60
CONNECT
{"code":200,"temp":22,"humidity":55,"city":"Shanghai"}
OK
```

### 2.3 HTTP POST 请求全流程 (JSON 数据上报)
```text
// 1. 设置请求 URL
AT+QHTTPURL=26,5
CONNECT
https://api.iot.com/report
OK

// 2. 发送 POST Body 数据 (例如 Body 长度 35 字节，输入超时 10s)
AT+QHTTPPOST=35,10,60
CONNECT
{"device_id":"EC600M_01","status":1}
OK

+QHTTPPOST: 200,45,45   // URC: 收到 200 OK 响应，响应长度 45 字节

// 3. 读取响应内容
AT+QHTTPREAD=60
CONNECT
{"success":true,"message":"Data recorded."}
OK
```

---

## 3. FTP / FTPS 文件传输协议指南 (AT+QFTP)

EC600M 内置 FTP/FTPS 客户端，支持主动模式（Active）与被动模式（Passive）、断点续传以及基于 SSL/TLS 的 FTPS 安全传输。常用于远程固件升级（FOTA）包下载或日志上传。

### 3.1 FTPS 基础参数配置
```text
// 1. 配置关联的 PDP 上下文
AT+QFTPCFG="contextid",1
OK

// 2. 配置 FTP 账户与密码
AT+QFTPCFG="account","ftp_user","ftp_password"
OK

// 3. 配置传输模式 (1=Passive 被动模式 推荐)
AT+QFTPCFG="transmode",1
OK

// 4. 启用 FTPS 安全加密 (0=纯明文 FTP, 1=显式 FTPS, 2=隐式 FTPS)
AT+QFTPCFG="ssltype",1
OK
AT+QFTPCFG="sslctxid",0
OK
```

### 3.2 登录 FTP、文件上传与下载实战

```text
// 步骤 1: 登录 FTP 服务器 (服务器 IP/域名, 端口)
AT+QFTPOPEN="112.25.78.90",21
OK

+QFTPOPEN: 0,0      // URC: 0=操作成功, 0=错误代码(成功)

// 步骤 2: 切换 FTP 工作目录
AT+QFTPCWD="/firmware"
OK
+QFTPCWD: 0,0

// 步骤 3: 从 FTP 服务器下载文件保存到模组 UFS (文件名: "app_v2.bin")
// 语法: AT+QFTPGET="<remote_file>","<local_file>"[,<offset>]
AT+QFTPGET="app_v2.bin","UFS:app_v2.bin",0
OK

+QFTPGET: 0,1048576 // URC: 下载完成，文件大小 1048576 字节 (1 MB)

// 步骤 4: 上传模组本地文件到 FTP 服务器
AT+QFTPPUT="log_2026.txt","UFS:log.txt",0
OK

+QFTPPUT: 0,4096    // URC: 上传完成，大小 4096 字节

// 步骤 5: 列出当前目录文件列表
AT+QFTPLST="."
OK
+QFTPLST: 0,128
-rw-r--r-- 1 ftp ftp 1048576 Sep 04 12:00 app_v2.bin
OK

// 步骤 6: 关闭 FTP 连接
AT+QFTPCLOSE
OK
+QFTPCLOSE: 0,0
```
