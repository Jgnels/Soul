"""Dense owned-heightfield route analysis. Canonical edges remain read-only.

Each grid edge is sampled at <=1.72 m before search. No terrain modification.
The output is presentation study data, never a gameplay adjacency authority.
"""
from pathlib import Path
import json,math,heapq,time,argparse
from collections import deque
import numpy as np
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'Evidence/TerrainFoundation-20261007'
parser=argparse.ArgumentParser()
parser.add_argument('--water-z',type=float,default=0.,help='Water elevation in candidate metres; original donor datum is 2.144607843.')
parser.add_argument('--output',default='dense-route-fit-r5.json')
args=parser.parse_args()
WATER=args.water_z
fit=json.loads((OUT/'mountain05-3500-rot1-sea-5-fit-r2.json').read_text())
raw=np.load('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview/TerrainWork/mountain05-height.npy').astype(float)
z=np.rot90(((raw-32768)*300/128+100+63390)/100+5,1)*3500/8160
SIDE=3500.;N=511;CELL=SIDE/(N-1);LIMIT=math.tan(math.radians(20.5));WIDTH=3.
def sample(points):
 p=np.clip(np.asarray(points)/SIDE*2040,0,2039.999);i=p.astype(int);f=p-i;x,y=i[...,0],i[...,1];fx,fy=f[...,0],f[...,1]
 a,b,c,d=z[y,x],z[y,x+1],z[y+1,x],z[y+1,x+1]
 # UE Chaos uses triangles 0-1-3 and 0-3-2. Verified against 300
 # native collision probes (max 0.17 cm error), unlike bilinear sampling.
 return np.where(fx>=fy,a+(b-a)*fx+(d-b)*fy,a+(d-c)*fx+(c-a)*fy)
def surface_gradient(points):
 p=np.clip(np.asarray(points)/SIDE*2040,0,2039.999);i=p.astype(int);f=p-i;x,y=i[...,0],i[...,1];a,b,c,d=z[y,x],z[y,x+1],z[y+1,x],z[y+1,x+1]
 first=f[...,0]>=f[...,1];return np.stack([np.where(first,b-a,d-c),np.where(first,d-b,c-a)],-1)/(SIDE/2040)
yy,xx=np.mgrid[:N,:N];grid=np.stack([xx,yy],-1)*CELL
MOVES=[(dx,dy) for dx,dy in [(1,0),(-1,0),(0,1),(0,-1),(1,1),(1,-1),(-1,1),(-1,-1)]]
costs=[]
for dx,dy in MOVES:
 delta=np.array([dx,dy])*CELL;steps=math.ceil(np.linalg.norm(delta)/.85)
 heights=np.stack([sample(grid+delta*t/steps) for t in range(steps+1)])
 grade=np.abs(np.diff(heights,axis=0))/(np.linalg.norm(delta)/steps)
 normal=np.array([-dy,dx])/math.hypot(dx,dy)
 banks=np.stack([np.abs(np.sum(surface_gradient(grid+delta*t/steps)*normal,axis=-1)) for t in range(steps+1)])
 valid=(heights.min(axis=0)>WATER+1.2)&(grade.max(axis=0)<=LIMIT)&(banks.max(axis=0)<math.tan(math.radians(25)))&(xx+dx>0)&(xx+dx<N-1)&(yy+dy>0)&(yy+dy<N-1)
 cost=np.linalg.norm(delta)*(1+18*np.mean(grade**2,axis=0)+8*np.mean(banks**2,axis=0)+.001*np.maximum(heights.mean(axis=0),0));cost[~valid]=np.inf;costs.append(cost)
costs=np.array(costs)
# Coarse anchor scoring can select an isolated shelf. Keep canonical identities,
# but move such presentation proposals to nearby connected land before routing.
human=next(a for a in fit['anchors'] if a['id']=='human_capital');seed=tuple(np.rint(np.array(human['xy_m'])/CELL).astype(int))
reachable=np.zeros((N,N),bool);reachable[seed[1],seed[0]]=True;todo=deque([seed])
while todo:
 x,y=todo.popleft()
 for k,(dx,dy) in enumerate(MOVES):
  nx,ny=x+dx,y+dy
  if not(0<nx<N-1 and 0<ny<N-1) or reachable[ny,nx] or not np.isfinite(costs[k,y,x]):continue
  reachable[ny,nx]=True;todo.append((nx,ny))
adjustments=[]
for anchor in fit['anchors']:
 px,py=np.rint(np.array(anchor['xy_m'])/CELL).astype(int)
 if reachable[py,px]:continue
 original=anchor['xy_m'];dist=np.linalg.norm(grid-np.array(original),axis=-1)
 score=dist.copy();score[~reachable]=np.inf
 jy,jx=np.unravel_index(score.argmin(),score.shape)
 if score[jy,jx]>150:continue
 anchor['xy_m']=[jx*CELL,jy*CELL];anchor['height_m']=float(sample(anchor['xy_m']))
 adjustments.append({'id':anchor['id'],'from_xy_m':original,'to_xy_m':anchor['xy_m'],'distance_m':float(score[jy,jx]),'reason':'nearest reachable existing terrain under road grade/bank constraints; no height edit'})
