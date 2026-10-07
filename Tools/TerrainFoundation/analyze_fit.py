"""Read-only terrain-constrained graph and scale study, using native owned samples.
No heightfield export, Unreal import, canonical-data edits or saved terrain changes.
"""
from pathlib import Path
import json,math,heapq,time,argparse
import numpy as np
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'Evidence/TerrainFoundation-20261007';OUT.mkdir(exist_ok=True)
SRC=Path('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview/TerrainWork')
raw=np.load(SRC/'mountain05-height.npy').astype(np.float32);native=((raw-32768)*300/128+100+63390)/100
world=json.loads((ROOT/'Data/soul_world_overmap_v1_20260922.json').read_text());nodes=world['nodes'];edges=world['edges'];ni={n['id']:i for i,n in enumerate(nodes)}
xy=np.array([[n['x'],n['y']] for n in nodes],float);lo=np.array([45,45]);span=np.array([935,855])
font=lambda n:ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',n)
STRIDE=8;N=256;MOVES=[(dx,dy,math.hypot(dx,dy)) for dx,dy in [(1,0),(-1,0),(0,1),(0,-1),(1,1),(1,-1),(-1,1),(-1,-1)]]
def sample(z,x,y,side):
 u=np.clip(np.asarray(x)/side*2040,0,2039.999);v=np.clip(np.asarray(y)/side*2040,0,2039.999);ix=u.astype(int);iy=v.astype(int);fx=u-ix;fy=v-iy
 return z[iy,ix]*(1-fx)*(1-fy)+z[iy,ix+1]*fx*(1-fy)+z[iy+1,ix]*(1-fx)*fy+z[iy+1,ix+1]*fx*fy

