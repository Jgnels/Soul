"""Deterministic presentation lanes around measured owned Human miniature bounds.
No campaign edges, movement points, terrain or donor packages are changed.
"""
from pathlib import Path
import array,hashlib,heapq,json,math
root=Path.cwd();folder=root/'Evidence/SettlementEnvironmentPlan-20261005/Population-20261006'
profile=json.loads((root/'Data/CampaignEvilCorridor/presentation.json').read_text());o=profile['regions']['human_capital']
raw=(root/'Data/CampaignMesaLocal/MesaHeight.r16').read_bytes();assert hashlib.sha256(raw).hexdigest()==profile['height_sha256'];h=array.array('H');h.frombytes(raw);n=profile['resolution']
def height(x,y):
 u=max(0,min(n-1.001,(x+o[0]-profile['minimum_xy_cm'])/profile['extent_cm']*(n-1)));v=max(0,min(n-1.001,(y+o[1]-profile['minimum_xy_cm'])/profile['extent_cm']*(n-1)));a,b=int(u),int(v);u-=a;v-=b
 z=lambda i,j:(h[j*n+i]-32768)*profile['height_unit_cm']
 return z(a,b)+(z(a+1,b)-z(a,b))*u+(z(a+1,b+1)-z(a+1,b))*v if u>=v else z(a,b)+(z(a+1,b+1)-z(a,b+1))*u+(z(a,b+1)-z(a,b))*v
native=json.loads((folder/'HumanMiniature/human-native-footprints-r6.json').read_text());bounds=[]
for q in native:
 if 'SM_LI_Front_Gate_' in q['mesh']:continue
 bounds.append(dict(actor=q['actor'],minimum=[q['minimum'][0]*2-500,q['minimum'][1]*2],maximum=[q['maximum'][0]*2-500,q['maximum'][1]*2]))
def clear(p):return -12500<=p[0]<=1000 and -1800<=p[1]<=6100 and all(not(q['minimum'][0]-95<p[0]<q['maximum'][0]+95 and q['minimum'][1]-95<p[1]<q['maximum'][1]+95) for q in bounds)
def segment(a,b):
 count=max(1,math.ceil(math.dist(a,b)/45));prev=None;grade=0
 for i in range(count+1):
  p=[a[j]+(b[j]-a[j])*i/count for j in (0,1)]
  if not clear(p):return False,90
  z=height(*p)
  if prev:grade=max(grade,math.degrees(math.atan(abs(z-prev)/max(math.dist(a,b)/count,.001))))
  prev=z
 return grade<=22,grade
def route(a,b):
 assert clear(a) and clear(b),(a,b)
 frontier=[(0,tuple(a))];cost={tuple(a):0};came={}
 while frontier:
  _,p=heapq.heappop(frontier)
  if p==tuple(b):break
  for dx,dy in [(100,0),(-100,0),(0,100),(0,-100),(100,100),(100,-100),(-100,100),(-100,-100)]:
   q=(p[0]+dx,p[1]+dy)
   if not clear(q):continue
   ok,g=segment(p,q)
   if not ok:continue
   new=cost[p]+math.hypot(dx,dy)*(1+g*.012)
   if new<cost.get(q,1e30):cost[q]=new;came[q]=p;heapq.heappush(frontier,(new+math.dist(q,b),q))
 assert tuple(b) in came,(a,b)
 points=[tuple(b)]
 while points[-1]!=tuple(a):points.append(came[points[-1]])
 points.reverse();simple=[points[0]];i=0
 while i<len(points)-1:
  end=next(j for j in range(len(points)-1,i,-1) if segment(points[i],points[j])[0]);simple.append(points[end]);i=end
 return simple
# Approaches stop at native footprint boundaries; no path crosses a building.
connections=[([-5900,-600],[-7400,400]),([-7400,400],[-7600,2500]),([-7600,2500],[-7200,4500]),([-7200,4500],[-5200,5100]),([-5200,5100],[-2700,4500]),([-2700,4500],[-2800,2300]),([-2800,2300],[-3700,900]),([-3700,900],[-3500,-200]),([-7600,2500],[-6100,2200]),([-6100,2200],[-5500,2500]),([-5500,2500],[-5200,3800]),([-7400,400],[-10000,1200])]
paths=[]
for a,b in connections:
 pts=route(a,b);paths.append(dict(points=pts,half_width_cm=75,max_grade_degrees=max(segment(a,b)[1] for a,b in zip(pts,pts[1:]))))
out=folder/'human-native-streets-r7.json';assert not out.exists();out.write_text(json.dumps(dict(height_sha256=profile['height_sha256'],miniature='R7',origin=o,scope='presentation-only native-material lanes; 95cm clearance from measured building AABBs; unchanged legal topology and terrain',bounds=bounds,paths=paths),indent=2)+'\n')
print('lanes',len(paths),'points',sum(len(q['points']) for q in paths),'maximum_grade',max(q['max_grade_degrees'] for q in paths))
