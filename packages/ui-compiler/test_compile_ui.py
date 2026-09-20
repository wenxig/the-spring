"""Validate XML scene generation and external/embedded asset build modes."""

import subprocess
import sys
from pathlib import Path

import pytest
from compile_ui import compile_screen

REPO = Path(__file__).resolve().parents[2]
XML = REPO / "packages/esp32p4/ui_xml"


def test_clock_expands_all_icon_components():
    scene = compile_screen(XML, "clock")
    assert sum("Kind::icon" in element for element in scene) == 5
    assert any('"snapshot.location"' in element for element in scene)
    assert any('"snapshot.countdown"' in element for element in scene)


@pytest.mark.parametrize("variant", ["reference", "large-time", "weather-focus"])
def test_variants_have_four_equal_forecast_columns(variant):
    scene = compile_screen(XML, "clock", variant)
    icons = [item for item in scene if '"forecast.icon"' in item]
    assert len(icons) == 4
    for slot, x in enumerate((8, 108, 208, 308)):
        assert f".x={x}," in icons[slot]
        assert f".slot={slot}," in icons[slot]
    assert any('"snapshot.month"' in item for item in scene)
    assert any('"snapshot.day"' in item for item in scene)


def test_variant_changes_clock_size():
    reference = compile_screen(XML, "clock")
    large = compile_screen(XML, "clock", "large-time")
    assert reference != large
    assert any(".size=82," in item and '"snapshot.time"' in item for item in large)


def test_external_assets_generate_scene(tmp_path):
    subprocess.run(
        [
            sys.executable,
            str(Path(__file__).with_name("compile_ui.py")),
            str(XML),
            str(tmp_path),
        ],
        check=True,
    )
    assert {file.name for file in tmp_path.iterdir()} == {"ui_scene.inc"}
    assert "status_scene" in (tmp_path / "ui_scene.inc").read_text()


def test_embedded_assets_generate_sources(tmp_path):
    subprocess.run(
        [
            sys.executable,
            str(Path(__file__).with_name("compile_ui.py")),
            str(XML),
            str(tmp_path),
            "--assets",
            str(REPO / "assets/ionicons5-weather"),
        ],
        check=True,
    )
    header = (tmp_path / "ionicons_assets.hpp").read_text()
    assert header.count("extern const char") == 10
    assert "ion5_location_outline" in header


def test_rejects_font_size_outside_cache(tmp_path):
    (tmp_path / "screens").mkdir()
    (tmp_path / "globals.xml").write_text("<globals />")
    (tmp_path / "screens/clock.xml").write_text(
        '<screen><label width="20" height="20" size="97" /></screen>'
    )
    with pytest.raises(ValueError, match="Invalid element dimensions"):
        compile_screen(tmp_path, "clock")
