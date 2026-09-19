"""Build the two-piece printable enclosure in FreeCAD.

Run with:
  /Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd \
    packages/hardware/enclosure/build_enclosure.py
"""
from pathlib import Path
import json
import sys

import FreeCAD as App
import Part
import Mesh
import MeshPart

ROOT = Path(__file__).resolve().parent
OUT = ROOT / "output"
OUT.mkdir(parents=True, exist_ok=True)

# All dimensions are mm. The enclosure envelope is X (length) x Y (depth) x Z (height).
P = {
    "outer_length": 130.0,
    "outer_depth": 60.0,
    "outer_height": 70.0,
    "wall": 4.0,
    "outer_radius": 8.0,
    "inner_radius": 4.0,
    "window_x": 14.0,
    "window_z": 16.0,
    "window_length": 102.0,
    "window_height": 38.0,
    "window_radius": 6.0,
    "boss_radius": 5.0,
    "screw_clearance": 3.4,
    "boss_start_y": 48.0,
    "boss_length": 12.0,
    "boss_x": (8.0, 122.0),
    "boss_z": (8.0, 62.0),
    "cover_y": 56.0,
    "cover_thickness": 4.0,
    "cover_edge": 0.5,
    "cover_lip_start_y": 54.0,
    "cover_lip_depth": 2.0,
    "cover_lip_clearance": 0.4,
    "cover_counterbore_radius": 3.2,
    "cover_counterbore_depth": 1.2,
}


def rounded_prism_xz(x0, z0, width, height, depth, y0, radius):
    """A rounded rectangle in the XZ plane extruded along +Y."""
    if radius <= 0 or 2 * radius >= min(width, height):
        raise ValueError("rounded rectangle radius is too large")
    shapes = [
        Part.makeBox(width - 2 * radius, depth, height, App.Vector(x0 + radius, y0, z0)),
        Part.makeBox(width, depth, height - 2 * radius, App.Vector(x0, y0, z0 + radius)),
    ]
    for cx in (x0 + radius, x0 + width - radius):
        for cz in (z0 + radius, z0 + height - radius):
            shapes.append(
                Part.makeCylinder(
                    radius, depth, App.Vector(cx, y0, cz), App.Vector(0, 1, 0)
                )
            )
    result = shapes[0].multiFuse(shapes[1:])
    return result.removeSplitter()


def screw_cylinder(x, z, y0, depth, radius):
    return Part.makeCylinder(radius, depth, App.Vector(x, y0, z), App.Vector(0, 1, 0))


def make_front_shell():
    outer = rounded_prism_xz(
        0,
        0,
        P["outer_length"],
        P["outer_height"],
        P["outer_depth"],
        0,
        P["outer_radius"],
    )
    inner = rounded_prism_xz(
        P["wall"],
        P["wall"],
        P["outer_length"] - 2 * P["wall"],
        P["outer_height"] - 2 * P["wall"],
        P["outer_depth"] - P["wall"] + 1.0,
        P["wall"],
        P["inner_radius"],
    )
    shell = outer.cut(inner)
    window = rounded_prism_xz(
        P["window_x"],
        P["window_z"],
        P["window_length"],
        P["window_height"],
        P["wall"] + 2.0,
        -1.0,
        P["window_radius"],
    )
    shell = shell.cut(window)

    bosses = []
    for x in P["boss_x"]:
        for z in P["boss_z"]:
            bosses.append(
                screw_cylinder(x, z, P["boss_start_y"], P["boss_length"], P["boss_radius"])
            )
    shell = shell.fuse(bosses).removeSplitter()

    # M3 clearance channels through the rear screw columns.
    for x in P["boss_x"]:
        for z in P["boss_z"]:
            shell = shell.cut(
                screw_cylinder(
                    x,
                    z,
                    P["boss_start_y"] - 1.0,
                    P["boss_length"] + 2.0,
                    P["screw_clearance"] / 2.0,
                )
            )
    return shell.removeSplitter()


def make_back_cover():
    edge = P["cover_edge"]
    cover = rounded_prism_xz(
        edge,
        edge,
        P["outer_length"] - 2 * edge,
        P["outer_height"] - 2 * edge,
        P["cover_thickness"],
        P["cover_y"],
        P["outer_radius"] - edge,
    )

    # A shallow locating lip enters the front shell cavity.
    lip_clear = P["cover_lip_clearance"]
    lip = rounded_prism_xz(
        P["wall"] + lip_clear,
        P["wall"] + lip_clear,
        P["outer_length"] - 2 * (P["wall"] + lip_clear),
        P["outer_height"] - 2 * (P["wall"] + lip_clear),
        P["cover_lip_depth"],
        P["cover_lip_start_y"],
        P["inner_radius"] - lip_clear,
    )
    cover = cover.fuse(lip).removeSplitter()

    for x in P["boss_x"]:
        for z in P["boss_z"]:
            cover = cover.cut(
                screw_cylinder(x, z, P["cover_y"] - 1.0, P["cover_thickness"] + 2.0, P["screw_clearance"] / 2.0)
            )
            # Rear-side counterbore for an M3 pan/cylinder head.
            cover = cover.cut(
                screw_cylinder(
                    x,
                    z,
                    P["cover_y"] + P["cover_thickness"] - P["cover_counterbore_depth"],
                    P["cover_counterbore_depth"] + 0.1,
                    P["cover_counterbore_radius"],
                )
            )
    return cover.removeSplitter()


