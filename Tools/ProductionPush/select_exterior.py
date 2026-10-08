from pathlib import Path
import json,sys,numpy as np
sys.path.insert(0,'Tools/MapPolish');from route_surface import sample
D=json.loads(Path('Evidence/DwarfPassGate-20261007/Local/routes-final.json').read_text());a=next(x for x in D['anchors'] if x['id']=='dwarf_hold');print(a);c=np.array(a['xy_m']);print('RING',[(int(t),round(float(sample(c+20*np.array([np.cos(np.radians(t)),np.sin(np.radians(t))]))),2)) for t in range(0,360,45)])
for r in D['routes']:
 if 'dwarf_hold' in [r['a'],r['b']]:print('ARRIVAL',r['a'],r['b'],r['segments'][0]['points_m'][:3],r['segments'][-1]['points_m'][-3:])
P=Path('Evidence/SettlementEnvironmentPlan-20261005/Continuation-20261006');d=json.loads((P/'dwarven-gate-miniature-source-r7.json').read_text());gate=set(d['selected_gate_actors']);d['instances']=[r for r in d['instances'] if r['actor'] in gate and 'Blocker' not in r['mesh']];d['common_pivot']=[-5500,0,0];Path('Evidence/ProductionPush-20261008/dwarf-exterior-source.json').write_text(json.dumps(d,indent=2));print('EXTERIOR_INSTANCES',len(d['instances']))
