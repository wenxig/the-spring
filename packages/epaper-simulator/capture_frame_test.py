import io
import struct
import tempfile
import unittest
from pathlib import Path

from capture_frame import HEADER, MAGIC, capture


class CaptureFrameTest(unittest.TestCase):
    def test_skips_console_noise_and_writes_pbm(self):
        payload = bytes(range(256)) * 58 + bytes(range(152))
        checksum = 2166136261
        for byte in payload:
            checksum = ((checksum ^ byte) * 16777619) & 0xFFFFFFFF
        packet = HEADER.pack(MAGIC, 1, 42, len(payload), checksum) + payload + struct.pack("<I", MAGIC)
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "frame.pbm"
            self.assertEqual(capture(io.BytesIO(b"log\n" + packet), output), 42)
            result = output.read_bytes()
        self.assertEqual(result[:11], b"P4\n400 300\n")
        self.assertEqual(len(result), 15011)


if __name__ == "__main__":
    unittest.main()