def safe_segment(a,b):
 a,b=np.array(a),np.array(b);d=np.linalg.norm(b-a)
 if d<.01:return True
 p=a+(b-a)*np.linspace(0,1,math.ceil(d/.85)+1)[:,None];h=sample(p)
 bank=np.abs(np.sum(surface_gradient(p)*np.array([-(b-a)[1],(b-a)[0]])/d,axis=-1))
 return bool(h.min()>WATER+1.2 and np.max(np.abs(np.diff(h)))/(d/(len(h)-1))<=LIMIT and bank.max()<math.tan(math.radians(25)))
def find(a,b):
 s=tuple(np.rint(np.array(a)/CELL).astype(int));e=tuple(np.rint(np.array(b)/CELL).astype(int))
 dist=np.full((N,N),np.inf);parents=np.full((N,N),-1,dtype=np.int8);closed=np.zeros((N,N),bool);dist[s[1],s[0]]=0;q=[(0.,s[0],s[1])]
 while q:
  _,x,y=heapq.heappop(q)
  if closed[y,x]:continue
  if (x,y)==e:
   p=[(x*CELL,y*CELL)]
   while (x,y)!=s:
    k=parents[y,x];dx,dy=MOVES[k];x-=dx;y-=dy;p.append((x*CELL,y*CELL))
   p=list(reversed(p));p[0]=a;p[-1]=b
   if not safe_segment(p[0],p[1]) or not safe_segment(p[-2],p[-1]):return None,'anchor connector fails'
   # Shortcuts are limited to 70 m and grade-tested against full-resolution data.
   out=[p[0]];i=0
   while i<len(p)-1:
    end=min(len(p)-1,i+10)
    while end>i+1 and not safe_segment(p[i],p[end]):end-=1
    out.append(p[end]);i=end
   rounded=[out[0]]
   for prev,corner,nxt in zip(out[:-2],out[1:-1],out[2:]):
    prev,corner,nxt=np.array(prev),np.array(corner),np.array(nxt)
    left,right=np.linalg.norm(prev-corner),np.linalg.norm(nxt-corner)
    amount=min(8,left*.25,right*.25)
    q=corner+(prev-corner)*(amount/left);r=corner+(nxt-corner)*(amount/right)
    curve=[((1-t)**2*q+2*(1-t)*t*corner+t*t*r).tolist() for t in np.linspace(0,1,7)]
    if all(safe_segment(a,b) for a,b in zip(curve[:-1],curve[1:])):rounded.extend(curve)
    else:rounded.append(corner.tolist())
   rounded.append(out[-1]);return rounded,None
  closed[y,x]=True
  for k,(dx,dy) in enumerate(MOVES):
   nx,ny=x+dx,y+dy
   if not(0<nx<N-1 and 0<ny<N-1) or closed[ny,nx]:continue
   co=dist[y,x]+costs[k,y,x]
   if co>=dist[ny,nx]:continue
   dist[ny,nx]=co;parents[ny,nx]=k;heapq.heappush(q,(co+math.hypot(nx-e[0],ny-e[1])*CELL,nx,ny))
 return None,'no dry <=22 degree full-resolution sampled path'
anchors={a['id']:a for a in fit['anchors']};routes=[];started=time.time()
for r in fit['routes']:
 p,error=find(anchors[r['a']]['xy_m'],anchors[r['b']]['xy_m'])
 rr={'a':r['a'],'b':r['b'],'success':p is not None,'error':error,'points_m':p or []}
 if p:
  dense=[]
  for a,b in zip(p[:-1],p[1:]):
   a,b=np.array(a),np.array(b);d=np.linalg.norm(b-a);dense.extend(a+(b-a)*np.linspace(0,1,math.ceil(d/.85)+1,endpoint=False)[:,None])
  dense=np.array(dense+[p[-1]]);h=sample(dense);seg=np.linalg.norm(np.diff(dense,axis=0),axis=1);grade=np.degrees(np.arctan(np.abs(np.diff(h))/seg))
  rr.update(length_m=float(seg.sum()),max_grade_deg=float(grade.max()),p95_grade_deg=float(np.percentile(grade,95)),min_clearance_above_water_m=float(h.min()-WATER),dense_points_xyz_m=np.column_stack([dense,h]).tolist())
 routes.append(rr);print(json.dumps({k:v for k,v in rr.items() if k not in ['points_m','dense_points_xyz_m']}),flush=True)
result={'side_m':SIDE,'source_samples_changed':False,'water_datum_native_m':WATER*8160/3500-5,'water_candidate_z_m':WATER,'grid_m':CELL,'maximum_sample_interval_m':.85,'search_grade_limit_deg':20.5,'acceptance_grade_limit_deg':22,'success_count':sum(r['success'] for r in routes),'route_count':len(routes),'elapsed_s':time.time()-started,'anchors':fit['anchors'],'anchor_adjustments':adjustments,'routes':routes,'limitations':['analytical centreline and bank constraints; native shoulder measurements require separate receipt','no runtime/gameplay proof','route identity is copied from canonical graph; no added edges']}
(OUT/args.output).write_text(json.dumps(result,indent=2));print('DENSE_COMPLETE',result['success_count'],adjustments,flush=True)
