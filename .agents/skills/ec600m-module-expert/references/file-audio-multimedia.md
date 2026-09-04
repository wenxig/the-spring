# EC600M 文件系统与多媒体手册 (FILE, Audio, IMS XML, MUX)

本文档系统总结 Quectel EC600M 系列（含 EC600M-CN 等）模组的内部文件存储系统（UFS/RAM/SD）、音频编解码与播放录音控制、VoLTE 语音通话、IMS XML 解析以及 3GPP 27.010 MUX 串口多路复用协议。

---

## 1. 内部文件存储系统 (AT+QFLST & AT+QF...)

EC600M 模组内部划分了专用 Flash 分区作为用户文件系统（UFS），同时支持高速运行内存文件系统（RAM）。
- **存储介质前缀**：
  - `UFS:`（默认非易失性存储区，掉电不丢失，典型可用空间约 4 MB ~ 16 MB）。
  - `RAM:`（高速易失性存储区，掉电丢失）。

### 1.1 文件存储空间与列表查询
```text
// 查询文件系统总空间与剩余可用空间 (字节)
AT+QFLDS="UFS"
+QFLDS: 8388608,6553600    // 总空间 8MB, 剩余可用 6.25MB
OK

// 列出当前所有文件
AT+QFLST="*"
+QFLST: "cacert.pem",1250
+QFLST: "prompt.wav",45200
+QFLST: "config.json",512
OK
```

### 1.2 文件上传与下载 (主控与模组之间)
常用于向模组下发证书、配置文件或语音播报音频：

```text
// 1. 上传文件到模组 (AT+QFUPL="<filename>",<filesize>[,<timeout>[,<ack>]])
AT+QFUPL="UFS:test.txt",20
CONNECT
<串口直接发送 20 字节数据: "Hello Quectel UFS!!">
OK
+QFUPL: 20,0x4A2B          // 上传成功，返回写入大小及校验和

// 2. 从模组下载文件到主控串口 (AT+QFDWL)
AT+QFDWL="UFS:test.txt"
CONNECT
Hello Quectel UFS!!
OK
+QFDWL: 20,0x4A2B

// 3. 删除文件
AT+QFDEL="UFS:test.txt"
OK

// 4. 计算文件 MD5 校验和
AT+QFMD5="UFS:cacert.pem"
+QFMD5: "7d2b8e3a4f1092a8b9c1d2e3f4a5b6c7"
OK
```

### 1.3 文件句柄级读写 (QFOPEN / QFREAD / QFWRITE)
支持随机访问大文件：
```text
// 打开文件 (0=创建/覆盖写, 1=追加写, 2=只读)
AT+QFOPEN="UFS:log.txt",0
+QFOPEN: 3000              // 返回文件句柄 fd=3000
OK

// 写入数据
AT+QFWRITE=3000,10
CONNECT
0123456789
OK
+QFWRITE: 10,10

// 定位指针与读取
AT+QFSEEK=3000,0,0         // 指针移到文件头
OK
AT+QFREAD=3000,10
CONNECT
0123456789
OK
+QFREAD: 10

// 关闭文件
AT+QFCLOSE=3000
OK
```

---

## 2. 音频控制、TTS 播报与录音功能

EC600M 硬件支持 PCM/I2S 数字音频接口，部分硬件版本支持外接音频 Codec 芯片（如 ES8311、NAU8810 等），内置硬件音频解码器支持 WAV、AMR、MP3 等常见音频格式。

### 2.1 音频通道与音量调节 (AT+QAUDMOD & AT+CLVL)
```text
// 1. 设置音频输出模式 (QAUDMOD)
// 0=Handset(手柄), 1=Headset(耳机), 2=Speaker(喇叭扬声器)
AT+QAUDMOD=2
OK

// 2. 调节喇叭输出音量 (范围 0~100)
AT+CLVL=80
OK
```

### 2.2 本地音频文件播放 (AT+QAUDPLAY)
用于扫码支付音箱、共享设备语音提示等应用：
```text
// 播放 UFS 中的 wav 提示音 (重复播放 1 次)
AT+QAUDPLAY="UFS:prompt.wav",1
OK

// 播放完毕后串口上报 URC:
+QAUDPLAY: 0               // 0 表示自然播放完成

// 随时停止当前播放
AT+QAUDSTOP
OK
```

### 2.3 音频录音控制 (AT+QAUDREC)
```text
// 开始录音并保存为 AMR 文件 (采样格式 1=AMR, 最长持续 30 秒)
AT+QAUDREC=1,"UFS:record.amr",30
OK

// 手动提前停止录音
AT+QAUDREC=0
OK
+QAUDREC: "UFS:record.amr",16384  // 录音结束，大小 16KB
```

---

## 3. VoLTE 语音通话与呼叫控制

Cat 1 bis 模组全面支持基于 IMS 的 VoLTE 高清语音通话。

### 3.1 VoLTE 呼叫基本流程
```text
// 1. 发起语音呼叫
ATD10086;
OK

// 2. 来电振铃与接听
RING
+CLIP: "13800138000",145,"",0,"",0  // 来电显示号码

ATA                         // 接听电话
OK

// 3. 挂断通话
ATH
OK
```

---

## 4. IMS XML 解析器 (AT+QXML)

EC600M 提供轻量级 XML 解析引擎，专门用于解析运营商下发的 IMS / RCS 业务配置或网络返回的复杂 XML 数据。
```text
// 1. 打开 XML 缓冲区
AT+QXMLOPEN="<xml_content_or_file>"
OK

// 2. 根据 XPath / Tag 获取指定节点值
AT+QXMLGET="root/device/id"
+QXMLGET: "EC600M_01"
OK

// 3. 关闭 XML 解析器
AT+QXMLCLOSE
OK
```

---

## 5. 3GPP 27.010 MUX 串口多路复用

3GPP TS 27.010 MUX 协议允许在单条物理串口（如 Main UART）上虚拟出多个逻辑通道（DLC），实现多任务并发（例如 DLC 1 维持 PPP 拨号或 Socket 数据传输，DLC 2 同时发送 AT 指令查询信号强度 CSQ）。

### 5.1 启动 MUX 模式 (AT+CMUX)
```text
// 语法: AT+CMUX=<mode>[,<subset>[,<port_speed>[,<N1>[,<T1>[,<N2>[,<T2>[,<T3>[,<k>]]]]]]]]
// 推荐参数: Basic 模式, 帧长 127 字节
AT+CMUX=0,0,5,127,10,3,30,10,2
OK
```
- 发送此指令并返回 `OK` 后，串口立即切换为 27.010 二进制帧格式。
- **虚拟通道定义**：
  - `DLC 0`：控制信道（用于链路协商与控制报文）。
  - `DLC 1 ~ DLC 3`：通用数据通道（可分别独立充当 AT 命令行与数据透传口）。
