import unreal, os, json, traceback

OUT = r"D:\RefinedBadger\Games\Soul\Evidence"
MANIFEST = os.path.join(OUT, "battlefield_overlay_build_manifest.json")
RECIPES = [
    {"id":"human.grassland_crossroads","src":"/Game/LandscapePackTwo/Maps/Grassland_01","dst":"/Game/Soul/Maps/Battlefields/Overlays/BF_Human_GrasslandCrossroads_Overlay","biome":"temperate","landform":"rolling_plain","feature":"crossroads"},
    {"id":"human.rolling_ridge","src":"/Game/LandscapePackTwo/Maps/Grassland_02","dst":"/Game/Soul/Maps/Battlefields/Overlays/BF_Human_RollingRidge_Overlay","biome":"temperate","landform":"ridge","feature":"high_ground"},
    {"id":"viking.snow_pass","src":"/Game/LandscapePackOne/Maps/SnowyMountain_01","dst":"/Game/Soul/Maps/Battlefields/Overlays/BF_Viking_SnowPass_Overlay","biome":"snow","landform":"mountain_pass","feature":"narrow_pass"},
    {"id":"dwarf.mountain_pass","src":"/Game/LandscapePackOne/Maps/Mountain_01","dst":"/Game/Soul/Maps/Battlefields/Overlays/BF_Dwarf_MountainPass_Overlay","biome":"mountain","landform":"pass","feature":"switchback"},
    {"id":"orc.badlands","src":"/Game/LandscapePackTwo/Maps/Mesa_01","dst":"/Game/Soul/Maps/Battlefields/Overlays/BF_Orc_Badlands_Overlay","biome":"badlands","landform":"mesa","feature":"dry_gully"},
    {"id":"dark.ash_plain","src":"/Game/LandscapePackTwo/Maps/Desert_01","dst":"/Game/Soul/Maps/Battlefields/Overlays/BF_Dark_AshPlain_Overlay","biome":"ash_waste","landform":"open_plain","feature":"dead_ground"},
    {"id":"neutral.coastal_ruins","src":"/Game/Elite_CoastalRuins/Maps/CoastalRuins_01","dst":"/Game/Soul/Maps/Battlefields/Overlays/BF_Neutral_CoastalRuins_Overlay","biome":"coast","landform":"valley_hill","feature":"ruins"},
]
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
def n(value):
    return unreal.Name(value)

def actor_center(world):
    landscapes = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Landscape)
    if landscapes:
        origin, _ = landscapes[0].get_actor_bounds(False, True)
        return origin, "Landscape"
    starts = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PlayerStart)
    if starts:
        return starts[0].get_actor_location(), "PlayerStart"
    return unreal.Vector(0, 0, 0), "Origin"

def configure(layout, grid, recipe):
    layout.set_actor_label("Soul_BattlefieldLayout", False)
    layout.set_editor_property("recipe_id", n(recipe["id"]))
    layout.set_editor_property("biome", n(recipe["biome"]))
    layout.set_editor_property("landform", n(recipe["landform"]))
    layout.set_editor_property("strategic_feature", n(recipe["feature"]))
    layout.set_editor_property("hex_cell_size", 300.0)
    layout.set_editor_property("board_radius", 8)
    grid.set_actor_label("Soul_HiddenHexGrid", False)
    grid.set_editor_property("hex_cell_size", 300.0)
    grid.set_editor_property("board_radius", 8)
    grid.set_editor_property("trace_height", 15000.0)
    grid.set_editor_property("trace_depth", 30000.0)

def cells_valid(grid):
    count = 0
    for cell in grid.get_editor_property("baked_cells"):
        try:
            valid = bool(cell.get_editor_property("valid_surface"))
        except Exception:
            valid = bool(getattr(cell, "valid_surface", False))
        if valid:
            count += 1
    return count
def loaded_level_paths(world):
    result = []
    for level in unreal.EditorLevelUtils.get_levels(world):
        try:
            result.append(level.get_outer().get_path_name())
        except Exception:
            result.append(str(level))
    return result

