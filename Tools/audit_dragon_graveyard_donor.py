import json
from collections import Counter
from pathlib import Path
import unreal

MAP = "/Game/Dragon_graveyard/Level/L_showcase_level"
OUT = Path(r"D:\RefinedBadger\Worktrees\Soul-dragon-graveyard-proof-20260922\Evidence\DragonGraveyard\donor_audit.json")

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not levels.load_level(MAP):
    raise RuntimeError("Could not load Dragon Graveyard donor map")

world = unreal.EditorLevelLibrary.get_editor_world()
if not world:
    raise RuntimeError("Dragon Graveyard editor world unavailable")

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
rows = []
classes = Counter()
for actor in actors:
    if not actor:
        continue
    cls = actor.get_class().get_name()
    classes[cls] += 1
    try:
        origin, extent = actor.get_actor_bounds(False, True)
        bounds = [origin.x, origin.y, origin.z, extent.x, extent.y, extent.z]
    except Exception:
        bounds = None
    rows.append({
        "label": actor.get_actor_label(),
        "class": cls,
        "location": [
            actor.get_actor_location().x,
            actor.get_actor_location().y,
            actor.get_actor_location().z,
        ],
        "bounds": bounds,
    })

def class_rows(name):
    return [r for r in rows if r["class"] == name]

landscape_rows = [r for r in rows if "Landscape" in r["class"]]
player_starts = [r for r in rows if r["class"] == "PlayerStart"]
nav_bounds = [r for r in rows if "NavMeshBoundsVolume" in r["class"]]
blocking_volumes = [r for r in rows if "BlockingVolume" in r["class"]]

finite_bounds = [r["bounds"] for r in rows if r["bounds"]]
if finite_bounds:
    mins = [
        min(b[i] - b[i+3] for b in finite_bounds)
        for i in range(3)
    ]
    maxs = [
        max(b[i] + b[i+3] for b in finite_bounds)
        for i in range(3)
    ]
else:
    mins = [0, 0, 0]
    maxs = [0, 0, 0]
large_static = []
for r in rows:
    if r["class"] not in {"StaticMeshActor", "InstancedFoliageActor"} or not r["bounds"]:
        continue
    ex, ey, ez = r["bounds"][3:]
    footprint = ex * ey
    if footprint > 2_500_000:
        large_static.append({
            "label": r["label"],
            "location": r["location"],
            "extent": [ex, ey, ez],
            "footprint": footprint,
        })
large_static.sort(key=lambda x: x["footprint"], reverse=True)

near_origin = []
for r in rows:
    x, y, z = r["location"]
    if x*x + y*y <= 6000*6000 and abs(z) < 5000:
        near_origin.append({
            "label": r["label"],
            "class": r["class"],
            "location": r["location"],
            "bounds": r["bounds"],
        })

report = {
    "schema": 1,
    "map": MAP,
    "actor_count": len(rows),
    "class_counts": dict(classes.most_common()),
    "world_bounds": {"min": mins, "max": maxs},
    "landscapes": landscape_rows,
    "player_starts": player_starts,
    "nav_bounds": nav_bounds,
    "blocking_volumes": blocking_volumes,
    "largest_static_actors": large_static[:30],
    "actors_near_origin": near_origin[:100],
}
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
unreal.log("SOUL_DRAGON_DONOR_AUDIT " + json.dumps({
    "actors": report["actor_count"],
    "landscapes": len(landscape_rows),
    "player_starts": len(player_starts),
    "nav_bounds": len(nav_bounds),
    "near_origin": len(near_origin),
    "bounds": report["world_bounds"],
}, sort_keys=True))
print(json.dumps({
    "actors": report["actor_count"],
    "landscapes": len(landscape_rows),
    "player_starts": len(player_starts),
    "nav_bounds": len(nav_bounds),
    "bounds": report["world_bounds"],
}, indent=2))
