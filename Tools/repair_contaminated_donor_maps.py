import unreal, json, os, traceback

OUT = r"D:\RefinedBadger\Games\Soul\Evidence"
MANIFEST = os.path.join(OUT, "donor_contamination_repair_manifest.json")
MAPS = [
    ("human", "/Game/Medieval_Megapack/Levels/PL_Fortress_Day"),
    ("viking", "/Game/JustBStudios/Water_City/Levels/LV_WaterVillage"),
]
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

def is_soul_actor(actor):
    label = actor.get_actor_label() or ""
    cls = actor.get_class().get_name() or ""
    return label.startswith("Soul_") or label.startswith("SoulGeom_") or cls.startswith("Soul")

rows = []
errors = []
for key, map_path in MAPS:
    try:
        if not levels.load_level(map_path):
            raise RuntimeError("Could not load " + map_path)
        before = actors.get_all_level_actors()
        offenders = [a for a in before if is_soul_actor(a)]
        removed = [{"label": a.get_actor_label(), "class": a.get_class().get_name()} for a in offenders]
        for actor in offenders:
            if not actors.destroy_actor(actor):
                raise RuntimeError("Could not destroy " + actor.get_actor_label())
        if offenders and not unreal.EditorLoadingAndSavingUtils.save_current_level():
            raise RuntimeError("Could not save repaired donor " + map_path)
        if not levels.load_level(map_path):
            raise RuntimeError("Could not reload repaired donor " + map_path)
        remaining = [a for a in actors.get_all_level_actors() if is_soul_actor(a)]
        row = {
            "id": key,
            "map": map_path,
            "removed_count": len(removed),
            "removed": removed,
            "remaining_soul_actor_count": len(remaining),
            "remaining": [{"label": a.get_actor_label(), "class": a.get_class().get_name()} for a in remaining],
            "pass": len(remaining) == 0,
        }
        rows.append(row)
        unreal.log("SOUL_DONOR_REPAIR " + json.dumps(row, sort_keys=True))
        if remaining:
            raise RuntimeError("Soul actors remain in repaired donor " + map_path)
    except Exception:
        err = traceback.format_exc()
        errors.append({"id": key, "map": map_path, "error": err})
        unreal.log_error("SOUL_DONOR_REPAIR_ERROR " + key + "\n" + err)
with open(MANIFEST, "w", encoding="utf-8") as handle:
    json.dump({"maps": rows, "errors": errors, "pass": not errors and all(r["pass"] for r in rows)}, handle, indent=2)

if errors or not rows or not all(r["pass"] for r in rows):
    raise RuntimeError("Donor contamination repair failed; see manifest")

unreal.log("SOUL_DONOR_REPAIR DONE")
