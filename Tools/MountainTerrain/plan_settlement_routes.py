from pathlib import Path
import numpy as np,json,heapq
r=Path(__file__).resolve().parents[1];p=r/'TerrainWork';data=json.loads((p/'settlement-plan.json').read_text());z=np.load(p/'evil_waterfront.npy');q=z[::4,::4];step=1500/2040*4
dy,dx=np.gradient(q,step);slope=np.degrees(np.arctan(np.hypot(dx,dy)));base_valid=(q>1.2)&(slope<22);ny,nx=q.shape
vy,vx=np.mgrid[:ny,:nx];vx=vx/(nx-1);vy=vy/(ny-1)
def point(s):return (round(s['uv'][1]*(ny-1)),round(s['uv'][0]*(nx-1)))
def project(y,x):
 u,v=x/(nx-1),y/(ny-1);scale=960/(np.tan(np.radians(25))*(324500-q[y,x]*100))
 return [round(960+(u-.5)*150000*scale,2),round(540+(v-.5)*150000*scale,2)]
routes=[]
for faction in dict.fromkeys(s['faction'] for s in data['sites']):
 zones={'Humans':(vx>.33)&(vx<.85)&(vy>.37)&(vy<.89),'Dwarves':((vx>.76)&(vy>.38))|((vy>.81)&(vx>.40)),'Nature':(vx>.25)&(vx<.64)&(vy<.38),'Orcs':(vx>.62)&(vy<.38),'Vikings':(vx<.34)&(vy>.36),'Evil':(vx<.29)&(vy<.355)}
 valid=base_valid&zones[faction]
 sites=[s for s in data['sites'] if s['faction']==faction];start=point(sites[0]);targets={point(s):s for s in sites[1:]};todo=[(0,start)];cost={start:0};prev={}
 while todo and any(t not in prev for t in targets):
  c,u=heapq.heappop(todo)
  if c!=cost[u]:continue
  for oy,ox in [(0,1),(0,-1),(1,0),(-1,0),(1,1),(-1,-1),(1,-1),(-1,1)]:
   v=(u[0]+oy,u[1]+ox)
   if not(0<=v[0]<ny and 0<=v[1]<nx) or not valid[v]:continue
   if ox and oy and (not valid[u[0],v[1]] or not valid[v[0],u[1]]):continue
   nc=c+np.hypot(ox,oy)*(1+slope[v]/8)
   if nc<cost.get(v,1e20):cost[v]=nc;prev[v]=u;heapq.heappush(todo,(nc,v))
 for end,site in targets.items():
  path=[]
  if end in prev:
   path=[end]
   while path[-1]!=start:path.append(prev[path[-1]])
   path=path[::-1]
  route={'faction':faction,'from':sites[0]['id'],'to':site['id'],'pass':bool(path),'points':[project(*v) for v in path[::3]+path[-1:]]}
  routes.append(route)
data['routes']=routes;(p/'settlement-plan.json').write_text(json.dumps(data,indent=2));print([{k:v for k,v in r.items() if k!='points'} for r in routes])
