"""Replace abrupt shared-trunk splices with grade-tested gradual joins."""
import sys,json
from pathlib import Path
import numpy as np
R=Path.cwd();E=R/'Evidence/MapFinalPolish-20261007';sys.path[:0]=[str(R/'Tools/MapPolish'),str(R/'Evidence/MapPolish-20261007/Local/python-libs')]
from route_surface import dense,assess,SITES
from scipy.spatial import cKDTree
D=json.loads((E/'Local/routes-art-r1.json').read_text());rows=[]
for i,j in [(2,5),(6,43),(7,43),(1,3),(16,17)]:
 for si,s in enumerate(D['routes'][i]['segments']):
  if s['type']!='road':continue
  p=dense(s['points_m'],1)
  for ref in D['routes'][j]['segments']:
   if ref['type']!='road':continue
   q=dense(ref['points_m'],1);d,ix=cKDTree(q).query(p)
   # Exact projection onto the two segments adjoining each nearest station.
   near=[];ds=[]
   for off in [-1,0]:
    a=q[np.clip(ix+off,0,len(q)-2)];b=q[np.clip(ix+off+1,1,len(q)-1)];v=b-a;t=np.clip(np.sum((p-a)*v,1)/np.maximum(np.sum(v*v,1),1e-12),0,1);z=a+t[:,None]*v;near.append(z);ds.append(np.linalg.norm(p-z,axis=1))
   choose=ds[0]<ds[1];proj=np.where(choose[:,None],near[0],near[1]);d=np.minimum(*ds)
   ids=np.flatnonzero((d<30)&(p[:,0]>200)&(p[:,0]<1350)&(p[:,1]>1150)&(p[:,1]<2450));runs=np.split(ids,np.where(np.diff(ids)>1)[0]+1) if len(ids) else []
   for run in runs:
    if len(run)<80 or d[run].max()<3:continue
    lo,hi=run[[0,-1]];accepted=False
    for margin in [40,65,90,120,25]:
     distances=np.arange(len(run));weight=np.minimum(np.minimum(distances/margin,(len(run)-1-distances)/margin),1);weight=np.clip(weight,0,1);weight=weight*weight*(3-2*weight)
     # Preserve endpoints exactly and protect the precise accepted ford reach.
     trial=p.copy();trial[run]=p[run]+weight[:,None]*(proj[run]-p[run]);trial[0]=p[0];trial[-1]=p[-1];m=assess(trial,.1)
     if m['max_grade_deg']<=22.1 and m['invalid_samples']==0:
      p=trial;accepted=True;rows.append(dict(route=[D['routes'][i]['a'],D['routes'][i]['b']],reference=[D['routes'][j]['a'],D['routes'][j]['b']],start=p[lo].tolist(),end=p[hi].tolist(),transition_m=margin,after=m));break
    if not accepted:rows.append(dict(rejected=[i,j],segment=si,reason='Gradual join failed frozen-ground grade gate'))
  s['points_m']=p.tolist()
checks=[]
for r in D['routes']:
 ms=[assess(s['points_m']) for s in r['segments'] if s['type']=='road'];checks.append(dict(a=r['a'],b=r['b'],max_grade_deg=max(v['max_grade_deg'] for v in ms),invalid_samples=sum(v['invalid_samples'] for v in ms)))
assert all(v['max_grade_deg']<=22.1 and v['invalid_samples']==0 for v in checks)
(E/'Local/routes-art-r3.json').write_text(json.dumps(D));(E/'shared-trunks-r3.json').write_text(json.dumps(dict(changes=rows,routes=checks,terrain_changed=False),indent=2));print(json.dumps(rows))
