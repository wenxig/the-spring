#!/usr/bin/env python3
"""Rasterize the checked-in Material weather SVGs into 32x32 1-bit glyphs."""

from __future__ import annotations

import struct
import subprocess
import tempfile
import zlib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIR = ROOT / "assets/material-weather"
OUTPUT = ROOT / "packages/esp32p4/components/ui_core/material_weather_icons.inc"
SIZE = 32
NAMES = ("wb_sunny", "cloud", "grain")


def unfilter(data: bytes, width: int, height: int, channels: int) -> bytes:
    stride = width * channels
    rows: list[bytes] = []
    offset = 0
    previous = bytes(stride)
    for _ in range(height):
        kind = data[offset]
        current = bytearray(data[offset + 1 : offset + 1 + stride])
        offset += stride + 1
        for index in range(stride):
            left = current[index - channels] if index >= channels else 0
            up = previous[index]
            upper_left = previous[index - channels] if index >= channels else 0
            if kind == 1:
                current[index] = (current[index] + left) & 0xFF
            elif kind == 2:
                current[index] = (current[index] + up) & 0xFF
            elif kind == 3:
                current[index] = (current[index] + ((left + up) // 2)) & 0xFF
            elif kind == 4:
                estimate = left + up - upper_left
                distances = (abs(estimate - left), abs(estimate - up), abs(estimate - upper_left))
                current[index] = (current[index] + (left, up, upper_left)[distances.index(min(distances))]) & 0xFF
            elif kind != 0:
                raise ValueError(f"unsupported PNG filter {kind}")
        rows.append(bytes(current))
        previous = bytes(current)
    return b"".join(rows)


def read_png(path: Path) -> tuple[int, int, bytes]:
    raw = path.read_bytes()
    if raw[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"not a PNG: {path}")
    offset = 8
    idat = bytearray()
    width = height = color_type = None
    while offset < len(raw):
        length = struct.unpack(">I", raw[offset : offset + 4])[0]
        kind = raw[offset + 4 : offset + 8]
        payload = raw[offset + 8 : offset + 8 + length]
        offset += 12 + length
        if kind == b"IHDR":
            width, height, depth, color_type, _, _, interlace = struct.unpack(">IIBBBBB", payload)
            if depth != 8 or color_type != 6 or interlace != 0:
                raise ValueError("sips output must be non-interlaced 8-bit RGBA")
        elif kind == b"IDAT":
            idat.extend(payload)
        elif kind == b"IEND":
            break
    if width is None or height is None or color_type is None:
        raise ValueError(f"missing PNG header: {path}")
    return width, height, unfilter(zlib.decompress(idat), width, height, 4)


def rasterize(name: str, directory: Path) -> bytes:
    with tempfile.TemporaryDirectory(prefix="spring-material-") as temporary:
        target = Path(temporary) / f"{name}.png"
        subprocess.run(
            ["/usr/bin/sips", "-s", "format", "png", "-z", str(SIZE), str(SIZE),
             str(directory / f"{name}.svg"), "--out", str(target)],
            check=True,
            stdout=subprocess.DEVNULL,
        )
        width, height, rgba = read_png(target)
    if (width, height) != (SIZE, SIZE):
        raise ValueError(f"unexpected raster size for {name}: {width}x{height}")
    result = bytearray(SIZE * SIZE // 8)
    for y in range(SIZE):
        for x in range(SIZE):
            red, green, blue, alpha = rgba[(y * SIZE + x) * 4 : (y * SIZE + x + 1) * 4]
            if alpha >= 128 and (red + green + blue) < 600:
                result[y * (SIZE // 8) + x // 8] |= 0x80 >> (x % 8)
    return bytes(result)


def main() -> None:
    glyphs = [rasterize(name, SOURCE_DIR) for name in NAMES]
    lines = [
        "#pragma once",
        "#include <cstdint>",
        "namespace spring::ui::detail {",
        "inline constexpr std::uint8_t material_weather_glyphs[3][128] = {",
    ]
    for glyph in glyphs:
        lines.append("  {")
        lines.append("    " + ", ".join(f"0x{value:02x}" for value in glyph) + ",")
        lines.append("  },")
    lines.extend(("};", "}", ""))
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"generated {len(glyphs)} Material weather icons at {SIZE}x{SIZE}")


if __name__ == "__main__":
    main()
