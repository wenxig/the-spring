"""QuecPython platform imports kept behind one small boundary."""

import checkNet
import net
import sms
import voiceCall


class QuecPlatform:
    """Runtime dependencies supplied to application services."""

    def __init__(self):
        self.check_net = checkNet
        self.net = net
        self.sms = sms
        self.voice_call = voiceCall
