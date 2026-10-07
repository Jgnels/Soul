"""Bend fitted road presentation into measured bridge ends, without new edges."""
from pathlib import Path
import json,math,copy
import numpy as np
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionWorldComposition-20261007';z=np.load(E/'Local/composed-height.npy')
data=json.loads((E/'route-composition-proposal.json').read_text());sites=[c for c in json.loads((E/'selected-crossing-sites-r3.json').read_text())['crossings'] if c['type']=='bridge'];receipts=[]
def height(p):
 p=np.clip(np.asarray(p)/3500*2040,0,2039.999);i=p.astype(int);f=p-i;x,y=i[...,0],i[...,1];a,b,c,d=z[y,x],z[y,x+1],z[y+1,x],z[y+1,x+1];fx,fy=f[...,0],f[...,1];return np.where(fx>=fy,a+(b-a)*fx+(d-b)*fy,a+(d-c)*fx+(c-a)*fy)
def dense(points,step=1.):
 out=[]
 for a,b in zip(points[:-1],points[1:]):
  a,b=np.array(a)[:2],np.array(b)[:2];out.extend(a+(b-a)*np.linspace(0,1,max(1,math.ceil(np.linalg.norm(b-a)/step)),endpoint=False)[:,None])
 return np.array(out+[np.array(points[-1])[:2]])
def curve(p0,p1,p2,p3):
 t=np.linspace(0,1,max(6,math.ceil(np.linalg.norm(p3-p0)*2)))[:,None];return (1-t)**3*p0+3*(1-t)**2*t*p1+3*(1-t)*t*t*p2+t**3*p3
for r in data['routes']:
 for seg in r['segments']:
  if seg['type']!='road':continue
  pts=dense(seg['points_m'],2.)
  for c in sites:
   center=np.array(c['center_xy_m']);axis=np.diff(c['bank_endpoints_xy_m'],axis=0)[0];axis/=np.linalg.norm(axis);half=c['span_m']/2
   inside=np.linalg.norm(pts-center,axis=1)<half+32;indexes=np.where(inside)[0]
   if len(indexes)<2:continue
   # Work only on a contiguous crossing, never an unrelated nearby road.
   splits=np.split(indexes,np.where(np.diff(indexes)>1)[0]+1)
   for run in reversed(splits):
    lo,hi=max(0,int(run[0])-1),min(len(pts)-1,int(run[-1])+1)
    p0,p3=pts[lo],pts[hi];s0=np.dot(p0-center,axis);s3=np.dot(p3-center,axis)
    if s0*s3>=0 or min(abs(s0),abs(s3))<half:continue
    sign=1 if s0>0 else -1;entry=center+sign*axis*half;exit=center-sign*axis*half
    incoming=pts[min(lo+1,len(pts)-1)]-pts[max(lo-1,0)];incoming/=max(np.linalg.norm(incoming),.001)
    outgoing=pts[min(hi+1,len(pts)-1)]-pts[max(hi-1,0)];outgoing/=max(np.linalg.norm(outgoing),.001)
    left=curve(p0,p0+incoming*9,entry+sign*axis*8,entry);right=curve(exit,exit-sign*axis*8,p3-outgoing*9,p3)
    # Terrain outside the bridge stays unmodified; require traversable approach.
    grades=[]
    for side in [left,right]:
     h=height(side);dist=np.linalg.norm(np.diff(side,axis=0),axis=1);grades.extend(np.degrees(np.arctan2(abs(np.diff(h)),np.maximum(dist,.0001))).tolist())
    accepted=max(grades)<=22
    receipts.append(dict(a=r['a'],b=r['b'],gate=c['gate_id'],max_approach_grade_deg=max(grades),accepted=accepted))
    if accepted:pts=np.concatenate([pts[:lo],left,dense([entry,exit],1.)[1:-1],right,pts[hi+1:]])
  seg['points_m']=pts.tolist()
data['status']='presentation refinement; base graph and endpoints unchanged; bridge approach exceptions listed separately'
(E/'route-presentation-r5.json').write_text(json.dumps(data,indent=2));(E/'bridge-approach-fit-r5.json').write_text(json.dumps({'repairs':receipts,'accepted':sum(r['accepted'] for r in receipts),'rejected':sum(not r['accepted'] for r in receipts)},indent=2))
print('BRIDGE_APPROACHES',len(receipts),'ACCEPTED',sum(r['accepted'] for r in receipts),'REJECTED',[(r['gate'],round(r['max_approach_grade_deg'],2)) for r in receipts if not r['accepted']])
