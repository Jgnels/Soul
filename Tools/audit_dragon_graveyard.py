import json
import os
import unreal

OUT = r"D:\RefinedBadger\Worktrees\Soul-battlefield-environments-20260922\Evidence\EnvironmentCaptures\DragonGraveyard\dragon_graveyard_audit.json"
os.makedirs(os.path.dirname(OUT), exist_ok=True)
MAP = "/Game/Dragon_graveyard/Level/L_showcase_level"

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
result = {"schema": 1, "map": MAP, "loaded": False}

try:
    result["loaded"] = bool(levels.load_level(MAP))
    rows = actors.get_all_level_actors() if result["loaded"] else []
    result["actor_count"] = len(rows)
    counts = {}
    bounds_rows = []
    starts = []
    nav = []
    for actor in rows:
        cls = actor.get_class().get_name()
        counts[cls] = counts.get(cls, 0) + 1
        label = actor.get_actor_label()
        if cls == "PlayerStart":
            p = actor.get_actor_location()
            starts.append([p.x, p.y, p.z])
        if "NavMeshBoundsVolume" in cls:
            nav.append(label)
        try:
            origin, extent = actor.get_actor_bounds(False, True)
            if max(abs(extent.x), abs(extent.y), abs(extent.z)) > 2000:
                bounds_rows.append({
                    "label": label, "class": cls,
                    "origin": [origin.x, origin.y, origin.z],
                    "extent": [extent.x, extent.y, extent.z],
                })
        except Exception:
            pass
    result["class_counts"] = dict(sorted(counts.items()))
    result["player_starts"] = starts
    result["nav_bounds"] = nav
    result["large_actor_bounds"] = bounds_rows[:120]
    result["combat_support"] = {
        "has_landscape": counts.get("Landscape", 0) > 0,
        "has_nav_bounds": len(nav) > 0,
        "has_player_start": len(starts) > 0,
    }
except Exception as exc:
    result["error"] = repr(exc)

with open(OUT, "w", encoding="utf-8") as f:
    json.dump(result, f, indent=2)
unreal.log("SOUL_DRAGON_GRAVEYARD_AUDIT_PASS " + OUT)
