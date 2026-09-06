"""
蜂窝网络管理服务
负责网络注册检查、信号强度监测与重连处理
"""

try:
    import typing
except ImportError:
    _type_checking = False
else:
    _type_checking = typing.TYPE_CHECKING

if _type_checking:
    from type_contracts import Platform


class NetworkService:
    def __init__(self, platform: "Platform") -> None:
        self._check_net = platform.check_net
        self._net = platform.net
        self._connected = False

    def wait_connected(self, timeout_sec: int = 60) -> bool:
        """阻塞等待基站注网与数据通道激活"""
        print("[Net] Waiting for cellular network...")
        stage, state = self._check_net.wait_network_connected(timeout_sec)
        self._connected = stage == 3 and state == 1
        if self._connected:
            print("[Net] Cellular network ready.")
        else:
            print("[Net] Network timeout: stage={}, state={}".format(stage, state))
        return self._connected

    def is_connected(self) -> bool:
        return self._connected

    def get_signal_csq(self) -> int:
        """获取当前信号强度 CSQ (0-31, 99 表示未知)"""
        try:
            csq = self._net.csqQueryPoll()
            return csq
        except Exception as e:
            print("[Net] CSQ query error:", e)
            return -1

    def get_cell_info(self) -> "object | None":
        """获取基站信号与网络注册信息"""
        try:
            return self._net.getCellInfo()
        except Exception as e:
            print("[Net] Cell info query error:", e)
            return None
