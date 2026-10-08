import sys,json
from pathlib import Path
import numpy as np
R=Path.cwd();E=R/'Evidence/MapFinalPolish-20261007';sys.path[:0]=[str(R/'Tools/MapPolish'),str(R/'Evidence/MapPolish-20261007/Local/python-libs')]
from route_surface import dense,assess
from scipy.spatial import cKDTree
D=json.loads((E/'Local/routes-art-r1.json').read_text());base=json.loads(json.dumps(D));diag=json.loads((E/'Local/human-overlap-diagnostic.json').read_text());receipts=[]
for i,j in [(2,5),(7,43),(1,3)]:
 item=next(v for v in diag if v['i']==i and v['j']==j);s=D['routes'][i]['segments'][item['segment']];ref=D['routes'][j]['segments'][item['reference_segment']]
 p=dense(s['points_m'],1);q=dense(ref['points_m'],1);lo=int(np.argmin(np.linalg.norm(p-item['start'],axis=1)));hi=int(np.argmin(np.linalg.norm(p-item['end'],axis=1)));ix=cKDTree(q).query(p[[lo,hi]])[1];seg=q[min(ix):max(ix)+1];seg=seg if ix[0]<ix[1] else seg[::-1]
 trial=np.r_[p[:lo+1],seg,p[hi:]];m=assess(trial,.1)
 assert np.allclose(trial[0],p[0]) and np.allclose(trial[-1],p[-1])
 if m['max_grade_deg']<=22.1 and m['invalid_samples']==0:
  s['points_m']=trial.tolist();receipts.append(dict(route=[D['routes'][i]['a'],D['routes'][i]['b']],reference=[D['routes'][j]['a'],D['routes'][j]['b']],start=p[lo].tolist(),end=p[hi].tolist(),max_divergence_m=item['dmax'],after=m))
 else:receipts.append(dict(rejected=[i,j],result=m))
checks=[]
for r in D['routes']:
 ms=[assess(s['points_m']) for s in r['segments'] if s['type']=='road'];checks.append(dict(a=r['a'],b=r['b'],max_grade_deg=max(v['max_grade_deg'] for v in ms),invalid_samples=sum(v['invalid_samples'] for v in ms)))
assert all(v['max_grade_deg']<=22.1 and v['invalid_samples']==0 for v in checks)
(E/'Local/routes-art-r2.json').write_text(json.dumps(D));(E/'human-shared-trunks-r2.json').write_text(json.dumps(dict(changes=receipts,routes=checks,anchors_unchanged=D['anchors']==base['anchors'],terrain_changed=False),indent=2));print(json.dumps(receipts))
