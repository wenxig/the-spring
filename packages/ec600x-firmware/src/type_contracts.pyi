"""Type contracts for QuecPython platform modules and callbacks."""

from collections.abc import Callable
from typing import Protocol

__all__ = [
    "CallCallback",
    "CallbackArgs",
    "CellLocatorModule",
    "CheckNetModule",
    "NetModule",
    "Platform",
    "SmsCallback",
    "SmsModule",
    "VoiceCallModule",
]

CallbackArgs = tuple[int, ...]
SmsCallback = Callable[[str, str], None]
CallCallback = Callable[[int, str, object], None]

class CheckNetModule(Protocol):
    def wait_network_connected(self, timeout: int) -> tuple[int, int]: ...

class NetModule(Protocol):
    def csqQueryPoll(self) -> int: ...
    def getCellInfo(self) -> object | int: ...

class SmsModule(Protocol):
    def setSaveLoc(self, storage1: str, storage2: str, storage3: str) -> int: ...
    def setCallback(self, callback: Callable[[CallbackArgs], None]) -> int: ...
    def searchTextMsg(self, index: int) -> tuple[str, str, int] | int | None: ...
    def searchPduMsg(self, index: int) -> str | bytes | int | None: ...
    def getPduLength(self, pdu: str | bytes) -> int: ...
    def decodePdu(self, pdu: str | bytes, length: int) -> tuple[str, str]: ...
    def deleteMsg(self, index: int, flag: int) -> int: ...
    def sendTextMsg(self, phone_number: str, message: str, encoding: str) -> int: ...

class VoiceCallModule(Protocol):
    def setCallback(self, callback: Callable[[CallbackArgs], None]) -> int: ...
    def callStart(self, phone_number: str) -> int: ...
    def callAnswer(self) -> int: ...
    def callEnd(self) -> int: ...
    def setVolume(self, volume: int) -> int: ...
    def setChannel(self, channel: int) -> int: ...

class CellLocatorModule(Protocol):
    def getLocation(
        self,
        server_addr: str,
        port: int,
        token: str,
        timeout: int,
        profile_idx: int,
    ) -> tuple[float, float, int]: ...

class Platform(Protocol):
    check_net: CheckNetModule
    net: NetModule
    sms: SmsModule
    voice_call: VoiceCallModule
    cell_locator: CellLocatorModule
