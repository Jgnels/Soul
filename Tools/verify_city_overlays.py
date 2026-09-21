import unreal, json, os, traceback

OUT = r"D:\RefinedBadger\Games\Soul\Evidence"
MANIFEST = os.path.join(OUT, "city_overlay_verify_manifest.json")
CONFIGS = [
    ("/Game/Soul/Maps/Cities/LV_Soul_HumanCapital_Overlay",
     "/Game/Medieval_Megapack/Levels/PL_Fortress_Day", 12, 4),
    ("/Game/Soul/Maps/Cities/LV_Soul_VikingHarbour_Overlay",
     "/Game/JustBStudios/Water_City/Levels/LV_WaterVillage", 12, 4),
]
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

def all_of(world, cls):
    return unreal.GameplayStatics.get_all_actors_of_class(world, cls)

def level_paths(world):
    paths = []
    for level in unreal.EditorLevelUtils.get_levels(world):
        try:
            paths.append(level.get_outer().get_path_name())
        except Exception:
            paths.append(str(level))
    return paths

def actor_level_path(actor):
    try:
        return actor.get_level().get_outer().get_path_name()
    except Exception:
        return None

rows = []
errors = []
for map_path, donor_path, expected_buildings, expected_objectives in CONFIGS:
    try:
        ok = bool(levels.load_level(map_path))
        if not ok:
            raise RuntimeError("load failed " + map_path)
        world = unreal.EditorLevelLibrary.get_editor_world()
        paths = level_paths(world)
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
        classes = [
            unreal.SoulSettlementPresentationController,
            unreal.SoulSettlementBootstrapActor,
            unreal.SoulTownViewAnchor,
            unreal.SoulBattlefieldLayoutActor,
            unreal.SoulSettlementBuildingActor,
            unreal.SoulFortificationSegmentActor,
            unreal.SoulSiegeObjectiveActor,
        ]
        soul_actor_levels = []
        for cls in classes:
            for actor in all_of(world, cls):
                soul_actor_levels.append({
                    "label": actor.get_actor_label(),
                    "class": cls.get_name(),
                    "level": actor_level_path(actor),
                })
        row = {
            "map": map_path,
            "donor": donor_path,
            "load_ok": True,
            "level_paths": paths,
            "streaming_paths": streaming_paths,
            "soul_actor_levels": soul_actor_levels,
            "presentation_count": len(all_of(world, unreal.SoulSettlementPresentationController)),
            "bootstrap_count": len(all_of(world, unreal.SoulSettlementBootstrapActor)),
            "town_view_count": len(all_of(world, unreal.SoulTownViewAnchor)),
            "layout_count": len(all_of(world, unreal.SoulBattlefieldLayoutActor)),
            "building_count": len(all_of(world, unreal.SoulSettlementBuildingActor)),
            "wall_count": len(all_of(world, unreal.SoulFortificationSegmentActor)),
            "objective_count": len(all_of(world, unreal.SoulSiegeObjectiveActor)),
        }
        donor_leaf = donor_path.rsplit("/", 1)[-1]
        row["donor_loaded"] = any(
            donor_leaf.lower() in x.lower()
            for x in (row["level_paths"] + row["streaming_paths"])
        )
        map_leaf = map_path.rsplit("/", 1)[-1].lower()
        row["misplaced_soul_actors"] = [
            item for item in row["soul_actor_levels"]
            if not item["level"] or map_leaf not in item["level"].lower()
        ]
        required = {
            "presentation_count": 1,
            "bootstrap_count": 1,
            "town_view_count": 1,
            "layout_count": 1,
            "building_count": expected_buildings,
            "wall_count": 1,
            "objective_count": expected_objectives,
        }
        mismatches = {k: {"expected": v, "actual": row[k]} for k, v in required.items() if row[k] != v}
        if not row["donor_loaded"]:
            mismatches["donor_loaded"] = {"expected": True, "actual": False}
        if row["misplaced_soul_actors"]:
            mismatches["soul_actor_ownership"] = {
                "expected": "all Soul actors owned by " + map_path,
                "actual": row["misplaced_soul_actors"],
            }
        row["mismatches"] = mismatches
        row["pass"] = not mismatches
        rows.append(row)
        unreal.log("SOUL_CITY_OVERLAY_VERIFY " + json.dumps(row, sort_keys=True))
        if mismatches:
            errors.append({"map": map_path, "mismatches": mismatches})
    except Exception:
        err = traceback.format_exc()
        unreal.log_error("SOUL_CITY_OVERLAY_VERIFY_ERROR\n" + err)
        errors.append({"map": map_path, "error": err})

with open(MANIFEST, "w", encoding="utf-8") as f:
    json.dump({"overlays": rows, "errors": errors, "pass": not errors}, f, indent=2)

if errors:
    raise RuntimeError("City overlay verification failed; see manifest")
