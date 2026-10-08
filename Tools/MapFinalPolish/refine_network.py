"""Bounded shared-trunk and curve refinement; frozen terrain and canonical pairs."""
from pathlib import Path
import sys,json,math
import numpy as np
R=Path(__file__).resolve().parents[2];sys.path.insert(0,str(R/'Tools/MapPolish'))
from route_surface import dense,assess,SITES
E=R/'Evidence/MapFinalPolish-20261007';P=R/'Evidence/MapPolish-20261007'
sys.path.insert(0,str(P/'Local/python-libs'))
from scipy.spatial import cKDTree
from shapely.geometry import LineString
D=json.loads((P/'Local/routes-presentation-r4.json').read_text());baseline=json.loads(json.dumps(D));anchors=np.array([a['xy_m'] for a in D['anchors']]);receipts=[]
windows=[(400,1100,1300,2250),(1850,2380,1100,1600),(300,850,2300,3050),(2350,3100,1050,1900)]
def local(p):return any(np.all((p[:,0]>=x0)&(p[:,0]<=x1)&(p[:,1]>=y0)&(p[:,1]<=y1)) for x0,x1,y0,y1 in windows)
def crossing(p):return {c['gate_id'] for c in SITES if np.linalg.norm(p-c['center_xy_m'],axis=1).min()<c['span_m']/2+6}
masters=[]
for r in sorted(D['routes'],key=lambda r:sum(math.dist(a,b) for s in r['segments'] for a,b in zip(s['points_m'][:-1],s['points_m'][1:]))):
 for si,s in enumerate(r['segments']):
  if s['type']!='road':continue
  original=s['points_m'];p=dense(original,1.5)
  for ref,key,tree in masters:
   distances,indices=tree.query(p);ids=np.flatnonzero(distances<65)
   runs=np.split(ids,np.where(np.diff(ids)>1)[0]+1) if len(ids) else []
   for run in reversed(runs):
    near=run[distances[run]<2.5]
    if len(near)<2:continue
    lo,hi=int(near[0]),int(near[-1]);j,k=int(indices[lo]),int(indices[hi])
    if hi-lo<40 or abs(j-k)<35 or distances[lo:hi+1].max()<3:continue
    old=p[lo:hi+1]
    if not local(old):continue
    q=ref[min(j,k):max(j,k)+1]
    if j>k:q=q[::-1]
    if cKDTree(old).query(q)[0].max()>65 or crossing(q)!=crossing(old):continue
    d0=cKDTree(old).query(anchors)[0];d1=cKDTree(q).query(anchors)[0]
    if np.any((d0<25)&(d1>d0+2)):continue
    replacement=np.concatenate([p[lo:lo+1],q,p[hi:hi+1]]);test=assess(replacement,.1)
    if test['max_grade_deg']>22.05 or test['invalid_samples'] or test['length_m']>assess(old)['length_m']*1.05:continue
    trial=np.concatenate([p[:lo],replacement,p[hi+1:]]);whole=assess(trial)
    if whole['max_grade_deg']>22.1 or whole['invalid_samples']:continue
    receipts.append(dict(route=[r['a'],r['b']],reference=key,kind='local shared trunk',start=old[0].tolist(),end=old[-1].tolist(),maximum_divergence_m=float(distances[lo:hi+1].max()),before_m=assess(old)['length_m'],after=test))
    p=trial;break
  if assess(p)['max_grade_deg']<=22.1 and not assess(p)['invalid_samples']:s['points_m']=p.tolist()
  else:p=np.asarray(original)
  masters.append((p,[r['a'],r['b']],cKDTree(p)))
# Round only high-impact corners in reviewed windows, preserving crossing seats.
cache={}
for r in D['routes']:
 for s in r['segments']:
  if s['type']!='road':continue
  old=s['points_m'];p=np.asarray(LineString(old).simplify(.025).coords);out=[p[0]];count=0
  for a,b,c in zip(p[:-2],p[1:-1],p[2:]):
   ab=b-a;bc=c-b;la=np.linalg.norm(ab);lb=np.linalg.norm(bc)
   if min(la,lb)<1 or not local(np.array([b])):out.append(b);continue
   angle=math.degrees(math.acos(np.clip(ab@bc/la/lb,-1,1)))
   if not 18<angle<125 or any(np.linalg.norm(b-x['center_xy_m'])<x['span_m']/2+15 for x in SITES):out.append(b);continue
   key=tuple(np.round(np.r_[a,b,c],3));curve=cache.get(key)
   if curve is None:
    for trim in [min(22,la*.42,lb*.42),min(12,la*.38,lb*.38),min(6,la*.3,lb*.3)]:
     if trim<1:continue
     left=b-ab/la*trim;right=b+bc/lb*trim;t=np.linspace(0,1,max(16,math.ceil(trim*10)))[:,None]
     q=(1-t)**2*left+2*(1-t)*t*b+t*t*right;m=assess(q,.1)
     if m['max_grade_deg']<=22.02 and not m['invalid_samples']:curve=q;cache[key]=q;break
   if curve is None:out.append(b)
   else:out.extend(curve);count+=1
  out.append(p[-1]);m=assess(out)
  if m['max_grade_deg']<=22.1 and not m['invalid_samples']:
   s['points_m']=np.asarray(out).tolist()
   if count:receipts.append(dict(route=[r['a'],r['b']],kind='local curve rounding',corners=count,after=m))
assert {tuple(sorted([r['a'],r['b']])) for r in D['routes']}=={tuple(sorted([r['a'],r['b']])) for r in baseline['routes']}
assert D['anchors']==baseline['anchors']
checks=[dict(a=r['a'],b=r['b'],max_grade_deg=max(assess(s['points_m'])['max_grade_deg'] for s in r['segments'] if s['type']=='road'),invalid_samples=sum(assess(s['points_m'])['invalid_samples'] for s in r['segments'] if s['type']=='road')) for r in D['routes']]
assert all(x['max_grade_deg']<=22.1 and x['invalid_samples']==0 for x in checks)
(E/'Local/routes-art-r1.json').write_text(json.dumps(D))
(E/'road-art-r1.json').write_text(json.dumps(dict(frozen_anchors=True,frozen_topology=True,terrain_changed=False,review_windows_xy_m=windows,changes=receipts,routes=checks),indent=2))
print('ROAD_ART',len(receipts),'shared edits',sum(x['kind']=='local shared trunk' for x in receipts),'corners',sum(x.get('corners',0) for x in receipts),'51/51 grades')
