"""Compress native gate/hall spacing for an owned-mesh strategic miniature.

This is presentation abstraction only. The actual city map, navigation and
building state remain unchanged. No replacement architecture is generated.
"""
import datetime
import argparse
import json
from pathlib import Path

root = Path(__file__).resolve().parents[2]
evidence = root/'Evidence/SettlementEnvironmentPlan-20261005/Continuation-20261006'
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--revision', choices=('r8','r9'), default='r9')
args = p.parse_args()
dest = evidence/('dwarven-gate-miniature-source-'+args.revision+'.json')
assert not dest.exists()
original = json.loads((evidence/'dwarven-miniature-source.json').read_text())
gate = json.loads((evidence/'dwarven-gate-miniature-source-r7.json').read_text())
hall = json.loads((evidence/'dwarven-miniature-renderlod-base-r5.json').read_text())
bounds = hall['selection_bounds']
rows = [r for r in original['instances'] if r['state']=='upgrade' or
    all(bounds[2*i] <= r['location'][i] <= bounds[2*i+1] for i in range(3))]
gate_paths = set(gate['selected_gate_actors'])
gate_count = 0
omitted = []
for row in gate['instances']:
    if row['actor'] not in gate_paths: continue
    if args.revision == 'r9' and '/SM_ST_ExteriorGate_Blocker.' in row['mesh']:
        omitted.append(row['actor'])
        continue
    row = dict(row)
    x,y,z = row['location']
    # One uniform group transform: native front gate faces the native hall's
    # approach. Compress its monumental scale and empty canyon separation.
    row['location'] = [4500-y*.35, 2500+(x+5500)*.35, z*.35]
    pitch,yaw,roll = row['rotation']
    row['rotation'] = [pitch,yaw+90,roll]
    row['scale'] = [s*.35 for s in row['scale']]
    rows.append(row)
    gate_count += 1
assert gate_count>100
gate.update(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),instances=rows,
    representation='native hall cutaway and native gate, one deterministic group transform compresses campaign spacing; no cliff backsides',
    group_transform=dict(source_pivot=[-5500,0,0], target_pivot=[4500,2500,0], yaw=90, uniform_scale=.35),
    selected_rock_actors=[], omitted_backstage_blockers=omitted,
    counts={state:sum(r['state']==state for r in rows) for state in ('base','upgrade')})
dest.write_text(json.dumps(gate,indent=2)+'\n')
print('SOUL_COMPACT_GATE_SOURCE',gate['counts'],str(dest))
