"""Bounded small-radius road fillets; no terrain changes or legal-edge removal."""
from pathlib import Path
import sys,json,math,time,numpy as np
R=Path.cwd();E=R/'Evidence/ProductionPush-20261008';sys.path[:0]=[str(R/'Tools/MapPolish'),str(R/'Evidence/MapPolish-20261007/Local/python-libs')]
from route_surface import assess,SITES
from shapely.geometry import LineString
from scipy.spatial import cKDTree
D=json.loads((E/'Local/routes-baseline.json').read_text());baseline=json.loads(json.dumps(D));anchors=np.array([a['xy_m'] for a in D['anchors']]);changes=[];rejected=0;started=time.monotonic()
# Preserve exact vertices outside the tiny changed interval, including fine grade-critical samples.
def arc(p):return np.r_[0,np.cumsum(np.linalg.norm(np.diff(p,axis=0),axis=1))]
def interp(p,t,v):return np.array([np.interp(v,t,p[:,k]) for k in [0,1]])
def allowed(c):
 x,y=c;return (1950<x<2700 and 200<y<1550) or (600<x<1150 and 1400<y<2100) or (300<x<850 and 2350<y<3100)
for ri,r in enumerate(D['routes']):
 for si,s in enumerate(r['segments']):
  if s['type']!='road':continue
  p=np.array(s['points_m']);simple=np.asarray(LineString(p).simplify(.10).coords);candidates=[]
  for k in range(1,len(simple)-1):
   c=simple[k];u=c-simple[k-1];v=simple[k+1]-c;nu=np.linalg.norm(u);nv=np.linalg.norm(v)
   if min(nu,nv)<3 or not allowed(c) or np.linalg.norm(anchors-c,axis=1).min()<35:continue
   if any(np.linalg.norm(c-q['center_xy_m'])<q['span_m']/2+50 for q in SITES):continue
   ang=math.degrees(math.acos(np.clip(u@v/nu/nv,-1,1)))
   if 40<ang<145:candidates.append((ang,c,min(nu,nv)))
  for ang,c,leg in sorted(candidates,key=lambda a:-a[0]):
   if len(changes)>=24 or time.monotonic()-started>540:break
   if any(np.linalg.norm(c-np.array(ch['center']))<15 for ch in changes):continue
   t=arc(p);i=int(np.argmin(np.linalg.norm(p-c,axis=1)))
   for trim in [10,7,5,3]:
    if trim>leg*.42 or t[i]<trim+2 or t[-1]-t[i]<trim+2:continue
    lo=t[i]-trim;hi=t[i]+trim;aa=interp(p,t,lo);bb=interp(p,t,hi);tt=np.linspace(0,1,101)[:,None];q=(1-tt)**2*aa+2*(1-tt)*tt*c+tt**2*bb;m=assess(q,.1)
    if m['max_grade_deg']>22.0 or m['invalid_samples']:continue
    trial=np.r_[p[t<lo],q,p[t>hi]];allm=assess(trial,.25)
    if allm['max_grade_deg']>22.1 or allm['invalid_samples']:continue
    # Keep shared road geography consistent on every overlapping legal use.
    patches=[];okay=True;old=p[(t>=lo)&(t<=hi)];old=np.r_[[aa],old,[bb]]
    for rj,other in enumerate(D['routes']):
     for sj,os in enumerate(other['segments']):
      if os['type']!='road' or (rj,sj)==(ri,si):continue
      op=np.asarray(os['points_m']);dist,idx=cKDTree(op).query([aa,bb]);a,b=sorted(idx)
      if max(dist)>.6 or b-a<2:continue
      if cKDTree(old).query(op[a:b+1])[0].max()>.6:continue
      qq=q if idx[0]<idx[1] else q[::-1];ot=np.r_[op[:a],qq,op[b+1:]];om=assess(ot,.25)
      if om['max_grade_deg']>22.1 or om['invalid_samples']:okay=False;break
      patches.append((rj,sj,ot))
     if not okay:break
    if not okay:continue
    p=trial;s['points_m']=p.tolist()
    for rj,sj,ot in patches:D['routes'][rj]['segments'][sj]['points_m']=ot.tolist()
    changes.append(dict(route=[r['a'],r['b']],center=c.tolist(),before_turn_deg=ang,trim_m=trim,local_check=m,shared_route_uses=len(patches)));print('FILLET',len(changes),r['a'],r['b'],trim,flush=True);break
   else:rejected+=1
checks=[]
for i,r in enumerate(D['routes']):
 ms=[assess(s['points_m']) for s in r['segments'] if s['type']=='road'];check=dict(a=r['a'],b=r['b'],max_grade_deg=max(m['max_grade_deg'] for m in ms),invalid_samples=sum(m['invalid_samples'] for m in ms));assert check['max_grade_deg']<=22.1 and check['invalid_samples']==0,check;checks.append(check)
assert D['anchors']==baseline['anchors']
(E/'Local/routes-final.json').write_text(json.dumps(D));(E/'road-fillet-r1.json').write_text(json.dumps(dict(changes=changes,rejected_candidates=rejected,routes=checks,terrain_changed=False,canonical_endpoints_unchanged=True,elapsed_s=time.monotonic()-started),indent=2));print('FILLET_COMPLETE',len(changes),flush=True)
