"""Remove reviewed short backtracking artifacts only when frozen-ground grade passes."""
import sys,json,math
from pathlib import Path
import numpy as np
R=Path.cwd();E=R/'Evidence/MapFinalPolish-20261007';sys.path[:0]=[str(R/'Tools/MapPolish'),str(R/'Evidence/MapPolish-20261007/Local/python-libs')]
from route_surface import dense,assess,SITES
from scipy.spatial import cKDTree
D=json.loads((E/'Local/routes-art-r5.json').read_text());rows=[]
def area(v):
 x,y=v;return (200<x<1000 and 2250<y<3200) or (1950<x<2250 and 980<y<1550)
for r in D['routes']:
 for s in r['segments']:
  if s['type']!='road':continue
  p=dense(s['points_m'],1);original=p.copy();count=0
  for window in [28,20,12,8]:
   i=window
   while i<len(p)-window:
    if not area(p[i]) or any(np.linalg.norm(p[i]-c['center_xy_m'])<c['span_m']/2+20 for c in SITES):i+=8;continue
    lo=i-window;hi=i+window;old=p[lo:hi+1];chord=np.linalg.norm(old[-1]-old[0]);length=np.linalg.norm(np.diff(old,axis=0),axis=1).sum()
    if length/max(chord,.1)<1.08:i+=8;continue
    q=dense([old[0],old[-1]],.2)
    if cKDTree(old).query(q)[0].max()>8:i+=8;continue
    m=assess(q,.1)
    if m['max_grade_deg']<=22.0 and not m['invalid_samples']:
     trial=np.r_[p[:lo],q,p[hi+1:]];check=assess(trial)
     if check['max_grade_deg']<=22.1 and not check['invalid_samples']:
      rows.append(dict(a=r['a'],b=r['b'],from_xy=old[0].tolist(),to_xy=old[-1].tolist(),removed_length_m=float(length-chord),max_grade_deg=m['max_grade_deg']));p=dense(trial,1);count+=1;i+=window*2;continue
    i+=8
  m=assess(p)
  if m['max_grade_deg']<=22.1 and not m['invalid_samples']:s['points_m']=p.tolist()
  else:s['points_m']=original.tolist();rows=[v for v in rows if [v['a'],v['b']]!=[r['a'],r['b']]]
checks=[]
for r in D['routes']:
 ms=[assess(s['points_m']) for s in r['segments'] if s['type']=='road'];checks.append(dict(a=r['a'],b=r['b'],max_grade_deg=max(v['max_grade_deg'] for v in ms),invalid_samples=sum(v['invalid_samples'] for v in ms)))
baseline=json.loads((E/'Local/routes-art-r5.json').read_text());rejected=[]
for i,v in enumerate(checks):
 if v['max_grade_deg']>22.1 or v['invalid_samples']:
  rejected.append(v);D['routes'][i]=baseline['routes'][i];r=D['routes'][i];ms=[assess(s['points_m']) for s in r['segments'] if s['type']=='road'];checks[i]=dict(a=r['a'],b=r['b'],max_grade_deg=max(m['max_grade_deg'] for m in ms),invalid_samples=sum(m['invalid_samples'] for m in ms));rows=[x for x in rows if [x['a'],x['b']]!=[r['a'],r['b']]]
assert all(v['max_grade_deg']<=22.1 and not v['invalid_samples'] for v in checks)
(E/'Local/routes-art-r6.json').write_text(json.dumps(D));(E/'short-kinks-r6.json').write_text(json.dumps(dict(changes=rows,routes=checks,rejected_routes=rejected,terrain_changed=False),indent=2));print('LOCAL_KINKS_REMOVED',len(rows))
