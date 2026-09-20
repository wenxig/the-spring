# QuecPython 物联网协议与云平台接入指南

本文档全面梳理 QuecPython `usocket`、`request`、`umqtt`、`cloudlib`（阿里云、腾讯云等）以及 TLS/SSL 安全连接的工程实现方案。

---

## 1. 基础网络通信：TCP/UDP 套接字 (`usocket` & `ussl`)

### 1.1 标准 TCP 客户端连接

```python
import usocket
import checkNet

# 1. 确保注网成功
checkNet.wait_network_connected(30)

# 2. 创建 TCP Socket
s = usocket.socket(usocket.AF_INET, usocket.SOCK_STREAM)
s.settimeout(10.0)

try:
    # 3. 解析域名并连接
    addr = usocket.getaddrinfo("echo.websocket.events", 80)[0][-1]
    s.connect(addr)
    print("TCP 连接建立成功")

    # 4. 发送数据
    s.send(b"GET / HTTP/1.1\r\nHost: echo.websocket.events\r\nConnection: close\r\n\r\n")

    # 5. 接收数据
    response = bytearray()
    while True:
        chunk = s.recv(512)
        if not chunk:
            break
        response.extend(chunk)
    print(f"收到响应: {response[:100]}...")

finally:
    s.close()
```

### 1.2 加密 TLS/SSL Socket (`ussl`)

```python
import usocket
import ussl

sock = usocket.socket(usocket.AF_INET, usocket.SOCK_STREAM)
sock.connect(("cloud.tencent.com", 443))

# 包装为 SSL 套接字
ssl_sock = ussl.wrap_socket(sock, server_side=False)
ssl_sock.write(b"GET / HTTP/1.1\r\nHost: cloud.tencent.com\r\nConnection: close\r\n\r\n")
data = ssl_sock.read(1024)
print(f"SSL 响应: {data[:128]}")
ssl_sock.close()
```

---

## 2. HTTP/HTTPS 高级请求库 (`request`)

QuecPython 内置类似 Python `requests` 的轻量封装模块 `request`：

```python
import request
import checkNet

checkNet.wait_network_connected(30)

# GET 请求示例
res = request.get("https://httpbin.org/get", headers={"User-Agent": "QuecPython-Client"})
print(f"状态码: {res.status_code}")
print(f"返回 JSON: {res.json()}")

# POST JSON 请求示例
payload = {"dev_id": "EC600M_001", "temperature": 25.4, "humidity": 60.2}
res_post = request.post(
    "https://httpbin.org/post",
    json=payload,
    headers={"Content-Type": "application/json"}
)
print(f"POST 返回状态: {res_post.status_code}")
print(f"服务器回复: {res_post.text}")
```

---

## 3. MQTT 通信协议与掉线重连机制 (`umqtt`)

MQTT 是蜂窝物联网设备最核心的通信协议，QuecPython 提供稳定高效的 `MQTTClient`。

### 3.1 工业级 MQTT 客户端封装模板

```python
from umqtt import MQTTClient
import checkNet
import utime
import ujson

class QuecMQTTManager:
    def __init__(self, client_id, server, port=1883, user=None, password=None):
        self.client_id = client_id
        self.server = server
        self.port = port
        self.user = user
        self.password = password
        self.client = None
        self.is_connected = False

    def on_message(self, topic, msg):
        topic_str = topic.decode()
        msg_str = msg.decode()
        print(f"[MQTT RECV] Topic: {topic_str}, Message: {msg_str}")
        try:
            payload = ujson.loads(msg_str)
            # 处理业务指令
        except Exception as e:
            print(f"解析 JSON 消息失败: {e}")

    def connect(self):
        checkNet.wait_network_connected(30)
        self.client = MQTTClient(
            client_id=self.client_id,
            server=self.server,
            port=self.port,
            user=self.user,
            password=self.password,
            keepalive=60
        )
        self.client.set_callback(self.on_message)
        
        try:
            self.client.connect()
            self.is_connected = True
            print(f"MQTT Broker 连接成功: {self.server}:{self.port}")
            # 订阅主题
            sub_topic = f"device/{self.client_id}/command"
            self.client.subscribe(sub_topic.encode(), qos=1)
            print(f"成功订阅: {sub_topic}")
        except Exception as e:
            self.is_connected = False
            print(f"MQTT 连接失败: {e}")
            raise e

    def publish(self, topic, data, qos=0):
        if not self.is_connected or not self.client:
            raise RuntimeError("MQTT 未连接")
        payload = ujson.dumps(data) if isinstance(data, dict) else str(data)
        self.client.publish(topic.encode(), payload.encode(), qos=qos)
        print(f"[MQTT PUB] {topic} -> {payload}")

    def loop(self):
        """定期调用处理接收缓冲区，支持无阻塞或有消息检测"""
        if self.client and self.is_connected:
            try:
                self.client.check_msg()
            except Exception as e:
                print(f"MQTT 循环检测异常: {e}, 尝试重连...")
                self.is_connected = False
                self.reconnect()

    def reconnect(self):
        while not self.is_connected:
            try:
                print("正在尝试重连 MQTT Broker...")
                self.connect()
            except Exception:
                utime.sleep(5)
```

---

## 4. 主流物联网云平台接入 (`aLiYun` & `TXyun`)

QuecPython 在 `cloudlib` 中为国内主流云厂商提供了深度优化、集成一机一密 / 一型一密认证协议的直连 SDK。

### 4.1 阿里云物联网平台接入 (`aLiYun`)

```python
import aLiYun
import checkNet
import utime

# 等待注网
checkNet.wait_network_connected(30)

# 设备三元组参数
product_key = "a1XXXXXXXXX"
device_name = "device_ec600m_01"
device_secret = "yyyyyyyyyyyyyyyyyyyyyyyy"
product_secret = None  # 一型一密时提供

# 回调函数
def aliyun_sub_cb(topic, msg):
    print(f"收到阿里云下发消息: Topic={topic.decode()}, Msg={msg.decode()}")

# 创建阿里云实例
ali = aLiYun(product_key, product_secret, device_name, device_secret)
ali.set_callback(aliyun_sub_cb)

# 发起认证连接
ali.start()
print("阿里云 IoT 平台连接就绪")

# 上报物模型属性或普通主题
topic_pub = f"/sys/{product_key}/{device_name}/thing/event/property/post"
ali.publish(topic_pub, '{"params": {"temperature": 26.5}}')
```

### 4.2 腾讯云 IoT Explorer 接入 (`TXyun`)

```python
import TXyun
import checkNet

checkNet.wait_network_connected(30)

product_id = "PRODUCT_ID"
device_name = "dev_01"
device_secret = "SECRET_KEY"

def tx_cb(topic, msg):
    print(f"腾讯云收到下行消息: {msg}")

tx = TXyun(product_id, device_name, device_secret)
tx.set_callback(tx_cb)
tx.start()
```
