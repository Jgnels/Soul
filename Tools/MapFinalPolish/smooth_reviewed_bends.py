"""Bounded tangent-continuous curve trials on reviewed Nature/pass road bends."""
import sys,json,math
from pathlib import Path
import numpy as np
R=Path.cwd();E=R/'Evidence/MapFinalPolish-20261007';sys.path[:0]=[str(R/'Tools/MapPolish'),str(R/'Evidence/MapPolish-20261007/Local/python-libs')]
from route_surface import dense,assess,SITES
from scipy.spatial import cKDTree
D=json.loads((E/'Local/routes-art-final.json').read_text());anchors=np.array([x['xy_m'] for x in D['anchors']]);replacements=[];rows=[]
def local(q):
 x,y=q;return (300<x<850 and 2350<y<3100) or (1950<x<2300 and 1050<y<1520)
def unit(v):return v/max(np.linalg.norm(v),1e-10)
for r in D['routes']:
 for s in r['segments']:
  if s['type']!='road':continue
  p=dense(s['points_m'],1)
  # Reuse already solved identical geography in either traversal direction.
  for old,new in replacements:
   tree=cKDTree(p);dd,ii=tree.query(old[[0,-1]])
   if max(dd)>.9:continue
   lo,hi=sorted(ii);seg=p[lo:hi+1]
   if len(seg)<2 or cKDTree(old).query(seg)[0].max()>.9:continue
   q=new if ii[0]<ii[1] else new[::-1];trial=np.r_[p[:lo],q,p[hi+1:]];m=assess(trial)
   if m['max_grade_deg']<=22.1 and not m['invalid_samples']:p=dense(trial,1)
  candidates=[]
  for i in range(25,len(p)-25,6):
   if not local(p[i]) or np.linalg.norm(anchors-p[i],axis=1).min()<35:continue
   if any(np.linalg.norm(p[i]-c['center_xy_m'])<c['span_m']/2+35 for c in SITES):continue
   aa=unit(p[i]-p[i-12]);bb=unit(p[i+12]-p[i]);angle=math.degrees(math.acos(np.clip(aa@bb,-1,1)))
   if angle>24:candidates.append((angle,p[i].copy()))
  accepted_centers=[]
  for angle,center in sorted(candidates,reverse=True,key=lambda x:x[0]):
   if any(np.linalg.norm(center-v)<45 for v in accepted_centers):continue
   i=int(np.argmin(np.linalg.norm(p-center,axis=1)));accepted=False
   for trim in [40,30,22,15]:
    lo=max(3,i-trim);hi=min(len(p)-4,i+trim);old=p[lo:hi+1];length=np.linalg.norm(np.diff(old,axis=0),axis=1).sum();aa=unit(p[lo+3]-p[lo-3]);bb=unit(p[hi+3]-p[hi-3])
    for strength in [.35,.24,.45]:
     c1=old[0]+aa*length*strength;c2=old[-1]-bb*length*strength;t=np.linspace(0,1,max(80,math.ceil(length*8)))[:,None];q=(1-t)**3*old[0]+3*(1-t)**2*t*c1+3*(1-t)*t*t*c2+t**3*old[-1]
     if cKDTree(old).query(q)[0].max()>20:continue
     m=assess(q,.1)
     if m['max_grade_deg']>22.03 or m['invalid_samples']:continue
     trial=np.r_[p[:lo],q,p[hi+1:]];allm=assess(trial)
     if allm['max_grade_deg']>22.1 or allm['invalid_samples']:continue
     replacements.append((old.copy(),q.copy()));p=dense(trial,1);accepted_centers.append(center);rows.append(dict(a=r['a'],b=r['b'],center_xy_m=center.tolist(),before_turn_deg=angle,trim_m=trim,after=m));accepted=True;break
    if accepted:break
   if len(accepted_centers)>=8:break
  s['points_m']=p.tolist()
checks=[]
rejected_routes=[]
baseline=json.loads((E/'Local/routes-art-final.json').read_text())
for ri,r in enumerate(D['routes']):
 ms=[assess(s['points_m']) for s in r['segments'] if s['type']=='road'];checks.append(dict(a=r['a'],b=r['b'],max_grade_deg=max(v['max_grade_deg'] for v in ms),invalid_samples=sum(v['invalid_samples'] for v in ms)))
for i,check in enumerate(checks):
 if check['max_grade_deg']>22.1 or check['invalid_samples']:
  rejected_routes.append(check);D['routes'][i]=baseline['routes'][i];r=D['routes'][i];ms=[assess(s['points_m']) for s in r['segments'] if s['type']=='road'];checks[i]=dict(a=r['a'],b=r['b'],max_grade_deg=max(v['max_grade_deg'] for v in ms),invalid_samples=sum(v['invalid_samples'] for v in ms));rows=[v for v in rows if [v['a'],v['b']]!=[r['a'],r['b']]]
assert all(v['max_grade_deg']<=22.1 and v['invalid_samples']==0 for v in checks)
(E/'Local/routes-art-r5.json').write_text(json.dumps(D));(E/'bounded-curves-r5.json').write_text(json.dumps(dict(changes=rows,routes=checks,rejected_routes=rejected_routes,terrain_changed=False),indent=2));print('TANGENT_CURVES',len(rows),flush=True)