def inspect_loaded(recipe):
    world = unreal.EditorLevelLibrary.get_editor_world()
    layouts = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SoulBattlefieldLayoutActor)
    grids = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SoulBattlefieldGridActor)
    paths = loaded_level_paths(world)
    streaming_paths = []
    try:
        for streaming in world.get_streaming_levels():
            if not streaming:
                continue
            try:
                streaming_paths.append(str(streaming.get_world_asset_package_name()))
            except Exception:
                streaming_paths.append(str(streaming))
    except Exception:
        pass
    donor_leaf = recipe["src"].rsplit("/", 1)[-1].lower()
    row = {
        "layout_count": len(layouts),
        "grid_count": len(grids),
        "loaded_levels": paths,
        "streaming_levels": streaming_paths,
        "donor_loaded": any(donor_leaf in value.lower() for value in (paths + streaming_paths)),
    }
    if len(grids) == 1:
        row["baked_cells"] = len(grids[0].get_editor_property("baked_cells"))
        row["expected_cells"] = int(grids[0].get_expected_cell_count())
        row["valid_surface_cells"] = cells_valid(grids[0])
    return row

def disk_bytes(map_path):
    content = unreal.SystemLibrary.get_project_content_directory()
    rel = map_path.replace("/Game/", "").replace("/", os.sep) + ".umap"
    path = os.path.join(content, rel)
    return os.path.getsize(path) if os.path.exists(path) else None
rows = []
errors = []
for recipe in RECIPES:
    row = dict(recipe)
    try:
        exists = unreal.EditorAssetLibrary.does_asset_exist(recipe["dst"])
        row["preexisting"] = bool(exists)
        if exists and not unreal.EditorAssetLibrary.delete_asset(recipe["dst"]):
            raise RuntimeError("Could not replace existing overlay " + recipe["dst"])
        if not levels.new_level(recipe["dst"]):
            raise RuntimeError("Could not create overlay " + recipe["dst"])
        world = unreal.EditorLevelLibrary.get_editor_world()
        layout = actors.spawn_actor_from_class(unreal.SoulBattlefieldLayoutActor, unreal.Vector(), unreal.Rotator())
        grid = actors.spawn_actor_from_class(unreal.SoulBattlefieldGridActor, unreal.Vector(), unreal.Rotator())
        if not layout or not grid:
            raise RuntimeError("Could not spawn Soul battlefield actors")
        configure(layout, grid, recipe)
        stream = unreal.EditorLevelUtils.add_level_to_world(world, recipe["src"], unreal.LevelStreamingAlwaysLoaded)
        if not stream:
            raise RuntimeError("Could not stream donor " + recipe["src"])
        center, source = actor_center(world)
        row["anchor_source"] = source
        row["anchor"] = [center.x, center.y, center.z]
        layout.set_actor_location(center, False, False)
        grid.set_actor_location(center, False, False)
        grid.bake_grid()
        if not unreal.EditorLoadingAndSavingUtils.save_map(world, recipe["dst"]):
            raise RuntimeError("Could not save overlay " + recipe["dst"])
        row["mode"] = "created"
        if not levels.load_level(recipe["dst"]):
            raise RuntimeError("Could not reload overlay " + recipe["dst"])
        row.update(inspect_loaded(recipe))
        row["disk_bytes"] = disk_bytes(recipe["dst"])
        if row["layout_count"] != 1 or row["grid_count"] != 1:
            raise RuntimeError("Soul battlefield actor persistence mismatch")
        if row.get("baked_cells") != row.get("expected_cells") or row.get("baked_cells") != 217:
            raise RuntimeError("Baked grid persistence mismatch")
        if not row["donor_loaded"]:
            raise RuntimeError("Streamed donor missing after reload")
        if row.get("valid_surface_cells", 0) <= 0:
            raise RuntimeError("Baked grid did not hit streamed donor terrain")
        row["pass"] = True
        rows.append(row)
        unreal.log("SOUL_BF_OVERLAY PASS " + json.dumps({
            "id": recipe["id"],
            "bytes": row["disk_bytes"],
            "valid_surface_cells": row["valid_surface_cells"],
        }, sort_keys=True))
    except Exception:
        err = traceback.format_exc()
        row["pass"] = False
        row["error"] = err
        rows.append(row)
        errors.append({"id": recipe["id"], "error": err})
        unreal.log_error("SOUL_BF_OVERLAY ERROR " + recipe["id"] + "\n" + err)

with open(MANIFEST, "w", encoding="utf-8") as handle:
    json.dump({"overlays": rows, "errors": errors, "pass": not errors}, handle, indent=2)

if errors:
    raise RuntimeError("One or more battlefield overlay proofs failed; see manifest")
