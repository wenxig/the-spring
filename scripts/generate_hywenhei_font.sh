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

cp "$root/scripts/generate_cjk_font.swift" "$script"
swift "$script" "$input" "$root/assets/HYWenHei-65W-3.ttf" \
  "$root/packages/esp32p4/components/ui_core/cjk_font.inc"
