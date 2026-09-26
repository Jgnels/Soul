import json, math
from pathlib import Path
import unreal

MAP = "/Game/Dragon_graveyard/Level/L_showcase_level"
OUT = Path(r"D:\RefinedBadger\Worktrees\Soul-dragon-graveyard-proof-20260922\Evidence\DragonGraveyard\battle_zone_visual_analysis.json")
HALF_X = 2200.0
HALF_Y = 1800.0

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
    row = {"label": label, "class": cls, "x": origin.x, "y": origin.y, "z": origin.z,
           "ex": extent.x, "ey": extent.y, "ez": extent.z}
    lower = label.lower()
    if cls == "BP_bones_C" or any(k in lower for k in ("bone", "skeleton", "skull", "dragon_kit", "horn")):
        landmarks.append(row)
    if extent.z >= 40.0:
        obstacles.append(row)

def intersection_area(cx, cy, row):
    left = max(cx - HALF_X, row["x"] - row["ex"])
    right = min(cx + HALF_X, row["x"] + row["ex"])
    low = max(cy - HALF_Y, row["y"] - row["ey"])
    high = min(cy + HALF_Y, row["y"] + row["ey"])
    if right <= left or high <= low:
        return 0.0
    return (right - left) * (high - low)

def distance(cx, cy, row):
    return math.hypot(cx-row["x"], cy-row["y"])

battle_area = (HALF_X * 2) * (HALF_Y * 2)
candidates = []
for cx in range(-15000, 15001, 1000):
    for cy in range(-15000, 15001, 1000):
        overlaps = []
        overlap_area = 0.0
        for row in obstacles:
            area = intersection_area(cx, cy, row)
            if area > 0:
                overlap_area += area
                overlaps.append((area, row))
        blocked = min(1.0, overlap_area / battle_area)
        distances = sorted((distance(cx, cy, r), r) for r in landmarks)
        nearest = distances[0][0] if distances else 999999.0
        visual_count = sum(1 for dist, _ in distances if dist <= 12000.0)
        target_penalty = abs(nearest - 5500.0) / 5500.0
        score = blocked * 14.0 + target_penalty - min(visual_count, 12) * 0.06
        candidates.append({
            "center": [cx, cy, 0],
            "blocked_fraction_upper_bound": round(blocked, 4),
            "nearest_landmark_cm": round(nearest, 1),
            "landmarks_within_12000_cm": visual_count,
            "score": round(score, 4),
            "nearest_landmarks": [
                {"distance_cm": round(dist,1), "label": r["label"], "class": r["class"],
                 "location": [round(r["x"],1), round(r["y"],1), round(r["z"],1)]}
                for dist, r in distances[:8]
            ],
            "largest_overlaps": [
                {"intersection_area": round(area,1), "label": r["label"], "class": r["class"],
                 "location": [round(r["x"],1), round(r["y"],1), round(r["z"],1)],
                 "extent": [round(r["ex"],1), round(r["ey"],1), round(r["ez"],1)]}
                for area, r in sorted(overlaps, key=lambda item: item[0], reverse=True)[:8]
            ],
        })

usable = [r for r in candidates
          if r["blocked_fraction_upper_bound"] <= 0.20
          and r["nearest_landmark_cm"] <= 10000.0
          and r["landmarks_within_12000_cm"] >= 2]
usable.sort(key=lambda r: (r["score"], r["blocked_fraction_upper_bound"], r["nearest_landmark_cm"]))
report = {
    "schema": 1,
    "map": MAP,
    "battle_half_extent_cm": [HALF_X, HALF_Y],
    "physical_obstacle_actor_count": len(obstacles),
    "landmark_actor_count": len(landmarks),
    "selection_rule": "blocked<=0.20, nearest landmark<=10000cm, >=2 landmarks within 12000cm",
    "top_visual_candidates": usable[:40],
}
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
print(json.dumps({
    "obstacles": len(obstacles),
    "landmarks": len(landmarks),
    "usable": len(usable),
    "top": report["top_visual_candidates"][:12],
}, indent=2))
