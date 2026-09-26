"""Unreal Python: build Soul's founder overmap as a structural proof from the v2 import bundle.

This is presentation staging only. It consumes canonical-derived import data and does not
implement campaign rules. Run only when the Soul UE lane is explicitly free.
"""
from __future__ import annotations

import json
import math
import os
import traceback
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parents[1]
BUNDLE = ROOT / "Data" / "UEImport" / "soul_founder_import_bundle_v2_20260922.json"
FIXTURES = ROOT / "Data" / "soul_overmap_ue_acceptance_fixtures_v1_20260922.json"
OUT = ROOT / "Evidence" / "WorldOvermap"
MAP_DEST = "/Game/Soul/Maps/Overmap/LV_Soul_FounderOvermap_Structural"

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

CUBE = "/Engine/BasicShapes/Cube.Cube"
CYLINDER = "/Engine/BasicShapes/Cylinder.Cylinder"
SPHERE = "/Engine/BasicShapes/Sphere.Sphere"

def log(message: str) -> None:
    unreal.log("SOUL_OVERMAP_STAGE " + message)

def load_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))

def set_tags(actor, values: list[str]) -> None:
    try:
        actor.set_editor_property("tags", [unreal.Name(x) for x in values])
    except Exception as exc:
        log("TAG_WARN " + actor.get_actor_label() + " " + repr(exc))

def spawn_mesh(mesh, location, rotation, scale, label, tags):
    actor = actors.spawn_actor_from_class(
        unreal.StaticMeshActor,
        location,
        rotation,
    )
    if not actor:
        raise RuntimeError("Failed to spawn " + label)
    actor.set_actor_label(label, False)
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.set_actor_scale3d(scale)
    set_tags(actor, tags)
    return actor

def spawn_label(text: str, location, parent_tags):
    try:
        actor = actors.spawn_actor_from_class(
            unreal.TextRenderActor,
            location,
            unreal.Rotator(0, 0, 0),
        )
        actor.set_actor_label("Label_" + text.replace(" ", "_"), False)
        component = actor.get_component_by_class(unreal.TextRenderComponent)
        component.set_editor_property("text", unreal.Text(text))
        component.set_editor_property("world_size", 1800.0)
        component.set_editor_property("horizontal_alignment", unreal.HorizontalTextAligment.EHTA_CENTER)
        set_tags(actor, parent_tags + ["overmap.label"])
        return actor
    except Exception as exc:
        log("LABEL_WARN " + text + " " + repr(exc))
        return None

def midpoint(a, b):
    return unreal.Vector(
        (a.x + b.x) * 0.5,
        (a.y + b.y) * 0.5,
        (a.z + b.z) * 0.5,
    )

def segment_transform(a, b, width_cm):
    dx, dy, dz = b.x - a.x, b.y - a.y, b.z - a.z
    horizontal = math.hypot(dx, dy)
    length = math.sqrt(dx * dx + dy * dy + dz * dz)
    yaw = math.degrees(math.atan2(dy, dx))
    pitch = math.degrees(math.atan2(dz, horizontal))
    location = midpoint(a, b)
    rotation = unreal.Rotator(pitch, yaw, 0)
    scale = unreal.Vector(max(length / 100.0, 0.01), max(width_cm / 100.0, 0.5), 1.5)
    return location, rotation, scale

def region_mesh_path(region):
    anchor_type = region["AnchorType"]
    if anchor_type == "major_settlement_silhouette":
        return CUBE
    if anchor_type in ("resource_infrastructure", "resource_landscape", "landmark"):
        return SPHERE
    return CYLINDER

def region_scale(region):
    scale_class = region["AnchorScaleClass"]
    if scale_class == "major":
        return unreal.Vector(80, 80, 55)
    if scale_class == "medium":
        return unreal.Vector(50, 50, 28)
    return unreal.Vector(34, 34, 18)

