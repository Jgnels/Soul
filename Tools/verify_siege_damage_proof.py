import unreal, json, os, traceback

OUT = r"D:\RefinedBadger\Games\Soul\Evidence"
MAP = "/Game/Soul/Maps/Tests/LV_Soul_SiegeDamageProof"
MANIFEST = os.path.join(OUT, "siege_damage_proof_verify_manifest.json")
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

def actor_state(actor):
    return {
        "label": actor.get_actor_label(),
        "collision": bool(actor.get_actor_enable_collision()),
        "temp_hidden_editor": bool(actor.is_temporarily_hidden_in_editor(False)),
    }

def find_single(world, cls):
    found = unreal.GameplayStatics.get_all_actors_of_class(world, cls)
    if len(found) != 1:
        raise RuntimeError("expected one %s, got %d" % (cls.get_name(), len(found)))
    return found[0]

try:
    if not levels.load_level(MAP):
        raise RuntimeError("Could not load " + MAP)
    world = unreal.EditorLevelLibrary.get_editor_world()
    seg = find_single(world, unreal.SoulFortificationSegmentActor)
    intact = list(seg.get_editor_property("intact_actors"))
    breached = list(seg.get_editor_property("breached_actors"))
    if len(intact) != 1 or len(breached) != 1:
        raise RuntimeError("wall presentation references did not persist")
    rows = []
    def capture(name):
        rows.append({
            "state": name,
            "intact": actor_state(intact[0]),
            "breached": actor_state(breached[0]),
        })

    seg.apply_wall_state(1000, False)
    capture("intact")
    seg.apply_wall_state(0, False)
    capture("breached")
    seg.apply_wall_state(500, True)
    capture("repairing")
    seg.apply_wall_state(1000, False)

    expected = {
        "intact": ((True, False), (False, True)),
        "breached": ((False, True), (True, False)),
        "repairing": ((False, True), (False, True)),
    }
    mismatches = []
    for row in rows:
        a = (row["intact"]["collision"], row["intact"]["temp_hidden_editor"])
        b = (row["breached"]["collision"], row["breached"]["temp_hidden_editor"])
        if (a, b) != expected[row["state"]]:
            mismatches.append({"state": row["state"], "actual": [a, b], "expected": expected[row["state"]]})
    objective_count = len(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SoulSiegeObjectiveActor))
    result = {
        "map": MAP,
        "segment_id": str(seg.get_editor_property("segment_id")),
        "breach_scar_id": str(seg.get_editor_property("breach_scar_id")),
        "objective_count": objective_count,
        "states": rows,
        "mismatches": mismatches,
        "pass": not mismatches and objective_count == 1,
    }
    with open(MANIFEST, "w", encoding="utf-8") as f:
        json.dump(result, f, indent=2)
    unreal.log("SOUL_SIEGE_PROOF_VERIFY " + json.dumps(result, sort_keys=True))
    if not result["pass"]:
        raise RuntimeError("Siege damage proof verification failed")
except Exception:
    err = traceback.format_exc()
    unreal.log_error("SOUL_SIEGE_PROOF_VERIFY_ERROR\n" + err)
    with open(os.path.join(OUT, "siege_damage_proof_verify_error.txt"), "w", encoding="utf-8") as f:
        f.write(err)
    raise
