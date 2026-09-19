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
    "shell_depth": 56.0,
    "window_z": 3.2,
    "window_length": 84.8,
    "window_height": 63.6,
    "window_radius": 5.5,
    "window_x": 4.0,
    "button_window_x": 91.0,
    "button_window_z": 6.0,
    "button_window_size": 35.0,
    "button_window_radius": 4.0,
    "divider_x": 88.8,
    "divider_width": 2.2,
    "divider_rear_clearance": 4.2,
    "screen_outer_length": 91.0,
    "screen_outer_height": 77.0,
    "screen_outer_thickness": 1.2,
    "boss_outer_af": 8.0,
    "boss_clearance_af": 8.8,
    "m2_clearance": 2.4,
    # M2 female brass hex standoff capture dimensions.
    "m2_standoff_af": 4.5,
    "m2_hex_pocket_af": 5.0,
    "m2_hex_lead_in_af": 5.4,
    "m2_standoff_length": 5.0,
    "m2_hex_lead_in_depth": 0.6,
    "m2_screw_length": 6.0,
    "boss_start_y": 44.0,
    "boss_length": 12.0,
    "boss_x": (8.0, 122.0),
    "boss_z": (8.0, 62.0),
    "cover_y": 56.0,
    "cover_thickness": 4.0,
    "cover_edge": 0.5,
    "cover_lip_start_y": 54.0,
    "cover_lip_depth": 2.0,
    "cover_lip_clearance": 0.4,
    "cover_counterbore_radius": 2.3,
    "cover_counterbore_depth": 1.7,
    # Waveshare ESP32-P4-Module-DEV-KIT reference from assets/esp32-size.webp.
    # The board is kept behind the display projection; GPIO edge is toward the display side.
    "devboard_length": 85.0,
    "devboard_height": 56.0,
    "devboard_thickness": 1.6,
    "devboard_x": 18.0,
    "devboard_z": 5.0,
    "devboard_hole_x": (4.5, 62.5),
    "devboard_hole_z": (3.5, 52.5),
    "devboard_boss_outer_af": 7.5,
    "devboard_boss_y": 50.8,
    "devboard_boss_height": 5.4,
    "devboard_insert_depth": 5.0,
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


def hex_prism_xz(x, z, y0, depth, across_flats, rotation_degrees=30.0):
    """Regular hexagonal prism in the XZ plane, extruded along +Y."""
    import math

    circumradius = across_flats / math.sqrt(3.0)
    angle = math.radians(rotation_degrees)
    points = [
        App.Vector(
            x + circumradius * math.cos(angle + index * math.pi / 3.0),
            y0,
            z + circumradius * math.sin(angle + index * math.pi / 3.0),
        )
        for index in range(6)
    ]
    points.append(points[0])
    wire = Part.makePolygon(points)
    return Part.Face(wire).extrude(App.Vector(0, depth, 0))


