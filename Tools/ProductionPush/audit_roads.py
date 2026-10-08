from pathlib import Path
import sys,json,numpy as np,time
R=Path.cwd();E=R/'Evidence/ProductionPush-20261008';sys.path[:0]=[str(R/'Tools/MapPolish'),str(R/'Evidence/MapPolish-20261007/Local/python-libs')]
from route_surface import dense,assess,SITES
from scipy.spatial import cKDTree
def uniform(points,step):
 p=np.asarray(points);arc=np.r_[0,np.cumsum(np.linalg.norm(np.diff(p,axis=0),axis=1))];ss=np.r_[np.arange(0,arc[-1],step),arc[-1]];return np.stack([np.interp(ss,arc,p[:,i]) for i in [0,1]],-1)
D=json.loads((R/'Evidence/DwarfPassGate-20261007/Local/routes-final.json').read_text());rs=[];rows=[]
for i,r in enumerate(D['routes']):
 for si,s in enumerate(r['segments']):
  if s['type']=='road':rs.append((i,si,uniform(s['points_m'],2)))
for i,si,p in rs:
 for j,sj,q in rs:
  if i>=j:continue
  d,ix=cKDTree(q).query(p);ids=np.flatnonzero(d<25);runs=np.split(ids,np.where(np.diff(ids)>1)[0]+1) if len(ids) else []
  for run in runs:
   if len(run)<40 or d[run].max()<4 or (d[run]>4).sum()<25:continue
   if any(np.linalg.norm(p[run]-c['center_xy_m'],axis=1).min()<c['span_m']/2+25 for c in SITES):continue
   lo,hi=run[[0,-1]];rows.append(dict(i=i,j=j,segment=si,reference_segment=sj,length_m=int(len(run)*2),mean_offset_m=float(d[run].mean()),max_offset_m=float(d[run].max()),start=p[lo].tolist(),end=p[hi].tolist(),route=[D['routes'][i]['a'],D['routes'][i]['b']],reference=[D['routes'][j]['a'],D['routes'][j]['b']]))
rows.sort(key=lambda r:r['length_m']*r['mean_offset_m'],reverse=True)
(E/'road-proximity-audit.json').write_text(json.dumps(rows,indent=2));(E/'Local/routes-baseline.json').write_text(json.dumps(D));print(json.dumps(rows[:9],indent=2))
