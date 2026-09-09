from collections.abc import Callable
from typing import TYPE_CHECKING

from services.location import LocationService
from services.network import NetworkService
from services.sms import SmsService
from services.volte import VolteService

if TYPE_CHECKING:
    from type_contracts import (
        CallbackArgs,
        CellLocatorModule,
        CheckNetModule,
        NetModule,
        SmsModule,
        VoiceCallModule,
    )


class FakeCheckNet:
    def __init__(self, result: tuple[int, int]) -> None:
        self.result = result

    def wait_network_connected(self, timeout: int) -> tuple[int, int]:
        del timeout
        return self.result


class FakeNet:
    def csqQueryPoll(self) -> int:
        return 23

    def getCellInfo(self) -> object | int:
        return {"registered": True}


class FakeSms:
    def __init__(self) -> None:
        self.callback: Callable[[CallbackArgs], None] | None = None
        self.sent: tuple[str, str, str] | None = None

    def setSaveLoc(self, storage1: str, storage2: str, storage3: str) -> int:
        del storage1, storage2, storage3
        return 0

    def setCallback(self, callback: Callable[["CallbackArgs"], None]) -> int:
        self.callback = callback
        return 0

    def searchTextMsg(self, index: int) -> tuple[str, str, int] | int | None:
        del index
        return None

    def searchPduMsg(self, index: int) -> str | bytes | int | None:
        del index
        return None

    def getPduLength(self, pdu: str | bytes) -> int:
        return len(pdu)

    def decodePdu(self, pdu: str | bytes, length: int) -> tuple[str, str]:
        del pdu, length
        return "", ""

    def deleteMsg(self, index: int, flag: int) -> int:
        del index, flag
        return 0

    def sendTextMsg(self, phone_number: str, message: str, encoding: str) -> int:
        self.sent = (phone_number, message, encoding)
        return 0


class FakeVoiceCall:
    def __init__(self) -> None:
        self.callback: Callable[[CallbackArgs], None] | None = None

    def setCallback(self, callback: Callable[["CallbackArgs"], None]) -> int:
        self.callback = callback
        return 0

    def callStart(self, phone_number: str) -> int:
        del phone_number
        return 0

    def callAnswer(self) -> int:
        return 0

    def callEnd(self) -> int:
        return 0

    def setVolume(self, volume: int) -> int:
        del volume
        return 0

    def setChannel(self, channel: int) -> int:
        del channel
        return 0


class FakeCellLocator:
    def getLocation(
        self,
        server_addr: str,
        port: int,
        token: str,
        timeout: int,
        profile_idx: int,
    ) -> tuple[float, float, int]:
        del server_addr, port, token, timeout, profile_idx
        return (39.908823, 116.397470, 550)


class FakePlatform:
    def __init__(
        self,
        check_net: "CheckNetModule | None" = None,
        sms: "SmsModule | None" = None,
        voice_call: "VoiceCallModule | None" = None,
        cell_locator: "CellLocatorModule | None" = None,
    ) -> None:
        self.check_net: CheckNetModule = check_net or FakeCheckNet((3, 1))
        self.net: NetModule = FakeNet()
        self.sms: SmsModule = sms or FakeSms()
        self.voice_call: VoiceCallModule = voice_call or FakeVoiceCall()
        self.cell_locator: CellLocatorModule = cell_locator or FakeCellLocator()


def test_network_service_reports_connection_and_signal() -> None:
    service = NetworkService(FakePlatform())

    assert service.wait_connected(timeout_sec=5) is True
    assert service.is_connected() is True
    assert service.get_signal_csq() == 23
    assert service.get_cell_info() == {"registered": True}


def test_sms_service_sends_using_injected_module() -> None:
    fake_sms = FakeSms()
    service = SmsService(FakePlatform(sms=fake_sms))

    assert service.send_text("10086", "hello", "GSM") is True
    assert fake_sms.sent == ("10086", "hello", "GSM")


def test_volte_service_dispatches_events_and_commands() -> None:
    events: list[tuple[int, str, object]] = []
    fake_voice = FakeVoiceCall()

    def record_event(event: int, phone: str, call_id: object) -> None:
        events.append((event, phone, call_id))

    service = VolteService(FakePlatform(voice_call=fake_voice), event_cb=record_event)

    assert service.call("10086") is True
    assert service.answer() is True
    assert service.hangup() is True
    assert fake_voice.callback is not None
    fake_voice.callback((11, 2, 0, 0, 0, 0, "10086"))

    assert events == [(11, "10086", 2)]


def test_location_service_returns_coordinates() -> None:
    fake_locator = FakeCellLocator()
    service = LocationService(fake_locator)

    result = service.get_position(timeout=10)

    assert result is not None
    lat, lng, accuracy = result
    assert lat == 39.908823
    assert lng == 116.397470
    assert accuracy == 550
