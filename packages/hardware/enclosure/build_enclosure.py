"""Build the two-piece printable enclosure in FreeCAD.

Run with:
  /Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd \
    packages/hardware/enclosure/build_enclosure.py
"""
import json
from pathlib import Path

import FreeCAD as App
import Mesh
import MeshPart
import Part

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
    # Keep the board on the screen-facing side of the boss tip.
    "devboard_reference_clearance": 0.2,
    # Upright PCB: long edge along X, component face toward +Y, USB-C toward +X.
    # Outline scaled from the PDF using the assumed 2.54 mm header pitch.
    "ec600x_length": 88.0,
    "ec600x_height": 48.0,
    "ec600x_corner_radius": 2.5,
    "ec600x_x": 28.0,
    "ec600x_y": 20.0,
    "ec600x_floor_z": 3.0,
    "ec600x_board_z": 8.0,
    "ec600x_thickness": 1.6,
    "ec600x_mount_hole_x": 55.46,
    "ec600x_mount_hole_z": 6.9,
    "ec600x_mount_hole_diameter": 2.4,
    "ec600x_slot_width": 2.0,
    "ec600x_guide_wall": 1.6,
    "ec600x_guide_height": 8.0,
    "ec600x_edge_gap": 0.4,
    "ec600x_guide_inset": 6.0,
    "ec600x_rear_edge_land": 1.0,
    "ec600x_latch_thickness": 1.5,
    "ec600x_latch_overlap": 4.0,
    "ec600x_latch_top_gap": 0.25,
    "ec600x_latch_release": 4.6,
    "ec600x_insertion_lift": 8.4,
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


def rounded_prism_xy(x0, y0, width, depth, height, z0, radius):
    """A rounded rectangle in the XY plane extruded along +Z."""
    if radius <= 0 or 2 * radius >= min(width, depth):
        raise ValueError("rounded rectangle radius is too large")
    shapes = [
        Part.makeBox(width - 2 * radius, depth, height, App.Vector(x0 + radius, y0, z0)),
        Part.makeBox(width, depth - 2 * radius, height, App.Vector(x0, y0 + radius, z0)),
    ]
    for cx in (x0 + radius, x0 + width - radius):
        for cy in (y0 + radius, y0 + depth - radius):
            shapes.append(
                Part.makeCylinder(radius, height, App.Vector(cx, cy, z0), App.Vector(0, 0, 1))
            )
    return shapes[0].multiFuse(shapes[1:]).removeSplitter()


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


def hex_prism_xy(x, y, z0, height, across_flats, rotation_degrees=30.0):
    """Regular hexagonal prism in the XY plane, extruded along +Z."""
    import math

    circumradius = across_flats / math.sqrt(3.0)
    angle = math.radians(rotation_degrees)
    points = [
        App.Vector(
            x + circumradius * math.cos(angle + index * math.pi / 3.0),
            y + circumradius * math.sin(angle + index * math.pi / 3.0),
            z0,
        )
        for index in range(6)
    ]
    points.append(points[0])
    wire = Part.makePolygon(points)
    return Part.Face(wire).extrude(App.Vector(0, 0, height))


def make_ec600x_mounts():
    """Two edge U-guides with relieved component-side jaws and bottom ledges."""
    x0, y0, z0 = P["ec600x_x"], P["ec600x_y"], P["ec600x_board_z"]
    x1 = x0 + P["ec600x_length"]
    gap = (P["ec600x_slot_width"] - P["ec600x_thickness"]) / 2
    wall = P["ec600x_guide_wall"]
    y_min = y0 - gap - wall
    depth = P["ec600x_slot_width"] + 2 * wall
    top = z0 + P["ec600x_guide_height"]
    floor = P["ec600x_floor_z"]
    supports = []
    for left in (True, False):
        start = x0 - 3.5 if left else x1 - P["ec600x_guide_inset"]
        end = x0 + P["ec600x_guide_inset"] if left else x1 + 2.0
        guide = Part.makeBox(end - start, depth, top - floor, App.Vector(start, y_min, floor))
        slot_x = x0 - P["ec600x_edge_gap"] if left else start - 0.1
        slot_end = end + 0.1 if left else x1 + P["ec600x_edge_gap"]
        guide = guide.cut(Part.makeBox(slot_end - slot_x, P["ec600x_slot_width"], top - z0 + 0.1, App.Vector(slot_x, y0 - gap, z0)))
        # Only the outer 1 mm board edge meets the tall component-side jaw.
        relief_x = x0 + P["ec600x_rear_edge_land"] if left else start - 0.1
        relief_end = end + 0.1 if left else x1 - P["ec600x_rear_edge_land"]
        guide = guide.cut(Part.makeBox(relief_end - relief_x, wall + 0.2, top - z0, App.Vector(relief_x, y0 + P["ec600x_thickness"] + gap, z0 + 1.0)))
        supports.append(guide.removeSplitter())
    return supports


