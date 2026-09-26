"""Read-only editor collision audit matching Soul's ECC_Visibility spawn trace.
No assets are saved, mutated, hidden, spawned, or removed. No gameplay runs.
"""
import json
import math
from datetime import datetime, timezone
from pathlib import Path
import unreal

MAP = "/Game/Dragon_graveyard/Level/L_showcase_level"
OUT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())) / "Evidence" / "DragonGroundContract.json"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not levels.load_level(MAP):
    raise RuntimeError("Could not load real Dragon Graveyard")
world = unreal.EditorLevelLibrary.get_editor_world()
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
landmarks = []
lava = []
for actor in actors:
    label = actor.get_actor_label()
    bounds, extent = actor.get_actor_bounds(False, True)
    if any(word in label.lower() for word in ("bone", "skeleton", "dragon_kit")):
        landmarks.append({"label": label, "center": [bounds.x, bounds.y, bounds.z]})
    for comp in actor.get_components_by_class(unreal.StaticMeshComponent):
        materials = [comp.get_material(i) for i in range(comp.get_num_materials())]
        paths = [m.get_path_name() for m in materials if m]
        if any("lava" in name.lower() for name in paths):
            lava.append({"label": label, "component": comp.get_path_name(), "center": [bounds.x, bounds.y, bounds.z], "extent": [extent.x, extent.y, extent.z], "materials": paths})

trace_errors = []
binding_probe = {}
cache = {}

def vec(v):
    return [round(v.x, 3), round(v.y, 3), round(v.z, 3)]


def trace(x, y):
    key = (x, y)
    if key in cache:
        return cache[key]
    row = {"xy": [x, y], "blocking_hit": False, "dry_flat_candidate": False}
    overlaps = [p["label"] for p in lava if abs(x-p["center"][0]) <= p["extent"][0] and abs(y-p["center"][1]) <= p["extent"][1]]
    row["lava_xy_bounds_overlap"] = overlaps
    try:
        # TraceTypeQuery1 is the default Visibility mapping used by the project.
        raw = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, 8000), unreal.Vector(x, y, -12000), unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
        hit = raw if isinstance(raw, unreal.HitResult) else next((v for v in raw if isinstance(v, unreal.HitResult)), None) if isinstance(raw, (tuple, list)) else None
        if hit is not None:
            if not binding_probe:
                binding_probe.update({"type": str(type(hit)), "repr": repr(hit)[:1800], "public_members": [name for name in dir(hit) if not name.startswith("_")], "native_break_schema": ["blocking_hit", "initial_overlap", "time", "distance", "location", "impact_point", "normal", "impact_normal", "phys_mat", "hit_actor", "hit_component", "hit_bone_name", "bone_name", "hit_item", "element_index", "face_index", "trace_start", "trace_end"]})
            values = hit.to_tuple()
            binding_probe["tuple_length"] = len(values)
            if len(values) != 18:
                binding_probe["tuple_repr"] = repr(values)[:2400]
                raise RuntimeError("HitResult native-break tuple schema differs from UE5.8 header: expected 18 fields, got " + str(len(values)))
            if not all(hasattr(values[i], "z") for i in (4,5,6,7,16,17)):
                raise RuntimeError("HitResult native-break tuple vector positions do not match verified UE5.8 schema")
            row["blocking_hit"] = bool(values[0])
            row["initial_overlap"] = bool(values[1])
            row["impact"] = vec(values[5])
            row["normal"] = vec(values[7])
            actor, component = values[9], values[10]
            row["actor"] = actor.get_actor_label() if actor else None
            row["actor_class"] = actor.get_class().get_name() if actor else None
            row["component"] = component.get_path_name() if component else None
            if component and hasattr(component, "get_num_materials"):
                row["materials"] = [m.get_path_name() for i in range(component.get_num_materials()) if (m := component.get_material(i))]
            else:
                row["materials"] = []
            row["hit_lava_material"] = any("lava" in p.lower() for p in row["materials"])
        elif raw is not None:
            row["unparsed_return_type"] = str(type(raw))
        row["dry_flat_candidate"] = bool(row["blocking_hit"] and not overlaps and not row.get("hit_lava_material") and row.get("normal", [0, 0, 0])[2] >= 0.85 and not row.get("initial_overlap"))
    except Exception as exc:
        row["error"] = str(exc)
        trace_errors.append({"xy": [x, y], "error": str(exc)})
    cache[key] = row
    return row

