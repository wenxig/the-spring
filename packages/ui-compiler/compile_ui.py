"""Compile the project's XML component schema into LVGL scene descriptions."""

import argparse
import json
import xml.etree.ElementTree as ET
from pathlib import Path


def compile_screen(root: Path, name: str) -> list[str]:
    constants = {
        e.attrib["name"]: e.attrib["value"]
        for e in ET.parse(root / "globals.xml").getroot()
    }
    result = []

    def number(value: str) -> int:
        return int(constants[value[1:]] if value.startswith("#") else value)

    def visit(node, x=0, y=0, slot=-1, stack=()):
        attrs = node.attrib
        x += number(attrs.get("x", "0"))
        y += number(attrs.get("y", "0"))
        slot = int(attrs.get("slot", slot))
        if node.tag.lower() == "use" or (
            node.tag.lower() == "component" and "ref" in attrs
        ):
            component = attrs.get("component", attrs.get("ref"))
            if component in stack:
                raise ValueError(f"Recursive component: {component}")
            visit(
                ET.parse(root / "components" / f"{component}.xml").getroot(),
                x,
                y,
                slot,
                (*stack, component),
            )
            return
        if node.tag.lower() in ("screen", "component", "view"):
            for child in node:
                visit(child, x, y, slot, stack)
            return
        tag = node.tag.lower()
        if tag in ("lv_label", "lv_obj", "lv_image"):
            tag = {"lv_label": "label", "lv_obj": "rule", "lv_image": "icon"}[tag]
        if tag not in ("label", "rule", "icon"):
            if tag == "component":
                return
            raise ValueError(f"Unsupported element: {node.tag}")
        allowed = {
            "name",
            "ref",
            "x",
            "y",
            "width",
            "height",
            "size",
            "font_size",
            "bind",
            "text",
            "align",
            "layout",
            "visual_reference",
            "slot",
        }
        if set(attrs) - allowed:
            raise ValueError(f"Unsupported attributes: {set(attrs) - allowed}")
        width = number(attrs.get("width", "1"))
        height = number(attrs.get("height", "1"))
        size = number(attrs.get("size", attrs.get("font_size", "14")))
        if not (0 <= x < 400 and 0 <= y < 300 and 0 < width <= 400 - x):
            raise ValueError(f"Element outside display: {attrs}")
        if not (0 < height <= 300 - y and 8 <= size <= 96):
            raise ValueError(f"Invalid element dimensions: {attrs}")
        text = json.dumps(attrs.get("text", ""), ensure_ascii=False)
        binding = json.dumps(attrs.get("bind", ""))
        centered = "true" if attrs.get("align") == "center" else "false"
        result.append(
            f"  {{.kind=Kind::{tag}, .x={x}, .y={y}, .width={width}, .height={height}, "
            f".size={size}, .slot={slot}, .centered={centered}, .text={text}, .binding={binding}}},"
        )

    visit(ET.parse(root / "screens" / f"{name}.xml").getroot())
    return result


def emit_svg_assets(asset_dir: Path, output: Path) -> None:
    names = [
        "sunny-outline",
        "partly-sunny-outline",
        "cloudy-outline",
        "rainy-outline",
        "thunderstorm-outline",
        "snow-outline",
        "moon-outline",
        "cloudy-night-outline",
        "help-circle-outline",
        "location-outline",
    ]
    declarations: list[str] = []
    definitions: list[str] = [
        '#include "ionicons_assets.hpp"',
        "namespace spring::ui {",
    ]
    for name in names:
        symbol = "ion5_" + name.replace("-", "_")
        data = (asset_dir / f"{name}.svg").read_text(encoding="utf-8")
        declarations.append(f"extern const char {symbol}[];")
        definitions.append(f'const char {symbol}[] = R"SVG({data})SVG";')
    definitions.append("}")
    (output / "ionicons_assets.hpp").write_text(
        "#pragma once\nnamespace spring::ui {\n" + "\n".join(declarations) + "\n}\n",
        encoding="utf-8",
    )
    (output / "ionicons_assets.cpp").write_text(
        "\n".join(definitions) + "\n", encoding="utf-8"
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("xml", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--font", type=Path)
    parser.add_argument("--assets", type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    scenes = ["// Generated from ui_xml.\n"]
    for screen in ("clock", "status"):
        scenes.append(f"constexpr Element {screen}_scene[] = {{\n")
        scenes.extend(line + "\n" for line in compile_screen(args.xml, screen))
        scenes.append("};\n")
    (args.output / "ui_scene.inc").write_text("".join(scenes))
    if args.assets is not None:
        emit_svg_assets(args.assets, args.output)
    if args.font is not None:
        data = args.font.read_bytes()
        with (args.output / "font_asset.cpp").open("w") as output:
            output.write('#include "font_asset.hpp"\nnamespace spring::ui {\n')
            output.write("alignas(4) const unsigned char default_font_data[] = {\n")
            for offset in range(0, len(data), 32):
                output.write(
                    ",".join(str(v) for v in data[offset : offset + 32]) + ",\n"
                )
            output.write(
                f"}};\nconst unsigned int default_font_size = {len(data)};\n}}\n"
            )


if __name__ == "__main__":
    main()
