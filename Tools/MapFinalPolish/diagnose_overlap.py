import sys,json,math
from pathlib import Path
import numpy as np
R=Path.cwd();E=R/'Evidence/MapFinalPolish-20261007';sys.path[:0]=[str(R/'Tools/MapPolish'),str(R/'Evidence/MapPolish-20261007/Local/python-libs')]
from route_surface import dense,assess,SITES
from scipy.spatial import cKDTree
D=json.loads((E/'Local/routes-art-r1.json').read_text());rs=[]
for i,r in enumerate(D['routes']):
 for si,s in enumerate(r['segments']):
  if s['type']=='road':rs.append((i,si,dense(s['points_m'],2)))
rows=[]
for i,si,p in rs:
 for j,sj,q in rs:
  if i>=j:continue
  d,ix=cKDTree(q).query(p);ids=np.flatnonzero((d<30)&(p[:,0]>200)&(p[:,0]<1350)&(p[:,1]>1150)&(p[:,1]<2450));runs=np.split(ids,np.where(np.diff(ids)>1)[0]+1) if len(ids) else []
  for run in runs:
   if len(run)<35 or d[run].max()<3:continue
   lo,hi=run[[0,-1]];a,b=ix[[lo,hi]];seg=q[min(a,b):max(a,b)+1];seg=seg if a<b else seg[::-1]
   trial=np.r_[p[:lo],seg,p[hi+1:]];m=assess(trial)
   rows.append(dict(i=i,j=j,segment=si,reference_segment=sj,n=len(run),dmax=d[run].max(),start=p[lo].tolist(),end=p[hi].tolist(),trial=m))
(E/'Local/human-overlap-diagnostic.json').write_text(json.dumps(rows,indent=2));print(json.dumps(rows,indent=2))