current = [trace(2000+dx, -15000+dy) for dx, dy in ((0, 0),(-700, 0),(700, 0),(-700,-600),(-700,600),(700,-600),(700,600),(-1800,0),(1800,0))]
# First establish the API/physics scene works; do not generate false-safe rows.
if all("error" in r for r in current):
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps({"map": MAP, "binding_probe": binding_probe, "current_origin_samples": current, "errors": trace_errors}, indent=2), encoding="utf-8")
    raise RuntimeError("All initial ground traces failed; see DragonGroundContract.json")

candidates = []
for cx in range(-20000, 20001, 2000):
    for cy in range(-20000, 20001, 2000):
        nearest = sorted(({"label": p["label"], "distance_cm": round(math.hypot(cx-p["center"][0], cy-p["center"][1]), 1), "center": p["center"]} for p in landmarks), key=lambda p:p["distance_cm"])
        if not nearest or nearest[0]["distance_cm"] > 9000:
            continue
        center = trace(cx, cy)
        if not center["dry_flat_candidate"]:
            continue
        samples = [trace(cx+dx, cy+dy) for dx in (-1800,-900,0,900,1800) for dy in (-1300,-650,0,650,1300)]
        dry = sum(r["dry_flat_candidate"] for r in samples)
        heights = [r["impact"][2] for r in samples if r["blocking_hit"]]
        span = max(heights)-min(heights) if heights else None
        row = {"center": [cx,cy,center["impact"][2]], "dry_flat_samples": dry, "sample_count": len(samples), "height_span_cm": round(span,3) if span is not None else None, "nearest_landmarks": nearest[:4], "all_samples_dry_flat": dry==len(samples), "provisional_collision_candidate": dry==len(samples) and span is not None and span<=180, "samples": samples}
        candidates.append(row)
candidates.sort(key=lambda r:(not r["provisional_collision_candidate"],-r["dry_flat_samples"],r["height_span_cm"] if r["height_span_cm"] is not None else 999999,r["nearest_landmarks"][0]["distance_cm"]))
report = {"schema":1,"generated_utc":datetime.now(timezone.utc).isoformat(),"map":MAP,"mode":"read_only_editor_visibility_collision_no_asset_save","trace_start_z":8000,"trace_end_z":-12000,"trace_complex":False,"trace_channel":"TraceTypeQuery1 (project default Visibility)","sample_half_extent_cm":[1800,1300],"candidate_rule":"25/25 blocking dry samples, normal.z>=0.85, no lava material or lava XY bounds, height span<=180cm; still requires navigation and visual validation","current_origin":[2000,-15000,0],"current_origin_samples":current,"lava_planes":lava,"candidate_count":len(candidates),"provisional_candidate_count":sum(r["provisional_collision_candidate"] for r in candidates),"top_candidates":candidates[:24],"trace_count":len(cache),"binding_probe":binding_probe,"errors":trace_errors,"limitations":"Editor collision is evidence for spawn placement, not runtime navigation or graphical acceptance. Lava XY bounds rejection is conservative and separate from actual blocking hits."}
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
unreal.log("SOUL_DRAGON_GROUND_CONTRACT: "+json.dumps({"output":str(OUT),"traces":len(cache),"candidates":len(candidates),"provisional":report["provisional_candidate_count"],"errors":len(trace_errors)}))
