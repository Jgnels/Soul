import unreal, json, os, traceback

OUT = r"D:\RefinedBadger\Games\Soul\Evidence"
MANIFEST = os.path.join(OUT, "donor_clean_verify_manifest.json")
MAPS = [
    ("human", "/Game/Medieval_Megapack/Levels/PL_Fortress_Day"),
    ("viking", "/Game/JustBStudios/Water_City/Levels/LV_WaterVillage"),
]
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

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
        offenders = [
            {"label": a.get_actor_label(), "class": a.get_class().get_name()}
            for a in actors.get_all_level_actors() if is_soul_actor(a)
        ]
        row = {
            "id": key,
            "map": map_path,
            "soul_actor_count": len(offenders),
            "soul_actors": offenders,
            "pass": len(offenders) == 0,
        }
        rows.append(row)
        unreal.log("SOUL_DONOR_CLEAN_VERIFY " + json.dumps(row, sort_keys=True))
        if offenders:
            errors.append({"id": key, "map": map_path, "soul_actors": offenders})
    except Exception:
        err = traceback.format_exc()
        errors.append({"id": key, "map": map_path, "error": err})
        unreal.log_error("SOUL_DONOR_CLEAN_VERIFY_ERROR " + key + "\n" + err)

passed = len(rows) == len(MAPS) and not errors and all(row["pass"] for row in rows)
with open(MANIFEST, "w", encoding="utf-8") as handle:
    json.dump({"maps": rows, "errors": errors, "pass": passed}, handle, indent=2)

if not passed:
    raise RuntimeError("Donor cleanliness verification failed; see manifest")
unreal.log("SOUL_DONOR_CLEAN_VERIFY DONE")
