# QuecPython 短信 (SMS) 与 VoLTE 语音通话 (voiceCall) 开发指南

本指南深入涵盖移远 QuecPython 模组在蜂窝网络下的 **短信系统 (SMS)** 与 **VoLTE 语音通话 (voiceCall)**，包含文本/PDU 收发、来电事件监听、语音通道切换、DTMF 识别与双向通话录音等核心实战。

---

## 1. 短信功能 (sms 模块)

### 1.1 模块机制与存储管理
QuecPython 中的短信支持存储在 SIM 卡 (`"SM"`) 或模组内部存储 (`"ME"`) 中。
- **存储位置统一**：建议保持读取 (`mem1`)、写入 (`mem2`)、接收 (`mem3`) 统一：
  ```python
  import sms

  # 设置统一存储到 SIM 卡或 ME
  sms.setSaveLoc("SM", "SM", "SM")
  # 获取存储信息: ([loc1, used, max], [loc2, used, max], [loc3, used, max])
  status = sms.getSaveLoc()
  print("SMS Storage Status:", status)
  ```
- **短信容量与长短信支持**：
  - UCS2 编码：单条 70 汉字（140 字节）；EC600N/EC200A 最多支持 420 字节，EC600U/EC200U 最多支持 280 字节长短信。
  - GSM 编码：单条 160 字符；长短信最多支持 640~960 字节。

### 1.2 发送短信 (TEXT 与 PDU)
```python
import sms

# 1. 发送中文或混排短信（必须指定 UCS2 编码）
ret = sms.sendTextMsg("13800000000", "【告警】设备温度过高，请及时排查！", "UCS2")
if ret == 0:
    print("UCS2 SMS sent successfully")

# 2. 发送纯英文/数字短信（可使用 GSM 编码）
sms.sendTextMsg("13800000000", "System reboot confirmed: OK", "GSM")

# 3. 发送 PDU 模式短信
sms.sendPduMsg("13800000000", "Alarm triggered via PDU mode", "GSM")
```

### 1.3 接收与读取短信 (事件驱动)
通过回调函数实时感知新短信事件，避免轮询降低功耗与延迟：
```python
import _thread
import sms


def sms_event_handler(args):
    """短信通知回调: args: (sim_id, index, storage)"""
    sim_id = args[0]
    msg_index = args[1]
    storage = args[2]
    print(
        f"[SMS] New message incoming on SIM {sim_id}, Storage: {storage}, Index: {msg_index}"
    )

    # 异步读取避免阻塞回调中断
    _thread.start_new_thread(process_sms, (msg_index,))


def process_sms(index):
    # 方式 1: 直接读取 TEXT 格式 (phoneNumber, msg, msgLen)
    res = sms.searchTextMsg(index)
    if res != -1:
        phone, content, length = res
        print(f"[SMS] From: {phone}, Length: {length}, Content: {content}")

        # 示例业务响应：根据短信指令触发动作
        if "REBOOT" in content.upper():
            print("[SMS CMD] Reboot requested via SMS")

        # 读取后删除短信，防止存储满溢
        sms.deleteMsg(index, 0)
    else:
        # 方式 2: PDU 格式解析
        pdu_hex = sms.searchPduMsg(index)
        if pdu_hex != -1:
            pdu_len = sms.getPduLength(pdu_hex)
            # decodePdu 返回: (phone, msg, timestamp, msgLen)
            decoded = sms.decodePdu(pdu_hex, pdu_len)
            print(f"[SMS PDU Decoded] From: {decoded[0]}, Time: {decoded[2]}, Msg: {decoded[1]}")
            sms.deleteMsg(index, 0)


# 注册全局短信回调
sms.setCallback(sms_event_handler)
```

---

## 2. VoLTE 语音通话 (voiceCall 模块)

> **固件与硬件前提**：
> 1. EC600M / EC600X 需要烧录**支持 VoLTE 固件**（如带 `_VOLTE` 后缀的版本）。
> 2. SIM 卡需开通 **VoLTE** 高清语音功能（纯数据物联网卡无电话功能）。
> 3. 天线性能良好且处于 LTE 覆盖良好区域。

### 2.1 通话生命周期与回调状态码
`voiceCall.setCallback` 是 VoLTE 语音控制的核心，当 `args[0]` >= 10 时为 VoLTE 通话事件：

| 状态码 (`args[0]`) | 含义 | 附加参数解析 |
|---|---|---|
| **10** | 来电通知（振铃） | `args[1]`: 呼叫ID, `args[2]`: 0=主叫/1=被叫, `args[6]`: 对端电话号码 |
| **11** | 通话接通 (Connected) | `args[6]`: 电话号码 |
| **12** | 通话挂断 (Disconnected) | `args[6]`: 电话号码 |
| **13** | 呼叫等待 (Call Waiting) | `args[6]`: 电话号码 |
| **14** | 呼出中 (Dialing) | `args[6]`: 电话号码 |
| **15** | 呼出中，对方振铃 (Alerting) | `args[6]`: 电话号码 |
| **16** | 呼叫保持 (Holding) | `args[6]`: 电话号码 |

