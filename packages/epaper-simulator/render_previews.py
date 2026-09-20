"""Render XML variants through LVGL and export 1-bit review sheets."""

import argparse
import subprocess
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

VARIANTS = ("reference", "large-time", "weather-focus")
STATES = ("sample", "stress", "fallback", "exam")


def run(*args: str | Path) -> None:
    subprocess.run([str(arg) for arg in args], check=True)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=Path(__file__).parent / "build")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    build = args.build.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    source = Path(__file__).resolve().parent
    font = ImageFont.truetype(str(source.parents[1] / "assets/HYWenHei-65W-3.ttf"), 18)
    sheet = Image.new("1", (1264, 368), 1)
    draw = ImageDraw.Draw(sheet)
    labels = ("A 参考比例", "B 大号时钟", "C 天气增强")
    try:
        for index, variant in enumerate(VARIANTS):
            run("cmake", "-S", source, "-B", build, f"-DUI_LAYOUT_VARIANT={variant}")
            (build / "ui_core/generated/ui_scene.inc").unlink(missing_ok=True)
            run("cmake", "--build", build, "--target", "epaper_simulator", "-j", "8")
            for state in STATES:
                path = output / f"{variant}-{state}"
                run(build / "epaper_simulator", path.with_suffix(".pbm"), state)
                with Image.open(path.with_suffix(".pbm")) as frame:
                    assert frame.size == (400, 300) and frame.mode == "1"
                    frame.save(path.with_suffix(".png"))
                    if state == "sample":
                        x = 16 + index * 416
                        draw.text((x, 10), labels[index], font=font, fill=0)
                        sheet.paste(frame, (x, 42))
                        draw.rectangle((x - 1, 41, x + 400, 342), outline=0)
        sheet.save(output / "comparison.png")
        states = Image.new("1", (832, 684), 1)
        state_draw = ImageDraw.Draw(states)
        for index, state in enumerate(STATES):
            x, y = 8 + index % 2 * 416, 8 + index // 2 * 342
            state_draw.text((x, y), state, font=font, fill=0)
            with Image.open(output / f"reference-{state}.png") as frame:
                states.paste(frame, (x, y + 30))
        states.save(output / "states.png")
    finally:
        run("cmake", "-S", source, "-B", build, "-DUI_LAYOUT_VARIANT=reference")
        run("cmake", "--build", build, "--target", "epaper_simulator", "-j", "8")


if __name__ == "__main__":
    main()
