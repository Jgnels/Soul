"""Read-only UE asset inspection for the campaign world art pass.

Run in the parent's already-open Soul editor Python session:
    exec(open(r'<project>/Tools/WorldTerrain/asset_manifest.py').read())

Loads meshes for inspection; never opens/saves maps, changes materials, creates
actors or edits donor assets. Writes a JSON receipt under the project Evidence.
Set SOUL_WORLD_ASSET_AUDIT_PATHS to a list of package paths before exec to narrow
the batch, and SOUL_WORLD_ASSET_AUDIT_OUTPUT to select a different receipt path.
Unknown API metrics remain explicit errors rather than invented values.
"""

import datetime
import json
from pathlib import Path

import unreal


DEFAULT_PATHS = [
    "/Game/Medieval_Megapack/Meshes/Courtyard/Houses/SM_Building_E",
    "/Game/Medieval_Megapack/Meshes/Courtyard/Houses/SM_Roof_E",
    "/Game/Medieval_Megapack/Meshes/Courtyard/Houses/SM_Building_A",
    "/Game/Medieval_Megapack/Meshes/Courtyard/Houses/SM_Roof_A",
    "/Game/Medieval_Megapack/Meshes/Towers/SM_Tower_Bottom_01",
    "/Game/Medieval_Megapack/Meshes/Towers/SM_Tower_Middle_01",
    "/Game/Medieval_Megapack/Meshes/Towers/SM_Tower_Top_01",
    "/Game/Medieval_Megapack/Meshes/Towers/SM_Gate_SM_Gate_Wall_Inner",
    "/Game/Medieval_Megapack/Meshes/Towers/SM_Gate_Sm_Gate_Wall_Outer",
    "/Game/Medieval_Megapack/Meshes/Towers/SM_Gate_SM_Arch_Tall",
    "/Game/Medieval_Megapack/Meshes/Towers/SM_Gate_SM_Portcullis",
    "/Game/Medieval_Megapack/Meshes/Walls/SM_Wall_6M",
    "/Game/Kingdom_Capital/Meshes/Buildings/SM_circular_module_01",
    "/Game/Kingdom_Capital/Meshes/Buildings/SM_circular_module_04",
    "/Game/Kingdom_Capital/Meshes/Bridge/SM_arch_bridge_01",
    "/Game/Forest_village/Meshes/Kit/SM_small_wood_building",
    "/Game/Forest_village/Meshes/Vegetation/SM_pine_tree_01",
    "/Game/Forest_village/Meshes/Vegetation/Update/SM_tree_01_Update",
    "/Game/Forest_village/Meshes/Rocks/SM_rock_03",
    "/Game/AlienPlanet/Meshes/SM_BigTowerComplex",
    "/Game/AlienPlanet/Meshes/SM_BigBetweenTower",
]


def xyz(vector):
    return [float(vector.x), float(vector.y), float(vector.z)]


def query(row, key, function):
    try:
        row[key] = function()
    except Exception as error:
        row.setdefault("unavailable", {})[key] = str(error)


def bounds(mesh):
    value = mesh.get_bounds()
    center, extent = xyz(value.origin), xyz(value.box_extent)
    minimum = [center[i] - extent[i] for i in range(3)]
    maximum = [center[i] + extent[i] for i in range(3)]
    return {
        "origin_cm": center,
        "extent_cm": extent,
        "min_cm": minimum,
        "max_cm": maximum,
        "dimensions_cm": [2.0 * item for item in extent],
        "pivot_to_floor_cm": -minimum[2],
        "pivot_to_center_xy_cm": [-center[0], -center[1]],
    }


def materials(mesh):
    result = []
    for index, slot in enumerate(mesh.get_editor_property("static_materials")):
        material = slot.get_editor_property("material_interface")
        result.append({
            "index": index,
            "slot": str(slot.get_editor_property("material_slot_name")),
            "material": material.get_path_name() if material else None,
        })
    return result


def inspect_mesh(path):
    row = {"path": path}
    try:
        mesh = unreal.load_asset(path)
        if not mesh:
            raise RuntimeError("Asset did not load")
        row["class"] = mesh.get_class().get_name()
        query(row, "bounds", lambda: bounds(mesh))
        query(row, "materials", lambda: materials(mesh))
        query(row, "lod_count", mesh.get_num_lods)
        query(row, "triangles_lod0", lambda: mesh.get_num_triangles(0))
        query(row, "vertices_lod0", lambda: mesh.get_num_vertices(0))
    except Exception as error:
        row["error"] = str(error)
    return row


project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
output = Path(globals().get(
    "SOUL_WORLD_ASSET_AUDIT_OUTPUT",
    project / "Evidence/CampaignWorldTerrain-20261005/asset-manifest.json",
))
paths = globals().get("SOUL_WORLD_ASSET_AUDIT_PATHS", DEFAULT_PATHS)
receipt = {
    "captured_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
    "project": str(project),
    "operation": "read-only load and inspect; no asset or level writes",
    "units": "Unreal centimeters in mesh local space",
    "assets": [inspect_mesh(path) for path in paths],
    "assembly_note": (
        "Use one shared XY scale for matching tower modules. Center each local "
        "bounds XY at the stack center; place its local min Z at the preceding "
        "module max Z. Inspect seams/overlap in a close render before acceptance."
    ),
}
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(receipt, indent=2), encoding="utf-8")
unreal.log("SOUL_WORLD_ASSET_AUDIT " + str(output))
