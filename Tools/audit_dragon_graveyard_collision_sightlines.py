import unreal, json, os

OUT = r"D:\RefinedBadger\Worktrees\Soul-battlefield-environments-20260922\Evidence\EnvironmentCaptures\DragonGraveyard\dragon_collision_sightline_audit.json"
world = unreal.EditorLevelLibrary.get_editor_world()
trace_channel = unreal.TraceTypeQuery.TRACE_TYPE_QUERY1
draw = unreal.DrawDebugTrace.NONE
rows = []
ground_hits = 0
los180 = 0
los500 = 0
los1200 = 0

def trace(start, end):
    return unreal.SystemLibrary.line_trace_single(
        world, start, end, trace_channel, True, [], draw, True)

def hit_location(hit):
    try:
        return hit.to_tuple()[4]
    except Exception:
        try:
            return hit.location
        except Exception:
            return None

for i in range(24):
    y = -4600.0 + i * (9200.0 / 23.0)
    a_xy = unreal.Vector(-8000, y, 0)
    b_xy = unreal.Vector(8000, -y, 0)
    a_hit = trace(unreal.Vector(a_xy.x, a_xy.y, 5000), unreal.Vector(a_xy.x, a_xy.y, -5000))
    b_hit = trace(unreal.Vector(b_xy.x, b_xy.y, 5000), unreal.Vector(b_xy.x, b_xy.y, -5000))
    a_loc = hit_location(a_hit) if a_hit else None
    b_loc = hit_location(b_hit) if b_hit else None
    rec = {"i": i, "a_ground": a_loc is not None, "b_ground": b_loc is not None}
    if a_loc is not None and b_loc is not None:
        ground_hits += 1
        rec["a_z"] = a_loc.z
        rec["b_z"] = b_loc.z
        for height, key in [(180.0, "los_180"), (500.0, "los_500"), (1200.0, "los_1200")]:
            s = unreal.Vector(a_loc.x, a_loc.y, a_loc.z + height)
            e = unreal.Vector(b_loc.x, b_loc.y, b_loc.z + height)
            blocked = bool(trace(s, e))
            rec[key] = not blocked
        los180 += 1 if rec["los_180"] else 0
        los500 += 1 if rec["los_500"] else 0
        los1200 += 1 if rec["los_1200"] else 0
    rows.append(rec)

result = {
    "schema": 1,
    "world": str(world.get_path_name()) if world else None,
    "sample_pairs": 24,
    "ground_pair_hits": ground_hits,
    "los_clear_180cm": los180,
    "los_clear_500cm": los500,
    "los_clear_1200cm": los1200,
    "rows": rows,
}
os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "w", encoding="utf-8") as f:
    json.dump(result, f, indent=2)
unreal.log("SOUL_DRAGON_COLLISION_SIGHTLINES " + json.dumps({
    "ground": ground_hits, "los180": los180, "los500": los500, "los1200": los1200}))
