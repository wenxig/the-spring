"""QuecPython platform imports kept behind one small boundary."""

import checkNet
import net
import sms
import voiceCall

try:
    import typing
except ImportError:
    _type_checking = False
else:
    _type_checking = typing.TYPE_CHECKING

if _type_checking:
    from type_contracts import CheckNetModule, NetModule, SmsModule, VoiceCallModule


class _CheckNetAdapter:
    def wait_network_connected(self, timeout: int) -> "tuple[int, int]":
        return checkNet.waitNetworkReady(timeout)


class QuecPlatform:
    """Runtime dependencies supplied to application services."""

    def __init__(self) -> None:
        self.check_net: "CheckNetModule" = _CheckNetAdapter()
        self.net: "NetModule" = net
        self.sms: "SmsModule" = sms
        self.voice_call: "VoiceCallModule" = voiceCall