def make_front_shell():
    outer = rounded_prism_xz(
        0,
        0,
        P["outer_length"],
        P["outer_height"],
        P["shell_depth"],
        0,
        P["outer_radius"],
    )
    inner = rounded_prism_xz(
        P["wall"],
        P["wall"],
        P["outer_length"] - 2 * P["wall"],
        P["outer_height"] - 2 * P["wall"],
        P["shell_depth"] - P["wall"] + 1.0,
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

    button_window = rounded_prism_xz(
        P["button_window_x"],
        P["button_window_z"],
        P["button_window_size"],
        P["button_window_size"],
        P["wall"] + 2.0,
        -1.0,
        P["button_window_radius"],
    )
    shell = shell.cut(button_window)

    # Vertical internal separator: screen on the left, 35 mm button module at lower right.
    divider_depth = (
        P["cover_lip_start_y"]
        - (P["wall"] - 0.2)
        - P["divider_rear_clearance"]
    )
    divider = Part.makeBox(
        P["divider_width"],
        divider_depth,
        P["outer_height"] - 2 * P["wall"],
        App.Vector(P["divider_x"], P["wall"] - 0.2, P["wall"]),
    )
    shell = shell.fuse(divider).removeSplitter()

    bosses = []
    for x in P["boss_x"]:
        for z in P["boss_z"]:
            bosses.append(
                hex_prism_xz(
                    x,
                    z,
                    P["boss_start_y"],
                    P["boss_length"],
                    P["boss_outer_af"],
                )
            )
    shell = shell.fuse(bosses).removeSplitter()

    # Rear-entry hexagonal capture seats for M2 female brass hex standoffs.
    for x in P["boss_x"]:
        for z in P["boss_z"]:
            shell = shell.cut(
                hex_prism_xz(
                    x,
                    z,
                    P["shell_depth"] - P["m2_standoff_length"],
                    P["m2_standoff_length"] + 0.1,
                    P["m2_hex_pocket_af"],
                )
            )
            shell = shell.cut(
                hex_prism_xz(
                    x,
                    z,
                    P["shell_depth"] - P["m2_hex_lead_in_depth"],
                    P["m2_hex_lead_in_depth"] + 0.1,
                    P["m2_hex_lead_in_af"],
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
    lip_outer = rounded_prism_xz(
        P["wall"] + lip_clear,
        P["wall"] + lip_clear,
        P["outer_length"] - 2 * (P["wall"] + lip_clear),
        P["outer_height"] - 2 * (P["wall"] + lip_clear),
        P["cover_lip_depth"],
        P["cover_lip_start_y"],
        P["inner_radius"] - lip_clear,
    )
    # Keep the locating lip as a perimeter ring so the recessed screw bases
    # and the internal electronics have clearance behind the cover.
    lip_inner = rounded_prism_xz(
        P["wall"] + 8.0,
        P["wall"] + 8.0,
        P["outer_length"] - 2 * (P["wall"] + 8.0),
        P["outer_height"] - 2 * (P["wall"] + 8.0),
        P["cover_lip_depth"] + 0.4,
        P["cover_lip_start_y"] - 0.2,
        max(1.0, P["inner_radius"] - 2.0),
    )
    lip = lip_outer.cut(lip_inner)
    for x in P["boss_x"]:
        for z in P["boss_z"]:
            lip = lip.cut(
                hex_prism_xz(
                    x,
                    z,
                    P["cover_lip_start_y"] - 0.4,
                    P["cover_lip_depth"] + 0.8,
                    P["boss_clearance_af"],
                )
            )
    cover = cover.fuse(lip).removeSplitter()

    for x in P["boss_x"]:
        for z in P["boss_z"]:
            cover = cover.cut(
                screw_cylinder(x, z, P["cover_y"] - 1.0, P["cover_thickness"] + 2.0, P["m2_clearance"] / 2.0)
            )
            # Rear-side counterbore for an M2 pan/cylinder head.
            cover = cover.cut(
                screw_cylinder(
                    x,
                    z,
                    P["cover_y"] + P["cover_thickness"] - P["cover_counterbore_depth"],
                    P["cover_counterbore_depth"] + 0.1,
                    P["cover_counterbore_radius"],
                )
            )

    # Four rear-cover mounting seats for M2 female brass hex standoffs. The seats
    # project into the front-shell cavity so the board can be installed before
    # the front shell is clipped over it. A 0.2 mm overlap with the cover body
    # keeps the seats a single printable solid.
    devboard_mounts = []
    for local_x in P["devboard_hole_x"]:
        for local_z in P["devboard_hole_z"]:
            x = P["devboard_x"] + local_x
            z = P["devboard_z"] + local_z
            devboard_mounts.append(
                hex_prism_xz(
                    x,
                    z,
                    P["devboard_boss_y"],
                    P["devboard_boss_height"],
                    P["devboard_boss_outer_af"],
                )
            )
    cover = cover.fuse(devboard_mounts).removeSplitter()
    for local_x in P["devboard_hole_x"]:
        for local_z in P["devboard_hole_z"]:
            x = P["devboard_x"] + local_x
            z = P["devboard_z"] + local_z
            cover = cover.cut(
                hex_prism_xz(
                    x,
                    z,
                    P["devboard_boss_y"],
                    P["devboard_insert_depth"],
                    P["m2_hex_pocket_af"],
                )
            )
            cover = cover.cut(
                hex_prism_xz(
                    x,
                    z,
                    P["devboard_boss_y"],
                    P["m2_hex_lead_in_depth"],
                    P["m2_hex_lead_in_af"],
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
        ("M2Clearance", P["m2_clearance"]),
        ("M2StandoffAcrossFlats", P["m2_standoff_af"]),
        ("M2HexPocketAcrossFlats", P["m2_hex_pocket_af"]),
        ("M2HexLeadInAcrossFlats", P["m2_hex_lead_in_af"]),
        ("M2StandoffLength", P["m2_standoff_length"]),
        ("M2HexLeadInDepth", P["m2_hex_lead_in_depth"]),
        ("M2ScrewLength", P["m2_screw_length"]),
        ("CoverThickness", P["cover_thickness"]),
        ("CoverLipClearance", P["cover_lip_clearance"]),
        ("ScreenOuterLength", P["screen_outer_length"]),
        ("ScreenOuterHeight", P["screen_outer_height"]),
        ("ButtonMaxSize", P["button_window_size"]),
        ("DividerX", P["divider_x"]),
        ("DividerWidth", P["divider_width"]),
        ("DividerRearClearance", P["divider_rear_clearance"]),
        ("DevboardLength", P["devboard_length"]),
        ("DevboardHeight", P["devboard_height"]),
        ("DevboardThickness", P["devboard_thickness"]),
        ("DevboardX", P["devboard_x"]),
        ("DevboardZ", P["devboard_z"]),
        ("DevboardMountHolePitchX", P["devboard_hole_x"][1] - P["devboard_hole_x"][0]),
        ("DevboardMountHolePitchZ", P["devboard_hole_z"][1] - P["devboard_hole_z"][0]),
        ("DevboardBossAcrossFlats", P["devboard_boss_outer_af"]),
        ("DevboardBossY", P["devboard_boss_y"]),
        ("DevboardBossHeight", P["devboard_boss_height"]),
        ("DevboardInsertDepth", P["devboard_insert_depth"]),
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


def axis_clearance_report(shape, y0, depth, radius):
    checks = []
    for x in P["boss_x"]:
        for z in P["boss_z"]:
            axis = screw_cylinder(x, z, y0, depth, radius)
            checks.append(
                {
                    "x": x,
                    "z": z,
                    "remaining_material_mm3": float(shape.common(axis).Volume),
                }
            )
    return checks


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
    add_property(front, "Window", "84.8 x 63.6 mm display area, R5.5")
    add_property(front, "WallThickness", "4 mm")
    add_property(front, "PrintOrientation", "Front face down; rotate +90 deg about X")
    assembly.addObject(front)

    cover = doc.addObject("PartDesign::Feature", "BackCover")
    cover.Label = "Back cover (printable)"
    cover.Shape = cover_shape
    add_property(cover, "Thickness", "4 mm")
    add_property(cover, "Fasteners", "4 x M2 x 6 mm, 2.4 mm clearance")
    add_property(front, "InsertSeats", "4 x hex M2 brass heat-set insert seats; standoff AF 4.5 mm, blind pocket AF 5.0 mm x 5.0 mm deep")
    add_property(
        cover,
        "DevboardMounts",
        "4 x M2 brass heat-set insert seats; board envelope 85 x 56 x 1.6 mm",
    )
    add_property(cover, "DevboardPlacement", "Behind display projection; GPIO edge toward display")
    add_property(cover, "Interfaces", "Internal only; no enclosure openings")
    add_property(cover, "PrintOrientation", "Flat face down; rotate +90 deg about X")
    assembly.addObject(cover)

    references = doc.addObject("App::Part", "ComponentReferences")
    references.Label = "Component references (non-printing)"

    devboard_ref = doc.addObject("Part::Feature", "ESP32P4DevKitReference")
    devboard_ref.Label = "ESP32-P4-Module-DEV-KIT envelope (85 x 56 x 1.6 mm)"
    devboard_ref.Shape = Part.makeBox(
        P["devboard_length"],
        P["devboard_thickness"],
        P["devboard_height"],
        App.Vector(
            P["devboard_x"],
            P["devboard_boss_y"] + 0.4,
            P["devboard_z"],
        ),
    )
    add_property(devboard_ref, "Source", "User-provided assets/esp32-size.webp: 85 x 56 mm")
    add_property(devboard_ref, "MountingHolePitch", "58 x 49 mm")
    add_property(devboard_ref, "GPIOOrientation", "Toward display side")
    add_property(devboard_ref, "Interfaces", "Internal; no enclosure openings")
    add_property(devboard_ref, "ReferencePlacement", "Behind display projection on rear cover")
    references.addObject(devboard_ref)

    params.Visibility = False
    references.Visibility = False
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
        "assembly": {
            "solid_intersection_mm3": float(front_shape.common(cover_shape).Volume),
            "front_insert_seat_material_mm3": axis_clearance_report(
                front_shape,
                P["shell_depth"] - P["m2_standoff_length"],
                P["m2_standoff_length"],
                P["m2_hex_pocket_af"] / 2.0,
            ),
            "back_cover_m2_axis_material_mm3": axis_clearance_report(
                cover_shape,
                P["cover_lip_start_y"] - 1.0,
                P["cover_thickness"] + P["cover_lip_depth"] + 2.0,
                P["m2_clearance"] / 2.0,
            ),
            "insert_boss_radial_wall_mm": (P["boss_outer_af"] - P["m2_hex_pocket_af"]) / 2.0,
            "devboard_mounts": [
                {
                    "x": P["devboard_x"] + local_x,
                    "z": P["devboard_z"] + local_z,
                    "insert_seat_material_mm3": float(
                        cover_shape.common(
                            hex_prism_xz(
                                P["devboard_x"] + local_x,
                                P["devboard_z"] + local_z,
                                P["devboard_boss_y"],
                                P["devboard_insert_depth"],
                                P["m2_hex_pocket_af"],
                            )
                        ).Volume
                    ),
                    "outer_wall_mm": (P["devboard_boss_outer_af"] - P["m2_hex_pocket_af"]) / 2.0,
                }
                for local_x in P["devboard_hole_x"]
                for local_z in P["devboard_hole_z"]
            ],
            "devboard_reference_bbox_mm": {
                "xmin": P["devboard_x"],
                "ymin": P["devboard_boss_y"] + 0.4,
                "zmin": P["devboard_z"],
                "xmax": P["devboard_x"] + P["devboard_length"],
                "ymax": P["devboard_boss_y"] + 0.4 + P["devboard_thickness"],
                "zmax": P["devboard_z"] + P["devboard_height"],
            },
        },
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