### 2.2 呼入与呼出实战
```python
import utime
import voiceCall


def voice_callback(args):
    if not isinstance(args, tuple) and not isinstance(args, list):
        return

    event = args[0]
    if event == 10:
        caller_no = args[6]
        print(f"[VoLTE] Incoming Call From: {caller_no}")
        # 来电自动或条件接听:
        # voiceCall.callAnswer()
    elif event == 11:
        print(f"[VoLTE] Call Connected with: {args[6]}")
    elif event == 12:
        print("[VoLTE] Call Disconnected")
    elif event == 14:
        print(f"[VoLTE] Dialing to: {args[6]}...")
    elif event == 15:
        print("[VoLTE] Remote party alerting (ringing)...")
    else:
        print(f"[VoLTE Event] Event code: {event}, Raw args: {args}")


# 1. 注册通话状态回调
voiceCall.setCallback(voice_callback)

# 2. 设置音频通道 (0=听筒, 1=耳机, 2=喇叭Speaker)
voiceCall.setChannel(2)

# 3. 设置通话音量 (0 ~ 11)
voiceCall.setVolume(9)

# 4. 拨打电话示例
phone_target = "13800138000"
res = voiceCall.callStart(phone_target)
if res == 0:
    print(f"Dialing {phone_target} started...")
```

### 2.3 DTMF 按键检测与发送
在 IVR（交互式语音应答）系统或远程电话遥控场景下，双方通话中可通过 DTMF 进行按键指令交互。

```python
import voiceCall


# 1. DTMF 接收回调
def on_dtmf_received(digit):
    print(f"[DTMF RECV] Remote pressed key: {digit}")
    if digit == "1":
        print("Trigger: Action 1 executed")
    elif digit == "9":
        print("Trigger: Hang up requested")
        voiceCall.callEnd()


# 注册 DTMF 回调并使能检测
voiceCall.dtmfSetCb(on_dtmf_received)
voiceCall.dtmfDetEnable(1)

# 2. DTMF 发送 (仅在通话接通后有效)
# 发送按键 '5'，持续 200ms
# voiceCall.startDtmf('5', 200)
```

### 2.4 通话双向录音与音频流处理
QuecPython 支持将通话音频录制到内部存储或以流数据输出：

```python
import uos
import voiceCall

# 录音配置:
# recordType: 0=AMR, 1=WAV
# recordMode: 0=对端(下行RX), 1=本端(上行TX), 2=混合双向(MIX)

# 方法 A: 自动录音 (通话开始前配置)
# voiceCall.setAutoRecord(1, 0, 2, 'U:/call_record.amr')

# 方法 B: 手动在通话接通后启动录音
# voiceCall.startRecord(0, 2, 'U:/call_active.amr')
# voiceCall.stopRecord()


# 方法 C: 流式回调录音 (边录音边处理/转发)
def record_stream_cb(args):
    # args[0]: 数据, args[1]: 长度, args[2]: 状态
    # 状态: 0=开始, 1=数据包, 2=暂停, 3=结束
    status = args[2]
    if status == 1:
        buf_len = args[1]
        raw_buf = bytearray(buf_len)
        voiceCall.readRecordStream(raw_buf, buf_len)
        # 将 raw_buf 写入文件或通过 socket 发送到服务器
    elif status == 3:
        print("[Record] Stream recording completed")


# voiceCall.startRecordStream(0, 2, record_stream_cb)
```

---

## 3. 短信与 VoLTE 综合守护单例模式 (Production Pattern)

在嵌入式终端中，建议将短消息和电话包装为单独的 TelephonyService 守护线程：

```python
import _thread
import sms
import utime
import voiceCall


class TelephonyManager:

    def __init__(self, admin_phone):
        self.admin_phone = admin_phone
        self.is_in_call = False
        self._init_telephony()

    def _init_telephony(self):
        # 初始化短信存储与回调
        sms.setSaveLoc("SM", "SM", "SM")
        sms.setCallback(self._on_sms_notify)

        # 初始化 VoLTE 回调与音频
        voiceCall.setCallback(self._on_call_notify)
        voiceCall.setChannel(2)
        voiceCall.setVolume(10)

        # 开启 DTMF
        voiceCall.dtmfSetCb(self._on_dtmf)
        voiceCall.dtmfDetEnable(1)

    def _on_sms_notify(self, args):
        msg_idx = args[1]
        _thread.start_new_thread(self._handle_sms, (msg_idx,))

    def _handle_sms(self, index):
        res = sms.searchTextMsg(index)
        if res != -1:
            phone, content, _ = res
            print(f"[Telephony] Incoming SMS from {phone}: {content}")
            if phone == self.admin_phone:
                if "CALL_ME" in content.upper():
                    print("[Telephony] Admin requested callback!")
                    voiceCall.callStart(self.admin_phone)
            sms.deleteMsg(index, 0)

    def _on_call_notify(self, args):
        if not isinstance(args, (list, tuple)):
            return
        event = args[0]
        if event == 10:  # 来电振铃
            caller = args[6]
            print(f"[Telephony] Ringing from {caller}")
            # 仅允许管理员电话自动接听，其余陌生号码拒接
            if caller == self.admin_phone:
                voiceCall.callAnswer()
            else:
                voiceCall.callEnd()
        elif event == 11:
            self.is_in_call = True
            print("[Telephony] Call active")
        elif event == 12:
            self.is_in_call = False
            print("[Telephony] Call ended")

    def _on_dtmf(self, digit):
        print(f"[Telephony DTMF] Key received: {digit}")

    def emergency_broadcast(self, alert_text):
        """发生关键报警时：先发短信，再呼叫管理员"""
        print("[Emergency] Sending alert SMS...")
        sms.sendTextMsg(self.admin_phone, alert_text, "UCS2")
        utime.sleep(2)
        print("[Emergency] Calling Admin...")
        voiceCall.callStart(self.admin_phone)
```
