"""Bounded fine-grid route repair around post-channel failures. Analysis only.

No terrain edit. Bridge surface is approximated from the native placement
receipt; native deck geometry remains a separate acceptance gate.
"""
from pathlib import Path
import json,math,heapq
import numpy as np
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionWorldComposition-20261007';L=E/'Local'
z=np.load(L/'composed-height.npy');water=np.maximum(np.load(L/'river-water.npy'),np.load(L/'lake-water.npy'));water=np.maximum(water,0)
data=json.loads((E/'route-presentation-r5.json').read_text());sites=json.loads((E/'selected-crossing-sites-r3.json').read_text())['crossings'];decks={b['id']:b for b in json.loads((E/'bridge-placement-r2.json').read_text())['bridges']};report=[]
# Exact bridge center avoids the old coarse grid endpoint falling beside deck.
orc=next(c for c in sites if c['gate_id']=='orc_broken_bridge')
next(a for a in data['anchors'] if a['id']=='orc_broken_bridge')['xy_m']=orc['center_xy_m']
for r in data['routes']:
 if r['a']=='orc_broken_bridge':r['segments'][0]['points_m'][0]=orc['center_xy_m']
 if r['b']=='orc_broken_bridge':r['segments'][-1]['points_m'][-1]=orc['center_xy_m']
def source(p,field=z):
 p=np.clip(np.asarray(p)/3500*2040,0,2039.999);i=p.astype(int);f=p-i;x,y=i[...,0],i[...,1];a,b,c,d=field[y,x],field[y,x+1],field[y+1,x],field[y+1,x+1];fx,fy=f[...,0],f[...,1];return np.where(fx>=fy,a+(b-a)*fx+(d-b)*fy,a+(d-c)*fx+(c-a)*fy)
def surface(p):
 p=np.asarray(p);h=source(p);valid=h>source(p,water)+.04
 for c in sites:
  yaw=math.radians(c['yaw_deg']);axis=np.array([math.cos(yaw),math.sin(yaw)]);q=p-c['center_xy_m'];along=abs(q@axis);across=abs(q@np.array([-axis[1],axis[0]]))
  if c['type']=='ford':valid|=(along<c['span_m']/2+3)&(across<3)&(h>source(p,water)-.3);continue
  deck=(along<=c['span_m']/2)&(across<=2.64);d=decks[c['gate_id']];dh=d['deck_z_m']+(q@axis)*d['deck_gradient'];h=np.where(deck,np.maximum(h,dh),h);valid|=deck
 return h,valid
def dense(points,spacing=1):
 out=[]
 for a,b in zip(points[:-1],points[1:]):
  a,b=np.array(a)[:2],np.array(b)[:2];dist=np.linalg.norm(b-a)
  if dist<1e-6:continue
  out.extend(a+(b-a)*np.linspace(0,1,max(1,math.ceil(dist/spacing)),endpoint=False)[:,None])
 return np.array(out+[np.array(points[-1])[:2]])
def good(p):
 p=dense(p,.5);h,valid=surface(p);d=np.linalg.norm(np.diff(p,axis=0),axis=1);g=abs(np.diff(h))/np.maximum(d,.00001)
 return bool(valid.all() and g.max(initial=0)<=math.tan(math.radians(22)))
def find(start,end):
 margin=24.;origin=np.minimum(start,end)-margin;size=np.ceil(np.abs(end-start)+2*margin).astype(int)+1
 if size.max()>160:return None
 yy,xx=np.mgrid[:size[1],:size[0]];pts=np.stack([xx,yy],-1)+origin;h,valid=surface(pts);s=tuple(np.rint(start-origin).astype(int));e=tuple(np.rint(end-origin).astype(int));moves=[(1,0),(-1,0),(0,1),(0,-1),(1,1),(-1,1),(1,-1),(-1,-1)];cost=np.full((8,size[1],size[0]),np.inf)
 for k,(dx,dy) in enumerate(moves):
  endpts=pts+np.array([dx,dy]);prev=h;ok=valid.copy();maxgrade=np.zeros(h.shape)
  for t in [.25,.5,.75,1.]:
   hh,v=surface(pts+np.array([dx,dy])*t);gg=abs(hh-prev)/(.25*math.hypot(dx,dy));maxgrade=np.maximum(maxgrade,gg);ok&=v;prev=hh
  ok&=maxgrade<math.tan(math.radians(21.8));cost[k]=np.where(ok,math.hypot(dx,dy)*(1+3*maxgrade**2),np.inf)
 q=[(0,s)];dist={s:0};parents={};closed=set()
 while q:
  _,p=heapq.heappop(q)
  if p in closed:continue
  if p==e:
   path=[p]
   while p!=s:p=parents[p];path.append(p)
   path=np.array(list(reversed(path)))+origin;path[0]=start;path[-1]=end
   if not good(path):return None
   out=[path[0]];i=0
   while i<len(path)-1:
    j=min(i+8,len(path)-1)
    while j>i+1 and not good(path[i:j+1][[0,-1]]):j-=1
    out.append(path[j]);i=j
   return np.array(out)
  closed.add(p);x,y=p
  for k,(dx,dy) in enumerate(moves):
   n=(x+dx,y+dy)
   if not (0<=n[0]<size[0] and 0<=n[1]<size[1]) or n in closed:continue
   new=dist[p]+cost[k,y,x]
   if new>=dist.get(n,np.inf):continue
   dist[n]=new;parents[n]=p;heapq.heappush(q,(new+math.dist(n,e),n))
 return None
for r in data['routes']:
 for seg in r['segments']:
  if seg['type']!='road':continue
  p=dense(seg['points_m']);h,valid=surface(p);d=np.linalg.norm(np.diff(p,axis=0),axis=1);bad=(abs(np.diff(h))/np.maximum(d,.00001)>math.tan(math.radians(22)))|(~valid[:-1])|(~valid[1:]);ids=np.where(bad)[0]
  if not len(ids):continue
  chunks=np.split(ids,np.where(np.diff(ids)>25)[0]+1)
  for chunk in reversed(chunks):
   lo=max(0,int(chunk[0])-20);hi=min(len(p)-1,int(chunk[-1])+21);repair=find(p[lo],p[hi]);ok=repair is not None
   report.append(dict(a=r['a'],b=r['b'],start=p[lo].tolist(),end=p[hi].tolist(),accepted=ok))
   if ok:p=np.concatenate([p[:lo],repair,p[hi+1:]])
  seg['points_m']=p.tolist()
 print('LOCAL_REPAIR',r['a'],r['b'],flush=True)
(E/'route-local-repair-study-r7.json').write_text(json.dumps(data,indent=2));(E/'local-route-repair-receipt-r7.json').write_text(json.dumps({'repairs':report,'accepted':sum(r['accepted'] for r in report),'rejected':sum(not r['accepted'] for r in report),'status':'analysis only; never silently promoted'},indent=2));print('COMPLETE',len(report),sum(r['accepted'] for r in report),flush=True)
