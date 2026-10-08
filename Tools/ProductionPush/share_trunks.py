from pathlib import Path
import sys,json,numpy as np
R=Path.cwd();E=R/'Evidence/ProductionPush-20261008';sys.path[:0]=[str(R/'Tools/MapPolish'),str(R/'Evidence/MapPolish-20261007/Local/python-libs')]
from route_surface import assess
from scipy.spatial import cKDTree
D=json.loads((E/'Local/routes-final.json').read_text());diag=json.loads((E/'road-proximity-audit.json').read_text());rows=[]
for i,j in [(1,43),(2,43)]:
 item=next(v for v in diag if v['i']==i and v['j']==j);s=D['routes'][i]['segments'][item['segment']];ref=D['routes'][j]['segments'][item['reference_segment']];p=np.asarray(s['points_m']);q=np.asarray(ref['points_m']);lo=int(np.argmin(np.linalg.norm(p-item['start'],axis=1)));hi=int(np.argmin(np.linalg.norm(p-item['end'],axis=1)));lo,hi=sorted([lo,hi]);run=np.arange(lo,hi+1);t=np.r_[0,np.cumsum(np.linalg.norm(np.diff(p[run],axis=0),axis=1))];dd,ix=cKDTree(q).query(p[run]);near=[];ds=[]
 for off in [-1,0]:
  aa=q[np.clip(ix+off,0,len(q)-2)];bb=q[np.clip(ix+off+1,1,len(q)-1)];v=bb-aa;f=np.clip(np.sum((p[run]-aa)*v,1)/np.maximum(np.sum(v*v,1),1e-12),0,1);z=aa+f[:,None]*v;near.append(z);ds.append(np.linalg.norm(p[run]-z,axis=1))
 proj=np.where((ds[0]<ds[1])[:,None],near[0],near[1]);accepted=False
 for margin in [20,30,40,12]:
  w=np.clip(np.minimum(t/margin,(t[-1]-t)/margin),0,1);w=w*w*(3-2*w);trial=p.copy();trial[run]+=w[:,None]*(proj-p[run]);m=assess(trial,.1)
  if m['max_grade_deg']<=22.1 and not m['invalid_samples']:
   s['points_m']=trial.tolist();rows.append(dict(route=[D['routes'][i]['a'],D['routes'][i]['b']],reference=[D['routes'][j]['a'],D['routes'][j]['b']],start=p[lo].tolist(),end=p[hi].tolist(),transition_m=margin,accepted=True,check=m));accepted=True;break
 if not accepted:rows.append(dict(route=[D['routes'][i]['a'],D['routes'][i]['b']],accepted=False,reason='No bounded gradual join passed grade'))
(E/'Local/routes-final.json').write_text(json.dumps(D));(E/'human-trunks-r1.json').write_text(json.dumps(rows,indent=2));print(json.dumps(rows,indent=2))