def main():
    bundle = load_json(BUNDLE)
    fixtures = load_json(FIXTURES)
    if bundle.get("schema") != 2 or bundle.get("status") != "FOUNDER_IMPORT_READY_NON_UE":
        raise RuntimeError("Founder v2 import bundle is not in accepted staging state")
    if bundle["counts"]["regions"] != 9 or bundle["counts"]["routes"] != 10:
        raise RuntimeError("Founder topology count drift")
    if not levels.new_level(MAP_DEST):
        raise RuntimeError("Could not create structural founder overmap " + MAP_DEST)

    cube = unreal.load_asset(CUBE)
    cylinder = unreal.load_asset(CYLINDER)
    sphere = unreal.load_asset(SPHERE)
    mesh_by_path = {CUBE: cube, CYLINDER: cylinder, SPHERE: sphere}
    if not all(mesh_by_path.values()):
        raise RuntimeError("One or more Engine BasicShapes are unavailable")

    # Structural ground only: no shipping terrain, no donor art.
    ground = spawn_mesh(
        cube,
        unreal.Vector(-20000, 45000, -2500),
        unreal.Rotator(0, 0, 0),
        unreal.Vector(7000, 3600, 20),
        "Soul_Founder_StructuralGround",
        ["overmap.structural_ground", "proof_only"],
    )

    region_actors = {}
    for region in bundle["regions"]:
        rid = region["RegionId"]
        loc = unreal.Vector(float(region["X"]), float(region["Y"]), float(region["Z"]))
        mesh_path = region_mesh_path(region)
        tags = [
            "overmap.region",
            "region." + rid,
            "biome." + region["Biome"],
            "landform." + region["Landform"],
            "feature." + region["Feature"],
            "anchor." + region["AnchorType"],
            "recipe." + region["BattleRecipe"],
        ]
        if region["InitialOwner"]:
            tags.append("owner." + region["InitialOwner"])
        if region["SettlementTier"]:
            tags.append("settlement." + region["SettlementTier"])
        actor = spawn_mesh(
            mesh_by_path[mesh_path],
            loc,
            unreal.Rotator(0, 0, 0),
            region_scale(region),
            "Region_" + rid,
            tags,
        )
        region_actors[rid] = actor
        spawn_label(region["DisplayName"], unreal.Vector(loc.x, loc.y, loc.z + 9000), tags)

    route_segment_count = 0
    for route in bundle["routes"]:
        points = [
            unreal.Vector(float(x), float(y), float(z))
            for x, y, z in json.loads(route["SplinePointsCmJson"])
        ]
        for index, (start, end) in enumerate(zip(points, points[1:])):
            loc, rot, scale = segment_transform(start, end, float(route["SplineWidth"]))
            tags = [
                "overmap.route",
                "route." + route["RouteId"],
                "routeclass." + route["RouteClass"],
                "road." + route["Road"],
                "chokepoint." + route["Chokepoint"],
            ]
            spawn_mesh(
                cube,
                loc,
                rot,
                scale,
                f"Route_{route['RouteId'].replace('.', '_')}_seg{index}",
                tags,
            )
            route_segment_count += 1

    # Built-in camera/light only. No project gameplay authority is created here.
    center = unreal.Vector(-20000, 45000, 12000)
    camera_location = unreal.Vector(-20000, -620000, 480000)
    camera = actors.spawn_actor_from_class(unreal.CameraActor, camera_location, unreal.Rotator(0, 0, 0))
    camera.set_actor_label("Soul_Founder_Overmap_Camera", False)
    camera.set_actor_rotation(
        unreal.MathLibrary.find_look_at_rotation(camera_location, center),
        False,
    )
    try:
        camera.get_component_by_class(unreal.CameraComponent).set_editor_property("field_of_view", 48.0)
    except Exception:
        pass
    set_tags(camera, ["overmap.camera", "proof_only"])

    light = actors.spawn_actor_from_class(
        unreal.DirectionalLight,
        unreal.Vector(0, 0, 200000),
        unreal.Rotator(-55, -25, 0),
    )
    light.set_actor_label("Soul_Founder_Overmap_KeyLight", False)
    set_tags(light, ["overmap.light", "proof_only"])
    try:
        light.get_component_by_class(unreal.DirectionalLightComponent).set_editor_property("intensity", 8.0)
    except Exception:
        pass

    unreal.EditorLevelLibrary.save_current_level()

    manifest = {
        "schema": 1,
        "status": "STRUCTURAL_MAP_STAGED",
        "map": MAP_DEST,
        "source_bundle": str(BUNDLE),
        "source_fixture_file": str(FIXTURES),
        "regions_spawned": len(region_actors),
        "routes_spawned": len(bundle["routes"]),
        "route_segments_spawned": route_segment_count,
        "acceptance_fixture_ids": [x["id"] for x in fixtures["fixtures"]],
        "proof_only_assets": [CUBE, CYLINDER, SPHERE],
        "shipping_art_claim": False,
        "notes": [
            "Structural Engine BasicShapes only; donor/production art is intentionally absent.",
            "SoulCore remains campaign authority; this level is presentation staging.",
            "RB Weather remains weather authority and is not replaced here.",
            "Run the six acceptance fixtures before treating the map as qualified.",
        ],
    }
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "founder_overmap_structural_ue_manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n",
        encoding="utf-8",
    )
    log("DONE " + MAP_DEST + " " + json.dumps(manifest))

try:
    main()
except Exception:
    OUT.mkdir(parents=True, exist_ok=True)
    error = traceback.format_exc()
    unreal.log_error("SOUL_OVERMAP_STAGE_ERROR\n" + error)
    (OUT / "founder_overmap_structural_ue_error.txt").write_text(error, encoding="utf-8")
    raise
