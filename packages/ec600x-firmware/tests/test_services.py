from types import SimpleNamespace

from services.network import NetworkService
from services.sms import SmsService
from services.volte import VolteService


class FakeCheckNet:
    def __init__(self, result):
        self.result = result

    def wait_network_connected(self, timeout):
        return self.result


class FakeNet:
    def csqQueryPoll(self):
        return 23

    def getCellInfo(self):
        return {"registered": True}


class FakeSms:
    def __init__(self):
        self.callback = None
        self.sent = None

    def setSaveLoc(self, *_args):
        pass

    def setCallback(self, callback):
        self.callback = callback

    def sendTextMsg(self, phone, message, encoding):
        self.sent = (phone, message, encoding)
        return 0


class FakeVoiceCall:
    def __init__(self):
        self.callback = None

    def setCallback(self, callback):
        self.callback = callback

    def callStart(self, _phone):
        return 0

    def callAnswer(self):
        return 0

    def callEnd(self):
        return 0

    def setVolume(self, _volume):
        pass

    def setChannel(self, _channel):
        pass


def platform(check_net=None, sms=None, voice_call=None):
    return SimpleNamespace(
        check_net=check_net or FakeCheckNet((3, 1)),
        net=FakeNet(),
        sms=sms or FakeSms(),
        voice_call=voice_call or FakeVoiceCall(),
    )


def test_network_service_reports_connection_and_signal():
    service = NetworkService(platform())

    assert service.wait_connected(timeout_sec=5) is True
    assert service.is_connected() is True
    assert service.get_signal_csq() == 23
    assert service.get_cell_info() == {"registered": True}


def test_sms_service_sends_using_injected_module():
    fake_sms = FakeSms()
    service = SmsService(platform(sms=fake_sms))

    assert service.send_text("10086", "hello", "GSM") is True
    assert fake_sms.sent == ("10086", "hello", "GSM")


def test_volte_service_dispatches_events_and_commands():
    events = []
    fake_voice = FakeVoiceCall()
    service = VolteService(platform(voice_call=fake_voice), event_cb=lambda *event: events.append(event))

    assert service.call("10086") is True
    assert service.answer() is True
    assert service.hangup() is True
    fake_voice.callback((11, 2, 0, 0, 0, 0, "10086"))

    assert events == [(11, "10086", 2)]
