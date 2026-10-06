"""Read-only native collision probes of the authored exterior deployment area.

Replicates existing cap-15 SpawnArmy/SpawnFormation positions and the existing
arena Visibility trace. It selects no gameplay outcome and writes no geometry.
"""
import datetime
import json
import math
from pathlib import Path
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_name() == 'L_DwarfHold_Authored'
root = Path(unreal.Paths.project_dir())
out = root / 'Evidence/SettlementEnvironmentPlan-20261005/Continuation-20261006/dwarven-deployment-probes.json'
assert not out.exists()

def sample(x, y):
    h = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, 8100),
        unreal.Vector(x, y, -11900), unreal.TraceTypeQuery.ECC_VISIBILITY,
        False, [], unreal.DrawDebugTrace.NONE)
    d = h.to_dict() if h else {}
    return dict(x=x, y=y, hit=d.get('blocking_hit', False),
        z=d['impact_point'].z if d else None,
        slope=math.degrees(math.acos(max(-1, min(1, d['impact_normal'].z)))) if d else None,
        actor=d['hit_actor'].get_name() if d and d['hit_actor'] else None)

candidates = []
for cx in (-14000, -13500, -13000, -12500, -12000):
    for cy in (-500, 0, 500):
        points = []
        for side in (0, 1):
            direction = 1 if side == 0 else -1
            for role, rearward, lateral, spacing in (('line', 0, 250, 135), ('ranged', 620, 250, 175), ('strike', 150, -650, 350)):
                for i in range(5):
                    row, col = divmod(i, 4)
                    members = min(4, 5 - row * 4)
                    x = cx - direction * 1500 - direction * rearward - direction * row * spacing
                    y = cy + direction * lateral + direction * (col - (members - 1) * .5) * spacing
                    points.append(dict(side=side, role=role, index=i, **sample(x, y)))
        hits = [p for p in points if p['hit']]
        candidates.append(dict(origin=[cx, cy, 0], misses=len(points)-len(hits),
            max_slope=max(p['slope'] for p in hits),
            mean_slope=sum(p['slope'] for p in hits)/len(hits),
            height_range=max(p['z'] for p in hits)-min(p['z'] for p in hits), points=points))
candidates.sort(key=lambda c: (c['misses'], c['max_slope'], c['height_range']))
out.write_text(json.dumps(dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
    map=world.get_path_name(), scope='cap 15 per side, native three-group deployment only; physical combat/navigation still needs runtime proof',
    candidates=candidates), indent=2)+'\n')
for row in candidates[:5]: print({k:v for k,v in row.items() if k != 'points'})
