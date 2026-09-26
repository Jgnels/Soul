import json, math
from pathlib import Path
import unreal

MAP = "/Game/Dragon_graveyard/Level/L_showcase_level"
OUT = Path(r"D:\RefinedBadger\Worktrees\Soul-dragon-graveyard-proof-20260922\Evidence\DragonGraveyard\battle_zone_analysis.json")

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not levels.load_level(MAP):
    raise RuntimeError("Could not load Dragon Graveyard donor map")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()

obstacles = []
landmarks = []
for actor in actors:
    if not actor:
        continue
    cls = actor.get_class().get_name()
    if cls not in {"StaticMeshActor", "BP_bones_C"}:
        continue
    origin, extent = actor.get_actor_bounds(False, True)
    label = actor.get_actor_label()
    row = {
        "label": label, "class": cls,
        "x": origin.x, "y": origin.y, "z": origin.z,
        "ex": extent.x, "ey": extent.y, "ez": extent.z,
    }
    lower = label.lower()
    if "bone" in lower or "skeleton" in lower or "dragon_kit" in lower:
        landmarks.append(row)
    # Ignore essentially-flat atmosphere/shadow planes when scoring physical obstruction.
    if extent.z >= 40.0:
        obstacles.append(row)
HALF_X = 2600.0
HALF_Y = 2200.0

def intersection_area(cx, cy, row):
    left = max(cx - HALF_X, row["x"] - row["ex"])
    right = min(cx + HALF_X, row["x"] + row["ex"])
    low = max(cy - HALF_Y, row["y"] - row["ey"])
    high = min(cy + HALF_Y, row["y"] + row["ey"])
    if right <= left or high <= low:
        return 0.0
    return (right - left) * (high - low)

def landmark_distance(cx, cy):
    if not landmarks:
        return 999999.0
    return min(math.hypot(cx-r["x"], cy-r["y"]) for r in landmarks)

candidates = []
for cx in range(-35000, 35001, 2500):
    for cy in range(-35000, 35001, 2500):
        overlap_rows = []
        overlap_area = 0.0
        for row in obstacles:
            area = intersection_area(cx, cy, row)
            if area > 0:
                overlap_area += area
                overlap_rows.append((area, row))
        battle_area = (HALF_X * 2) * (HALF_Y * 2)
        blocked_fraction = min(1.0, overlap_area / battle_area)
        ldist = landmark_distance(cx, cy)
        # Prefer an open pocket that is still visually inside the graveyard.
        landmark_penalty = abs(ldist - 9000.0) / 9000.0
        edge_penalty = max(0.0, (abs(cx)-30000)/5000) + max(0.0, (abs(cy)-30000)/5000)
        score = blocked_fraction * 10.0 + landmark_penalty + edge_penalty
        candidates.append({
            "center": [cx, cy, 0],
            "blocked_fraction_upper_bound": round(blocked_fraction, 4),
            "overlap_actor_count": len(overlap_rows),
            "nearest_landmark_cm": round(ldist, 1),
            "score": round(score, 4),
            "largest_overlaps": [
                {
                    "label": r["label"],
                    "class": r["class"],
                    "intersection_area": round(area, 1),
                    "actor_location": [round(r["x"],1), round(r["y"],1), round(r["z"],1)],
                    "actor_extent": [round(r["ex"],1), round(r["ey"],1), round(r["ez"],1)],
                }
                for area, r in sorted(overlap_rows, key=lambda item: item[0], reverse=True)[:8]
            ],
        })
candidates.sort(key=lambda r: (r["score"], r["blocked_fraction_upper_bound"], r["nearest_landmark_cm"]))
report = {
    "schema": 1,
    "map": MAP,
    "battle_half_extent_cm": [HALF_X, HALF_Y],
    "physical_obstacle_actor_count": len(obstacles),
    "landmark_actor_count": len(landmarks),
    "top_candidates": candidates[:30],
}
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
print(json.dumps({
    "obstacles": len(obstacles),
    "landmarks": len(landmarks),
    "top": report["top_candidates"][:10],
}, indent=2))
