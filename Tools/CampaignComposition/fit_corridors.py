"""Fit canonical edges to land corridors plus named, explicit ocean ferry passages."""
from pathlib import Path
import numpy as np,json,math,heapq,time
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'Evidence/ProductionWorldComposition-20261007';LOCAL=OUT/'Local';SIDE=3500.;N=511;CELL=SIDE/(N-1)
d=np.load(LOCAL/'routing-grid.npz');costs=d['costs'];labels=d['labels'];grid=d['grid'];z=d['original'];ocean=np.load(LOCAL/'ocean-connected-mask.npy');g=z[::4,::4];exclusion=np.load(LOCAL/'road-water-exclusion.npy')
MOVES=[(1,0),(-1,0),(0,1),(0,-1),(1,1),(1,-1),(-1,1),(-1,-1)]
data=json.loads((OUT/'region-placement-proposal.json').read_text());anchors={a['id']:a for a in data['anchors']};world=json.loads((ROOT/'Data/soul_world_overmap_v1_20260922.json').read_text());major={a['road_component'] for a in anchors.values()};print('COMPONENTS',major,flush=True)
def sample(p):
 p=np.clip(np.asarray(p)/SIDE*2040,0,2039.999);i=p.astype(int);f=p-i;x,y=i[...,0],i[...,1];a,b,c,d=z[y,x],z[y,x+1],z[y+1,x],z[y+1,x+1];fx,fy=f[...,0],f[...,1];return np.where(fx>=fy,a+(b-a)*fx+(d-b)*fy,a+(d-c)*fx+(c-a)*fy)
def safe(a,b):
 a,b=np.array(a),np.array(b);length=np.linalg.norm(b-a)
 if length<.01:return True
 p=a+(b-a)*np.linspace(0,1,math.ceil(length/.8)+1)[:,None];h=sample(p);normal=np.array([-(b-a)[1],(b-a)[0]])/length;bank=np.abs(sample(p+normal*1.5)-sample(p-normal*1.5))/3
 ii=np.clip(np.rint(p/3500*2040).astype(int),0,2040)
 return bool(not exclusion[ii[:,1],ii[:,0]].any() and h.min()>1.2 and np.max(np.abs(np.diff(h)))/(length/(len(h)-1))<=math.tan(math.radians(22)) and bank.max()<math.tan(math.radians(28)))
shore=np.zeros(ocean.shape,bool)
for dy in range(-7,8):
 for dx in range(-7,8):
  if dx*dx+dy*dy<=49:shore |=np.roll(np.roll(ocean,dy,0),dx,1)
def dock(target,component):
 valid=(labels==component)&shore&(np.isfinite(costs).sum(0)>=4)&(g>1.4)&(g<9);dist=np.linalg.norm(grid-target,axis=-1);dist[~valid]=np.inf;y,x=np.unravel_index(dist.argmin(),dist.shape);assert dist[y,x]<260,(target,dist[y,x]);return (int(x),int(y))
west=anchors['human_capital']['road_component'];east=anchors['dwarf_hold']['road_component'];port_defs=[('central_inlet_ferry',(1080,1320),(1510,1370),west,east),('southern_inlet_ferry',(2230,2110),(2190,2460),east,west)];jumps={};ports=[]
for name,p,q,c1,c2 in port_defs:
 a,b=dock(p,c1),dock(q,c2);pa=np.array(a)*CELL;pb=np.array(b)*CELL
 # This is an explicit navigable water corridor, never rendered as a road.
 points=pa+(pb-pa)*np.linspace(0,1,101)[:,None];h=sample(points);wet=h<=0
 print('PASSAGE',name,pa.tolist(),pb.tolist(),float(wet.mean()),flush=True);assert wet.mean()>.35,(name,'not a water passage',wet.mean())
 span=float(np.linalg.norm(pb-pa));jumps.setdefault(a,[]).append((b,span*1.6+120,name));jumps.setdefault(b,[]).append((a,span*1.6+120,name));ports.append({'id':name,'endpoints_xy_m':[pa.tolist(),pb.tolist()],'span_m':span,'water_fraction':float(wet.mean()),'type':'ferry','gameplay_authority':'presentation segment only; no added legal edge'})

