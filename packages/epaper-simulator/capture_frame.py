#!/usr/bin/env python3
"""Capture SPRING_FRAME_BEGIN/END output from a board console into PBM."""

import argparse
import binascii
import re
import sys


FRAME_RE = re.compile(r"^SPRING_FRAME_BEGIN (\d+) (\d+) ([0-9a-fA-F]{8})$")
END_RE = re.compile(r"^SPRING_FRAME_END (\d+)$")


def capture(stream, output):
    pending = None
    for raw in stream:
        line = raw.decode("ascii", errors="ignore").strip()
        match = FRAME_RE.fullmatch(line)
        if match:
            pending = (int(match.group(1)), int(match.group(2)), match.group(3).lower(), "")
            continue
        if pending is None:
            continue
        end = END_RE.fullmatch(line)
        if end:
            frame_id, length, checksum, encoded = pending
            if int(end.group(1)) != frame_id:
                pending = None
                continue
            payload = binascii.unhexlify(encoded)
            if len(payload) != length or len(payload) != 15000:
                raise ValueError(f"frame {frame_id}: payload length is {len(payload)}, expected {length}")
            actual = 2166136261
            for byte in payload:
                actual = ((actual ^ byte) * 16777619) & 0xFFFFFFFF
            if f"{actual:08x}" != checksum:
                raise ValueError(f"frame {frame_id}: checksum mismatch")
            with open(output, "wb") as image:
                image.write(b"P4\n400 300\n")
                image.write(payload)
            return frame_id
        if re.fullmatch(r"[0-9a-fA-F]+", line):
            pending = (*pending[:3], pending[3] + line)
    raise ValueError("no complete SPRING frame found")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("output", help="destination PBM file")
    parser.add_argument("--input", help="capture text file; stdin by default")
    args = parser.parse_args()
    source = open(args.input, "rb") if args.input else sys.stdin.buffer
    try:
        frame_id = capture(source, args.output)
    finally:
        if args.input:
            source.close()
    print(f"captured frame {frame_id} -> {args.output}")


if __name__ == "__main__":
    main()
