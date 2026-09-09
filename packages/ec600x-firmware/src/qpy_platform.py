"""QuecPython platform imports kept behind one small boundary."""

import cellLocator  # type: ignore[import-untyped]
import checkNet  # type: ignore[import-untyped]
import net  # type: ignore[import-untyped]
import sms  # type: ignore[import-untyped]
import voiceCall  # type: ignore[import-untyped]

try:
    import typing
except ImportError:
    _type_checking = False
else:
    _type_checking = typing.TYPE_CHECKING

if _type_checking:
    from type_contracts import (
        CellLocatorModule,
        CheckNetModule,
        NetModule,
        SmsModule,
        VoiceCallModule,
    )


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
        self.cell_locator: "CellLocatorModule" = cellLocator  # type: ignore[assignment]