def find(a,b):
 s=tuple(np.rint(np.array(a)/CELL).astype(int));e=tuple(np.rint(np.array(b)/CELL).astype(int));dist=np.full((N,N),np.inf);parents={};closed=np.zeros((N,N),bool);dist[s[1],s[0]]=0;q=[(0.,s)];visits=0
 while q:
  _,(x,y)=heapq.heappop(q)
  if closed[y,x]:continue
  if (x,y)==e:
   p=[((x,y),None)]
   while (x,y)!=s:
    prev,kind=parents[(x,y)];p[-1]=(p[-1][0],kind);x,y=prev;p.append(((x,y),None))
   return list(reversed(p)),visits
  closed[y,x]=True;visits+=1
  options=[((x+dx,y+dy),float(costs[k,y,x]),None) for k,(dx,dy) in enumerate(MOVES) if 0<x+dx<N-1 and 0<y+dy<N-1]
  options+=jumps.get((x,y),[])
  for (nx,ny),c,kind in options:
   if closed[ny,nx]:continue
   co=dist[y,x]+c
   if co>=dist[ny,nx]:continue
   dist[ny,nx]=co;parents[(nx,ny)]=((x,y),kind);heapq.heappush(q,(co+math.hypot(nx-e[0],ny-e[1])*CELL,(nx,ny)))
 return None,visits

def simplify(p):
 out=[p[0]];i=0
 while i<len(p)-1:
  end=min(len(p)-1,i+8)
  while end>i+1 and not safe(p[i],p[end]):end-=1
  out.append(p[end]);i=end
 # Keep bounded polyline corners; next road pass may round only revalidated arcs.
 return out
routes=[];started=time.time()
for e in world['edges']:
 a,b=anchors[e['a']]['xy_m'],anchors[e['b']]['xy_m'];path,visits=find(a,b);r={'a':e['a'],'b':e['b'],'success':path is not None,'segments':[],'visited_cells':visits}
 if path:
  land=[(np.array(path[0][0])*CELL).tolist()]
  for coord,kind in path[1:]:
   xy=(np.array(coord)*CELL).tolist()
   if kind:
    if len(land)>1:r['segments'].append({'type':'road','points_m':simplify(land)})
    r['segments'].append({'type':'ferry','id':kind,'points_m':[land[-1],xy]});land=[xy]
   else:land.append(xy)
  if len(land)>1:r['segments'].append({'type':'road','points_m':simplify(land)})
  dense=[];bad=[]
  for s in r['segments']:
   if s['type']!='road':continue
   for p,q in zip(s['points_m'][:-1],s['points_m'][1:]):
    p,q=np.array(p),np.array(q);length=np.linalg.norm(q-p);pp=p+(q-p)*np.linspace(0,1,math.ceil(length/.8)+1)[:,None];h=sample(pp);grade=np.degrees(np.arctan(np.abs(np.diff(h))/(length/(len(h)-1))));dense.extend(grade.tolist())
    if grade.max()>22.01:bad.append({'p':p.tolist(),'q':q.tolist(),'grade':float(grade.max())})
  r.update(max_road_grade_deg=max(dense),p95_road_grade_deg=float(np.percentile(dense,95)),unqualified_segments=bad,length_m=sum(math.dist(p,q) for s in r['segments'] for p,q in zip(s['points_m'][:-1],s['points_m'][1:])),ferry_count=sum(s['type']=='ferry' for s in r['segments']))
 routes.append(r);print(e['a'],e['b'],r['success'],r.get('max_road_grade_deg'),r.get('ferry_count'),flush=True)
result={'status':'analytical corridor proposal, not runtime accepted','canonical_nodes':36,'canonical_edges':51,'success_count':sum(r['success'] for r in routes),'side_m':SIDE,'grade_limit_deg':22,'bank_limit_deg':28,'anchors':list(anchors.values()),'ferry_passages':ports,'routes':routes,'elapsed_s':time.time()-started,'limitations':['road grade is pre-channel ground; river crossing deck/ford details need separate qualification','native Landscape collision not measured yet','ferries change no edge/action/save rules; presentation only']};(OUT/'route-composition-proposal.json').write_text(json.dumps(result,indent=2))
im=Image.open(LOCAL/'region-composition-proposal.png');draw=ImageDraw.Draw(im)
for r in routes:
 for s in r['segments']:draw.line([(x/3500*1020,y/3500*1020) for x,y in s['points_m']],fill='#e5c78c' if s['type']=='road' else '#cc809b',width=1 if s['type']=='road' else 2)
im.save(LOCAL/'route-composition-proposal.png');print('COMPLETE',result['success_count'],flush=True)
