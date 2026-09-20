"""Artifact checks; FreeCAD also reopens exported solids independently."""

import json
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent
REPORT = json.loads((ROOT / "output/validation.json").read_text())


def test_printable_parts_and_assembly():
    for part in REPORT["parts"]:
        assert part["is_valid"] and part["solids"] == 1
    for part in REPORT["print_parts"]:
        assert part["is_solid_mesh"]
        assert abs(part["bbox_mm"]["zmin"]) < 1e-5
    assembly = REPORT["assembly"]
    for name, value in assembly.items():
        if name.endswith("intersection_mm3"):
            for volume in value if isinstance(value, list) else [value]:
                assert abs(volume) < 1e-5, name
    for group, key in [("front_insert_seat_material_mm3", "remaining_material_mm3"),
                       ("back_cover_m2_axis_material_mm3", "remaining_material_mm3"),
                       ("devboard_mounts", "insert_seat_material_mm3")]:
        assert all(abs(item[key]) < 1e-5 for item in assembly[group])


def test_board_can_be_inserted_and_retained():
    result = REPORT["assembly"]["ec600x_insertion"]
    assert abs(result["rear_entry_sweep_intersection_mm3"]) < 1e-5
    assert abs(result["descent_sweep_intersection_mm3"]) < 1e-5
    assert result["roof_clearance_mm"] > 0
    assert result["latched_lift_contact_mm3"] > 0
    assert REPORT["assembly"]["ec600x_esp32_pcb_gap_mm"] > 0


def test_reopen_freecad_step_and_meshes():
    executable = Path("/Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd")
    if not executable.exists():
        pytest.skip("FreeCAD CLI required for native artifact checks")
    code = """
import FreeCAD as A, Part, Mesh
d = A.openDocument('output/enclosure_130x60x70.FCStd')
d.recompute()
assert all('Invalid' not in o.State for o in d.Objects)
front, cover = d.FrontShell.Shape, d.BackCover.Shape
assert all(s.isValid() and len(s.Solids) == 1 for s in [front, cover])
assert front.common(cover).Volume < 1e-5
ec, esp = d.EC600XEVBReference.Shape, d.ESP32P4DevKitReference.Shape
assert all(ec.common(s).Volume < 1e-5 for s in [front, cover, esp])
step = Part.read('output/enclosure_130x60x70.step')
assert step.isValid() and len(step.Solids) == 2
assert abs(step.Volume - front.Volume - cover.Volume) < 1e-3
for solid in step.Solids:
    assert min(solid.cut(s).Volume + s.cut(solid).Volume for s in [front, cover]) < 1e-3
for name in ['front_shell_print', 'back_cover_print']:
    mesh = Mesh.Mesh('output/' + name + '.stl')
    assert mesh.isSolid() and abs(mesh.BoundBox.ZMin) < 1e-5
A.closeDocument(d.Name)
print('ENCLOSURE_ARTIFACTS_VERIFIED')
"""
    result = subprocess.run([str(executable), "-c", code], cwd=ROOT, text=True,
                            capture_output=True, timeout=120, check=False)
    assert result.returncode == 0, result.stdout + result.stderr
    assert "ENCLOSURE_ARTIFACTS_VERIFIED" in result.stdout, result.stdout + result.stderr
