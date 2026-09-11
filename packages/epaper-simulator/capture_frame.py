#!/usr/bin/env python3
"""Capture binary SPRING framebuffer packets from a board console into PBM."""

import argparse
import struct
import sys


MAGIC = 0x31504653
HEADER = struct.Struct("<I H I I I")
FRAME_BYTES = 15000


def capture(stream, output):
    data = stream.read()
    marker = struct.pack("<I", MAGIC)
    offset = data.find(marker)
    while offset >= 0:
        if len(data) - offset < HEADER.size:
            break
        magic, version, frame_id, length, checksum = HEADER.unpack_from(data, offset)
        end = offset + HEADER.size + length + 4
        if magic == MAGIC and version == 1 and length == FRAME_BYTES and len(data) >= end:
            payload = data[offset + HEADER.size : offset + HEADER.size + length]
            trailer = struct.unpack_from("<I", data, offset + HEADER.size + length)[0]
            actual = 2166136261
            for byte in payload:
                actual = ((actual ^ byte) * 16777619) & 0xFFFFFFFF
            if trailer == MAGIC and actual == checksum:
                with open(output, "wb") as image:
                    image.write(b"P4\n400 300\n")
                    image.write(payload)
                return frame_id
        offset = data.find(marker, offset + 1)
    raise ValueError("no complete valid SPRING frame found")


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