def add_property(obj, name, value, group="Design"):
    obj.addProperty("App::PropertyString", name, group)
    setattr(obj, name, str(value))


def add_parameters(doc):
    sheet = doc.addObject("Spreadsheet::Sheet", "Parameters")
    sheet.Label = "Parameters (mm)"
    rows = [
        ("OuterLength", P["outer_length"]),
        ("OuterDepth", P["outer_depth"]),
        ("OuterHeight", P["outer_height"]),
        ("Wall", P["wall"]),
        ("OuterRadius", P["outer_radius"]),
        ("WindowLength", P["window_length"]),
        ("WindowHeight", P["window_height"]),
        ("WindowRadius", P["window_radius"]),
        ("ScrewClearance", P["screw_clearance"]),
        ("CoverThickness", P["cover_thickness"]),
        ("CoverLipClearance", P["cover_lip_clearance"]),
    ]
    sheet.set("A1", "Parameter")
    sheet.set("B1", "Value")
    sheet.set("C1", "Unit")
    for row, (name, value) in enumerate(rows, start=2):
        sheet.set(f"A{row}", name)
        sheet.set(f"B{row}", str(value))
        sheet.set(f"C{row}", "mm")
        sheet.setAlias(f"B{row}", name)
    return sheet


def transformed_for_print(shape):
    printed = shape.copy()
    printed.rotate(App.Vector(0, 0, 0), App.Vector(1, 0, 0), 90)
    bb = printed.BoundBox
    printed.translate(App.Vector(-bb.XMin, -bb.YMin, -bb.ZMin))
    return printed


def export_stl(shape, path):
    mesh = MeshPart.meshFromShape(
        Shape=shape,
        LinearDeflection=0.05,
        AngularDeflection=0.15,
        Relative=False,
    )
    mesh.write(str(path))
    return Mesh.Mesh(str(path))


def shape_report(name, shape):
    bb = shape.BoundBox
    return {
        "name": name,
        "is_valid": bool(shape.isValid()),
        "solids": len(shape.Solids),
        "shells": len(shape.Shells),
        "volume_mm3": float(shape.Volume),
        "bbox_mm": {
            "xmin": float(bb.XMin),
            "ymin": float(bb.YMin),
            "zmin": float(bb.ZMin),
            "xmax": float(bb.XMax),
            "ymax": float(bb.YMax),
            "zmax": float(bb.ZMax),
        },
    }


def main():
    doc = App.newDocument("PrintableEnclosure")
    params = add_parameters(doc)
    front_shape = make_front_shell()
    cover_shape = make_back_cover()

    assembly = doc.addObject("App::Part", "EnclosureAssembly")
    assembly.Label = "Enclosure Assembly (130x60x70 mm)"

    front = doc.addObject("PartDesign::Feature", "FrontShell")
    front.Label = "Front shell (printable)"
    front.Shape = front_shape
    add_property(front, "OverallSize", "130 x 60 x 70 mm")
    add_property(front, "Window", "102 x 38 mm, R6")
    add_property(front, "WallThickness", "4 mm")
    add_property(front, "PrintOrientation", "Front face down; rotate +90 deg about X")
    assembly.addObject(front)

    cover = doc.addObject("PartDesign::Feature", "BackCover")
    cover.Label = "Back cover (printable)"
    cover.Shape = cover_shape
    add_property(cover, "Thickness", "4 mm")
    add_property(cover, "Fasteners", "4 x M3, 3.4 mm clearance")
    add_property(cover, "PrintOrientation", "Flat face down; rotate +90 deg about X")
    assembly.addObject(cover)

    params.Visibility = False
    doc.recompute()

    fcstd = OUT / "enclosure_130x60x70.FCStd"
    step = OUT / "enclosure_130x60x70.step"
    doc.recompute()
    doc.saveAs(str(fcstd))
    Part.export([front, cover], str(step))

    front_print = transformed_for_print(front_shape)
    cover_print = transformed_for_print(cover_shape)
    front_mesh = export_stl(front_print, OUT / "front_shell_print.stl")
    cover_mesh = export_stl(cover_print, OUT / "back_cover_print.stl")

    report = {
        "parameters": P,
        "parts": [shape_report("front_shell", front_shape), shape_report("back_cover", cover_shape)],
        "print_parts": [
            {
                "name": "front_shell_print",
                "is_solid_mesh": bool(front_mesh.isSolid()),
                "bbox_mm": {
                    "xmin": float(front_mesh.BoundBox.XMin),
                    "ymin": float(front_mesh.BoundBox.YMin),
                    "zmin": float(front_mesh.BoundBox.ZMin),
                    "xmax": float(front_mesh.BoundBox.XMax),
                    "ymax": float(front_mesh.BoundBox.YMax),
                    "zmax": float(front_mesh.BoundBox.ZMax),
                },
            },
            {
                "name": "back_cover_print",
                "is_solid_mesh": bool(cover_mesh.isSolid()),
                "bbox_mm": {
                    "xmin": float(cover_mesh.BoundBox.XMin),
                    "ymin": float(cover_mesh.BoundBox.YMin),
                    "zmin": float(cover_mesh.BoundBox.ZMin),
                    "xmax": float(cover_mesh.BoundBox.XMax),
                    "ymax": float(cover_mesh.BoundBox.YMax),
                    "zmax": float(cover_mesh.BoundBox.ZMax),
                },
            },
        ],
    }
    (OUT / "validation.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    doc.recompute()
    doc.saveAs(str(fcstd))
    App.closeDocument(doc.Name)
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
