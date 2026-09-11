#!/usr/bin/env python3
"""Capture binary SPRING framebuffer packets from a board console into PBM."""

import argparse
import os
import struct
import sys
import termios
import tty


MAGIC = 0x31504653
HEADER = struct.Struct("<I H I I I")
FRAME_BYTES = 15000


def decode(data, output):
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


def capture(stream, output):
    return decode(stream.read(), output)


def capture_port(port, output, baud):
    fd = os.open(port, os.O_RDONLY | os.O_NOCTTY)
    saved = termios.tcgetattr(fd)
    try:
        tty.setraw(fd)
        speeds = {9600: termios.B9600, 115200: termios.B115200, 230400: termios.B230400}
        if baud not in speeds:
            raise ValueError("baud must be one of 9600, 115200, 230400")
        attrs = termios.tcgetattr(fd)
        attrs[4] = speeds[baud]
        attrs[5] = speeds[baud]
        termios.tcsetattr(fd, termios.TCSANOW, attrs)
        buffer = bytearray()
        marker = struct.pack("<I", MAGIC)
        while True:
            buffer.extend(os.read(fd, 4096))
            start = buffer.find(marker)
            if start > 0:
                del buffer[:start]
            if len(buffer) >= HEADER.size:
                _, _, _, length, _ = HEADER.unpack_from(buffer)
                packet_size = HEADER.size + length + 4
                if length == FRAME_BYTES and len(buffer) >= packet_size:
                    try:
                        return decode(bytes(buffer[:packet_size]), output)
                    except ValueError:
                        del buffer[:4]
    finally:
        termios.tcsetattr(fd, termios.TCSANOW, saved)
        os.close(fd)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("output", help="destination PBM file")
    parser.add_argument("--input", help="capture text file; stdin by default")
    parser.add_argument("--port", help="live USB serial device, for example /dev/cu.usbmodem*")
    parser.add_argument("--baud", type=int, default=115200)
    args = parser.parse_args()
    if args.input and args.port:
        parser.error("--input and --port are mutually exclusive")
    if args.port:
        frame_id = capture_port(args.port, args.output, args.baud)
        print(f"captured frame {frame_id} -> {args.output}")
        return
    source = open(args.input, "rb") if args.input else sys.stdin.buffer
    try:
        frame_id = capture(source, args.output)
    finally:
        if args.input:
            source.close()
    print(f"captured frame {frame_id} -> {args.output}")


if __name__ == "__main__":
    main()
