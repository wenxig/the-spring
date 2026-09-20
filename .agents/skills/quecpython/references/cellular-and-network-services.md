# QuecPython 核心蜂窝通信与网络服务开发指南

本文档全面梳理 QuecPython 官方文档中与蜂窝连接、SIM卡、网络检测、短信、通话等通信核心相关的 API、生命周期机制与工程最佳实践。

---

## 1. 蜂窝网络基础与生命周期 (net & checkNet)

在 QuecPython 嵌入式应用中，开机第一步是确认 SIM 卡就绪并等待蜂窝基站附着与网络注网（CS / PS 域）。

### 1.1 `checkNet` 快速网络状态就绪检测

`checkNet` 是移远官方提供的极简网络自检模块，内部封装了 SIM 识别、网络附着、信号强度与 APN 拨号检测：

```python
import checkNet
import utime

# 等待注网并自动完成数据拨号，最大等待 60 秒
# 成功返回: (3, 1) -> 第一个参数为阶段状态, 第二个参数为拨号结果 (1 为成功)
stage, state = checkNet.wait_network_connected(timeout=60)
if stage == 3 and state == 1:
    print("网络已连接且拨号成功，可启动网络套接字与云服务")
else:
    print(f"注网或拨号超时: stage={stage}, state={state}")
```

### 1.2 `net` 模块网络状态与信号监控

`net` 模块提供低层蜂窝网络状态查询、网络模式设置与信号强度 (CSQ) 回调监听：

```python
import net
import utime

# 1. 查询网络状态
# 返回 (cs_state, ps_state)
# 1: 已注册本地网络; 5: 漫游; 0: 未注册; 2: 正在搜网
cs, ps = net.getState()
print(f"CS 域状态: {cs}, PS 域状态: {ps}")

# 2. 查询信号强度 (CSQ: 0~31, 99 为未知)
csq = net.csqQueryPoll()
print(f"当前蜂窝 CSQ: {csq}")

# 3. 注册网络事件回调
def net_callback(args):
    # args: (event, value)
    event, val = args
    if event == 0:  # CS 状态变化
        print(f"[NET EVENT] CS 状态: {val}")
    elif event == 1:  # PS 状态变化
        print(f"[NET EVENT] PS 状态: {val}")
    elif event == 2:  # 信号质量变化
        print(f"[NET EVENT] CSQ: {val}")

net.setCallback(net_callback)
```

---

## 2. 数据拨号服务 (`dataCall`)

`dataCall` 控制 PDP 上下文（Packet Data Protocol Context）激活与 APN 参数配置，支持多路 PDP 实例。

### 2.1 PDP 激活与 IP 信息查询

```python
import dataCall

# 配置第 1 路 PDP 的 APN (profileIdx=1, ipType=0 (IPv4/IPv6), apn="ctnet", user="", pwd="", authType=0)
# 针对不同运营商，默认大部分国内 SIM 卡直接留空即可自动读取
# dataCall.setApn(1, 0, "ctnet", "", "", 0)

# 查询第 1 路 PDP 激活状态及网络配置
# 返回格式: (profileIdx, [ipType, ip, dns1, dns2, gateway])
pdp_info = dataCall.getInfo(1)
print(f"PDP-1 信息: {pdp_info}")

# 手动激活与断开
# dataCall.start(1, 0, "ctnet", "", "", 0)
# dataCall.stop(1)
```

### 2.2 拨号状态异步回调监听与自动重连

```python
import dataCall

def pdp_callback(args):
    # args 为 (profileIdx, nw_err, state)
    # nw_err: 0 表示无错误; state: 1 为激活/连接, 0 为断开
    profile, err, state = args
    print(f"PDP 回调: profile={profile}, err={err}, state={state}")
    if state == 0:
        print("警告: 4G 网络数据连接掉线，触发重新拨号流程")
        # 执行重连或状态机重置

dataCall.setCallback(pdp_callback)
```

---

## 3. SIM 卡管理 (`sim`)

`sim` 模块用于检测 SIM 卡插入状态、查询 IMSI / ICCID / 电话号码，以及 PIN 码管理。

```python
import sim

# 1. 检测 SIM 状态 (1: 就绪, 0: 未插卡或锁卡)
status = sim.getStatus()
print(f"SIM 卡状态: {status}")

# 2. 查询 ICCID (卡体序列号) 与 IMSI (国际移动用户识别码)
iccid = sim.getIccid()
imsi = sim.getImsi()
imei = sim.getImei()  # 注意: 部分固件通过 modem.getDevImei() 获取
print(f"ICCID: {iccid}")
print(f"IMSI: {imsi}")

# 3. 查询本机号码 (取决于运营商 SIM 卡内是否写入 MSISDN)
phone_num = sim.getPhoneNumber()
print(f"本机号码: {phone_num}")
```

---

## 4. 短信收发 (`sms`)

短信功能常用于设备远程激活、告警短信发送以及配置下发。

### 4.1 发送与读取短信

```python
import sms

# 1. 注册短信到达监听
def sms_cb(args):
    # args: (index, msg_storage)
    idx, storage = args
    print(f"收到新短信，位置: {idx}, 存储区: {storage}")
    # 读取短信
    msg = sms.searchText(idx)
    # msg: (status, phone_number, timestamp, content)
    print(f"发件人: {msg[1]}, 内容: {msg[3]}")
    # 读后即删保持存储区可用
    sms.deleteMsg(idx)

sms.setCallback(sms_cb)

# 2. 发送短信 (目标号码, 内容, 编码格式: 1=GSM 7-bit, 2=UCS2 中文)
# 发送中文短信示例 (QuecPython 自动处理 UCS2 编码转换)
ret = sms.sendText("10086", "CXLL", "GSM")
print(f"短信发送结果: {ret}")
```

---

## 5. 蜂窝网络基站定位 (`cellLocator`)

QuecPython 支持无需物理 GPS/GNSS 天线，仅凭借基站信号实现基站定位（LBS）及 Wi-Fi 扫描定位：

```python
import cellLocator
import checkNet

# 需确保注网就绪
checkNet.wait_network_connected(30)

# 使用移远服务器或配置的 token 获取经纬度
# cellLocator.perform(server_url, token)
# 返回 (lat, lng, accuracy)
```