def analyze(side,turn,water_native=0):
 t=time.time();z=(np.rot90(native,turn).copy()-water_native)*(side/8160);g=z[::STRIDE,::STRIDE];cell=side/255
 yy,xx=np.mgrid[:N,:N];X=xx*cell;Y=yy*cell;gy,gx=np.gradient(g,cell);slope=np.degrees(np.arctan(np.hypot(gx,gy)))
 target=(xy-lo)/935*side;target[:,1]+=(side-span[1]/935*side)/2
 placed=[];fits=[]
 for k,n in enumerate(nodes):
  tx,ty=target[k];cap=n['kind']=='capital';radius=40 if cap else 16
  ring=np.stack([sample(z,X+dx,Y+dy,side) for dx,dy in [(radius,0),(-radius,0),(0,radius),(0,-radius),(radius*.7,radius*.7),(-radius*.7,-radius*.7)]])
  relief=ring.max(axis=0)-ring.min(axis=0)
  hinter=np.stack([sample(z,X+dx,Y+dy,side) for dx,dy in [(150,0),(-150,0),(0,150),(0,-150)]])
  hr=hinter.max(axis=0)-hinter.min(axis=0)
  dist=np.hypot(X-tx,Y-ty);allowed=(g>2)&(dist<side*.13)&(X>70)&(Y>70)&(X<side-70)&(Y<side-70)&(ring.min(axis=0)>1)
  score=dist/side*120+slope*.35+relief*(1.8 if cap else .35)
  if n['macro_region']=='crownspine':score+=np.maximum(35-g,0)*.2
  if n['id']=='human_capital':score+=np.maximum(hr-35,0)*.25+np.maximum(5-hr,0)*2
  if n['id']=='viking_harbour':score+=g*.3
  if n['landform'] in ('pass','mountain_pass'):score+=np.maximum(20-g,0)*.4
  for pp in placed:
   dd=np.hypot(X-pp[0],Y-pp[1]);score+=np.maximum(120-dd,0)*3;allowed &= dd>75
  score[~allowed]=1e9
  j,i=np.unravel_index(score.argmin(),score.shape)
  if score[j,i]>=1e9:raise RuntimeError(n['id']+' no land candidate')
  placed.append((float(X[j,i]),float(Y[j,i])))
  fits.append({'id':n['id'],'xy_m':placed[-1],'height_m':float(g[j,i]),'displacement_m':float(dist[j,i]),'footprint_test_radius_m':radius,'radial_relief_m':float(relief[j,i]),'hinterland_cardinal_relief_150m':float(hr[j,i]),'point_slope_deg':float(slope[j,i])})
 # Water may not silently become traversable. First test only untouched dry ground.
 def route(a,b):
  s=(round(a[0]/cell),round(a[1]/cell));e=(round(b[0]/cell),round(b[1]/cell));cost={s:0.};q=[(0.,s)];parent={};closed=set()
  while q:
   _,p=heapq.heappop(q)
   if p in closed:continue
   if p==e:
    path=[p]
    while p!=s:p=parent[p];path.append(p)
    path=np.array([(x*cell,y*cell) for x,y in reversed(path)])
    length=np.linalg.norm(np.diff(path,axis=0),axis=1).sum();return path,float(length)
   closed.add(p);px,py=p;h=g[py,px]
   for dx,dy,dl in MOVES:
    qx,qy=px+dx,py+dy;dest=(qx,qy)
    if not(1<=qx<N-1 and 1<=qy<N-1) or dest in closed:continue
    hh=g[qy,qx]
    if hh<1.2:continue
    grade=abs(float(hh-h))/(cell*dl)
    if grade>math.tan(math.radians(22)):continue
    # Avoid passing diagonally through sea on corner cuts.
    if dx and dy and min(g[py,qx],g[qy,px])<1.2:continue
    co=cost[p]+cell*dl*(1+18*grade*grade+.001*max(float(h),0))
    if co>=cost.get(dest,math.inf):continue
    cost[dest]=co;parent[dest]=p;heapq.heappush(q,(co+math.hypot(qx-e[0],qy-e[1])*cell,dest))
  return None,None
 routes=[]
 for e in edges:
  a,b=e['a'],e['b'];p,length=route(placed[ni[a]],placed[ni[b]])
  routes.append({'a':a,'b':b,'dry_search_success':p is not None,'length_m':length,'straight_distance_m':math.dist(placed[ni[a]],placed[ni[b]]),'points_m':p.tolist() if p is not None else []})
 failed=[r['a']+'|'+r['b'] for r in routes if not r['dry_search_success']]
 result={'side_m':side,'source':'native Mountain05 complete domain, uniform XYZ compression for analytical study only','rotation_quarters':turn,'water_datum_native_m':water_native,'sample_grid_m':cell,'source_height_unchanged':True,'anchors':fits,'routes':routes,'dry_failures':failed,'dry_success_count':51-len(failed),'max_anchor_displacement_m':max(f['displacement_m'] for f in fits),'capital_radial_relief_max_m':max(f['radial_relief_m'] for f,n in zip(fits,nodes) if n['kind']=='capital'),'elapsed_s':time.time()-t,'limitations':['13.7-15.7 m dry search grid; no dense-grade or smoothing acceptance','Placement scoring is an analytical proposal, not canonical/gameplay authority','No water crossings or source terrain changes introduced to make the test pass']}
 fn=f'mountain05-{side}-rot{turn}-sea{water_native:g}-fit-r2';(OUT/(fn+'.json')).write_text(json.dumps(result,indent=2))
 # Plot actual unchanged source heights plus proposed anchor and route evidence.
 dz=z[::2,::2];dy,dx=np.gradient(dz,side/1020);light=np.clip((.8-.45*dx-.3*dy)/np.sqrt(1+dx*dx+dy*dy),.15,1);h=np.clip(dz/(side/8160*355),0,1)
 rgb=np.stack([(.3+h*.4)*light,(.46+h*.25)*light,(.22+h*.5)*light],-1);rgb[dz<=0]=[.08,.24,.33]
 im=Image.new('RGB',(1140,1180),'#101923');im.paste(Image.fromarray(np.uint8(np.clip(rgb,0,1)*255)),(55,95));d=ImageDraw.Draw(im)
 d.text((28,16),f'Mountain05 {side/1000:g} km / rotation {turn*90} / {51-len(failed)}/51 dry search paths',font=font(26),fill='white');d.text((28,55),'Evidence-only fitting; no new terrain, no crossing invented, no dense route acceptance.',font=font(18),fill='#bac8d8')
 pixel=lambda p:(55+p[0]/side*1020,95+p[1]/side*1020)
 for r in routes:
  if r['points_m']:d.line([pixel(p) for p in r['points_m']],fill='#d5b862',width=2)
  else:d.line([pixel(placed[ni[r['a']]]),pixel(placed[ni[r['b']]])],fill='#ef7770',width=3)
 for k,(n,f) in enumerate(zip(nodes,fits)):
  x,y=pixel(f['xy_m']);color='white' if n['kind']=='capital' else '#d6edf5';r=6 if n['kind']=='capital' else 3
  d.ellipse((x-r,y-r,x+r,y+r),fill=color);d.text((x+7,y-10),str(k+1),font=font(14),fill='white')
 d.text((35,1138),'White: capitals. Gold: coarse dry path. Red: no acceptable dry connection found.',font=font(19),fill='#bac8d8');im.save(OUT/(fn+'.png'))
 print(json.dumps({k:v for k,v in result.items() if k not in ['anchors','routes']}),flush=True)
 return result
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--side',type=int,action='append');p.add_argument('--rotation',type=int,action='append');p.add_argument('--water-native',type=float,default=0);a=p.parse_args()
 for side in a.side or [3500,4000]:
  for turn in a.rotation or [0,1,2,3]:analyze(side,turn,a.water_native)
