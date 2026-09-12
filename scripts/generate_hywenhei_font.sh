#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
input=$(mktemp)
script=$(mktemp).swift
trap 'rm -f "$input" "$script"' EXIT

python3 - "$root/packages/esp32p4/components/ui_core/cjk_font.inc" "$input" <<'PY'
import re, sys
text = open(sys.argv[1], encoding="utf-8").read()
values = re.search(r"cjk_codepoints\[\] = \{([^}]*)\}", text, re.S).group(1)
open(sys.argv[2], "w", encoding="utf-8").write(" ".join(re.findall(r"\d+", values)))
PY

sed 's/CTFontCreateWithName("STHeiti" as CFString, 16, nil)/CTFontCreateWithName("HYWenHei-65W" as CFString, 16, nil)/' \
  /tmp/generate_cjk.swift > "$script"
swift "$script" "$input" "$root/packages/esp32p4/components/ui_core/cjk_font.inc"
