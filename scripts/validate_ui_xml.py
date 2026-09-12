#!/usr/bin/env python3
"""Validate the declarative LVGL UI source files without requiring LVGL Pro."""

from pathlib import Path
import sys
import xml.etree.ElementTree as ET


def main() -> int:
    root = Path(__file__).resolve().parents[1] / "packages" / "esp32p4" / "ui_xml"
    files = sorted(root.rglob("*.xml"))
    if not files:
        print("no XML UI files found", file=sys.stderr)
        return 1
    for path in files:
        try:
            document = ET.parse(path)
        except ET.ParseError as error:
            print(f"{path}: {error}", file=sys.stderr)
            return 1
        if document.getroot().tag not in {"globals", "screen", "component"}:
            print(f"{path}: root must be globals, screen, or component", file=sys.stderr)
            return 1
    print(f"validated {len(files)} declarative LVGL XML files")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