def make_ec600x_latch(released=False):
    """Side cantilever attached to the left guide; the hook captures the top edge.

    The released shape is a kinematic clearance envelope, not a stress simulation.
    """
    x = P["ec600x_x"] - 3.5
    y = P["ec600x_y"] - 0.2
    root = P["ec600x_board_z"] + P["ec600x_guide_height"] - 0.2
    catch = P["ec600x_board_z"] + P["ec600x_height"] + P["ec600x_latch_top_gap"]
    t = P["ec600x_latch_thickness"]
    shift = P["ec600x_latch_release"] if released else 0.0
    tip = P["ec600x_x"] + P["ec600x_latch_overlap"] - shift
    outline = [(x, root), (x+t, root), (x+t-shift, catch), (tip, catch),
               (tip, catch+0.7), (tip-1.0, catch+2.0), (x-shift, catch+2.0)]
    points = [App.Vector(px, y, pz) for px, pz in outline]
    points.append(points[0])
    latch = Part.Face(Part.makePolygon(points)).extrude(App.Vector(0, P["ec600x_slot_width"], 0))
    tab = Part.makeBox(3.0, 4.0, 2.0, App.Vector(x-shift-2.0, y, catch-1.0))
    return latch.fuse(tab).removeSplitter()


def make_front_shell(latch_released=False):
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
    shell = shell.fuse(make_ec600x_mounts() + [make_ec600x_latch(latch_released)]).removeSplitter()
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
        ("DevboardReferenceClearance", P["devboard_reference_clearance"]),
        ("EC600XLength", P["ec600x_length"]),
        ("EC600XHeight", P["ec600x_height"]),
        ("EC600XX", P["ec600x_x"]),
        ("EC600XY", P["ec600x_y"]),
        ("EC600XBoardZ", P["ec600x_board_z"]),
        ("EC600XMountHoleX", P["ec600x_mount_hole_x"]),
        ("EC600XMountHoleZ", P["ec600x_mount_hole_z"]),
        ("EC600XSlotWidth", P["ec600x_slot_width"]),
        ("EC600XGuideWall", P["ec600x_guide_wall"]),
        ("EC600XGuideHeight", P["ec600x_guide_height"]),
        ("EC600XEdgeGap", P["ec600x_edge_gap"]),
        ("EC600XLatchThickness", P["ec600x_latch_thickness"]),
        ("EC600XLatchRelease", P["ec600x_latch_release"]),
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


