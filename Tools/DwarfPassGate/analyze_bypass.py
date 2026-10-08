"""Read-only slope-limited bypass test around a hypothetical local pass wall."""
import sys,json,heapq,math
from pathlib import Path
import numpy as np
R=Path.cwd();E=R/'Evidence/DwarfPassGate-20261007';sys.path.insert(0,str(R/'Tools/MapPolish'));from route_surface import sample,assess
c=np.array([2730.9150326797385,675.1416122004357]);cross=np.array([.5547001962252325,.8320502943378414]);along=np.array([cross[1],-cross[0]])
# At 2 m spacing, test existing ground around a 60 or 100 m transverse wall.
axis=np.arange(-140,141,2.);yy,xx=np.meshgrid(axis,axis,indexing='ij');pts=c+xx[...,None]*cross+yy[...,None]*along;z=sample(pts);N=len(axis);mid=N//2;start=(mid-25,mid);end=(mid+25,mid);rows=[]
grades={}
for dy,dx in [(i,j) for i in [-1,0,1] for j in [-1,0,1] if i or j]:
 length=2*math.hypot(dy,dx);delta=2*dx*cross+2*dy*along;n=16;prior=sample(pts);peak=np.zeros(z.shape)
 for t in np.linspace(0,1,n+1)[1:]:
  h=sample(pts+delta*t);peak=np.maximum(peak,np.degrees(np.arctan(abs(h-prior)/(length/n))));prior=h
 grades[(dy,dx)]=peak
for halfwidth in [30,50]:
 blocked=(abs(yy)<6)&(abs(xx)<=halfwidth);dist=np.full((N,N),np.inf);dist[start]=0;prev={};q=[(0,*start)];moves=[(i,j) for i in [-1,0,1] for j in [-1,0,1] if i or j]
 while q:
  cost,y,x=heapq.heappop(q)
  if cost!=dist[y,x]:continue
  if (y,x)==end:break
  for dy,dx in moves:
   ny,nx=y+dy,x+dx
   if not(0<=ny<N and 0<=nx<N) or blocked[ny,nx]:continue
   if dy and dx and (blocked[y,nx] or blocked[ny,x]):continue
   length=2*math.hypot(dy,dx);g=grades[(dy,dx)][y,x]
   if g>22.0:continue
   v=cost+length*(1+(g/22.1)**2)
   if v<dist[ny,nx]:dist[ny,nx]=v;prev[(ny,nx)]=(y,x);heapq.heappush(q,(v,ny,nx))
 if math.isfinite(dist[end]):
  path=[end]
  while path[-1]!=start:path.append(prev[path[-1]])
  path=path[::-1];p=np.array([pts[y,x] for y,x in path]);h=np.array([z[y,x] for y,x in path]);ds=np.linalg.norm(np.diff(p,axis=0),axis=1);gg=np.degrees(np.arctan(abs(np.diff(h))/ds));rows.append(dict(wall_total_width_m=halfwidth*2,bypass_found=True,length_m=float(ds.sum()),max_station_grade_deg=float(gg.max()),max_height_above_gate_m=float(h.max()-sample(c)),path_xy_m=p.tolist(),fine_check=assess(p,.25)))
 else:rows.append(dict(wall_total_width_m=halfwidth*2,bypass_found=False))
(E/'constriction-bypass-analysis.json').write_text(json.dumps(dict(analysis_only=True,center_xy_m=c.tolist(),start_xy_m=pts[start].tolist(),end_xy_m=pts[end].tolist(),sampling_m=2,search_station_grade_limit_deg=22.0,not_gameplay_topology=True,tests=rows),indent=2));print([{k:v for k,v in r.items() if k!='path_xy_m'} for r in rows])