def ec600x_insertion_report(board):
    """Conservative rectangular sweeps: rear entry, then down into both guides."""
    x, y, z = P["ec600x_x"], P["ec600x_y"], P["ec600x_board_z"]
    lift = P["ec600x_insertion_lift"]
    raised = Part.makeBox(P["ec600x_length"], 80.0 - y, P["ec600x_height"], App.Vector(x, y, z + lift))
    descent = Part.makeBox(P["ec600x_length"], P["ec600x_thickness"], P["ec600x_height"] + lift, App.Vector(x, y, z))
    released_shell = make_front_shell(latch_released=True)
    raised_board = board.copy()
    raised_board.translate(App.Vector(0, 0, 0.8))
    return {
        "rear_entry_sweep_intersection_mm3": float(raised.common(released_shell).Volume),
        "descent_sweep_intersection_mm3": float(descent.common(released_shell).Volume),
        "latched_lift_contact_mm3": float(raised_board.common(make_ec600x_latch()).Volume),
        "latch_release_travel_mm": P["ec600x_latch_release"],
        "lift_mm": lift,
        "roof_clearance_mm": P["outer_height"] - P["wall"] - z - P["ec600x_height"] - lift,
        "scope": "Bare PCB envelopes, cover removed, latch held released; elastic behavior and components require trial fit",
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
    add_property(front, "Window", "84.8 x 63.6 mm display area, R5.5")
    add_property(front, "WallThickness", "4 mm")
    add_property(front, "PrintOrientation", "Front face down; rotate +90 deg about X")
    add_property(front, "EC600XMount", "2 x bottom U-guides; left releasable top-edge latch")
    add_property(front, "EC600XPlacement", "88 x 48 x 1.6 mm upright reference; component face +Y; USB-C +X")
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
    devboard_ref_y = (
        P["devboard_boss_y"]
        - P["devboard_thickness"]
        - P["devboard_reference_clearance"]
    )
    devboard_ref.Shape = Part.makeBox(
        P["devboard_length"],
        P["devboard_thickness"],
        P["devboard_height"],
        App.Vector(
            P["devboard_x"],
            devboard_ref_y,
            P["devboard_z"],
        ),
    )
    add_property(devboard_ref, "Source", "User-provided assets/esp32-size.webp: 85 x 56 mm")
    add_property(devboard_ref, "MountingHolePitch", "58 x 49 mm")
    add_property(devboard_ref, "GPIOOrientation", "Toward display side")
    add_property(devboard_ref, "Interfaces", "Internal; no enclosure openings")
    add_property(
        devboard_ref,
        "ReferencePlacement",
        "Behind display projection; 0.2 mm clearance from boss tips",
    )
    references.addObject(devboard_ref)

    ec600x_ref = doc.addObject("Part::Feature", "EC600XEVBReference")
    ec600x_ref.Label = "EC600X-EVB upright PCB (88 x 48 x 1.6 mm; USB-C +X)"
    ec600x_ref.Shape = rounded_prism_xz(
        P["ec600x_x"],
        P["ec600x_board_z"],
        P["ec600x_length"],
        P["ec600x_height"],
        P["ec600x_thickness"],
        P["ec600x_y"],
        P["ec600x_corner_radius"],
    )
    ec600x_ref.Shape = ec600x_ref.Shape.cut(screw_cylinder(
        P["ec600x_x"] + P["ec600x_mount_hole_x"],
        P["ec600x_board_z"] + P["ec600x_mount_hole_z"],
        P["ec600x_y"] - 0.1, P["ec600x_thickness"] + 0.2,
        P["ec600x_mount_hole_diameter"] / 2,
    ))
    add_property(ec600x_ref, "Source", "User-provided EC600X系列开发板丝印.pdf; size scaled from 2.54 mm header pitch")
    add_property(ec600x_ref, "BoardSize", "88 x 48 x 1.6 mm estimated; verify against physical PCB")
    add_property(ec600x_ref, "MountingHole", "Single M2 hole beside QuecPython marking")
    add_property(ec600x_ref, "MountingHolePosition", "Local X=55.46 mm, Z=6.9 mm; nominal 2.4 mm diameter")
    add_property(ec600x_ref, "ComponentFace", "Rearward (+Y); component heights and solder joints require physical verification")
    add_property(ec600x_ref, "USBOrientation", "USB-C toward +X button side")
    add_property(ec600x_ref, "InterfacePolicy", "Internal; no enclosure openings")
    references.addObject(ec600x_ref)

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
            "devboard_cover_intersection_mm3": float(
                devboard_ref.Shape.common(cover_shape).Volume
            ),
            "ec600x_front_intersection_mm3": float(
                ec600x_ref.Shape.common(front_shape).Volume
            ),
            "ec600x_mount_intersection_mm3": [
                float(ec600x_ref.Shape.common(shape).Volume)
                for shape in make_ec600x_mounts() + [make_ec600x_latch()]
            ],
            "ec600x_cover_intersection_mm3": float(ec600x_ref.Shape.common(cover_shape).Volume),
            "ec600x_esp32_intersection_mm3": float(ec600x_ref.Shape.common(devboard_ref.Shape).Volume),
            "ec600x_esp32_pcb_gap_mm": float(ec600x_ref.Shape.distToShape(devboard_ref.Shape)[0]),
            "esp32_front_intersection_mm3": float(devboard_ref.Shape.common(front_shape).Volume),
            "ec600x_insertion": ec600x_insertion_report(ec600x_ref.Shape),
            "ec600x_reference_bbox_mm": {
                "xmin": P["ec600x_x"],
                "ymin": P["ec600x_y"],
                "zmin": P["ec600x_board_z"],
                "xmax": P["ec600x_x"] + P["ec600x_length"],
                "ymax": P["ec600x_y"] + P["ec600x_thickness"],
                "zmax": P["ec600x_board_z"] + P["ec600x_height"],
            },
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
                "ymin": devboard_ref_y,
                "zmin": P["devboard_z"],
                "xmax": P["devboard_x"] + P["devboard_length"],
                "ymax": devboard_ref_y + P["devboard_thickness"],
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
