"""Geographic composition proposals from licensed source samples, never gameplay.

Phase 1 writes numerical analyses/plots only. No Unreal Landscape import.
Source polygons, drainage and region/route fitting remain inspectable separately.
"""
from pathlib import Path
import numpy as np,json,heapq,math,collections
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'Evidence/ProductionWorldComposition-20261007';LOCAL=OUT/'Local';SIDE=3500.;SIZE=2041;STEP=SIDE/(SIZE-1)
SRC=Path('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview/TerrainWork')
def smooth(x):x=np.clip(x,0,1);return x*x*(3-2*x)
def sample(z,p,side=SIDE):
 limits=np.array([z.shape[1]-1,z.shape[0]-1]);p=np.clip(np.asarray(p)/side*limits,0,limits-1e-6);i=p.astype(int);f=p-i;x,y=i[...,0],i[...,1];return z[y,x]*(1-f[...,0])*(1-f[...,1])+z[y,x+1]*f[...,0]*(1-f[...,1])+z[y+1,x]*(1-f[...,0])*f[...,1]+z[y+1,x+1]*f[...,0]*f[...,1]
raw=np.load(SRC/'mountain05-height.npy').astype(float);base=np.rot90(((raw-32768)*300/128+63490)/100,1)*SIDE/8160
yy,xx=np.mgrid[:SIZE,:SIZE];X=xx*STEP;Y=yy*STEP
z=base.copy();manifest=[]
# FreshCan estuary source centred on its directly inspected river mouth.
# Source cross-section is uniformly scaled, embedded with an irregular oval
# transition that follows its valley/coast. This is a local western connection,
# not a lowered global water plane or rectangular biome insertion.
fresh=np.load(ROOT/'Evidence/TerrainFoundation-20261007/Local/FreshCanComparison/freshcan-collision-relative-water-m.npy')
fx=(X-230)/.62+(-751.05+2016);fy=(Y-730)/.62+(-434.82+2016)
rho=np.sqrt(((X-200)/520)**2+((Y-760)/560)**2)
fw=smooth((1-rho)/.32)
target=sample(fresh,np.stack([fx,fy],-1),4032)*.62
z=z+(target-z)*fw
manifest.append({'source':'FreshCan Map_CoastalLandscape native collision heights','role':'western Viking estuary / real ocean connection','source_center_m':[-751.05,-434.82],'target_center_m':[230,730],'xyz_scale':.62,'mask':'elliptical smooth perimeter, clipped only at world edge','area_changed_m2':float((fw>0).sum()*STEP**2),'water_datum_m':0})
# Actual Mesa_01 relief crop used by prior Soul terrain R&D, enlarged with
# proportionate relief; no invented plateau/terrace geometry.
img=np.array(Image.open(SRC/'mesa01-2041-rg.png')).astype(np.uint16);mesa=img[:,:,0]*256+img[:,:,1];crop=mesa[400:1160,480:1220].astype(float);lo,hi=np.percentile(crop,[1,99]);crop=np.clip((crop-lo)/(hi-lo),0,1)
mx=(X-2380)/1000*3500;my=(Y-1050)/1150*3500;donor=sample(crop,np.stack([mx,my],-1));rho=np.sqrt(((X-2880)/580)**2+((Y-1625)/650)**2);mw=smooth((1-rho)/.28)*smooth((base-2)/9)
target=24+donor*94;z=z+(target-z)*mw
manifest.append({'source':'LandscapePackTwo/Mesa_01; native RG16 export','role':'eastern badlands mesas/gullies','source_crop_pixels':[480,400,1220,1160],'target_core_m':[2380,1050,3380,2200],'elevation_mapping_m':[24,118],'mask':'elliptical perimeter and existing dry-shore attenuation','area_changed_m2':float((mw>0).sum()*STEP**2)})
np.save(LOCAL/'proposed-relief-before-rivers.npy',z);np.save(LOCAL/'mountain05-original-water.npy',base);np.save(LOCAL/'fresh-mask.npy',fw.astype(np.float32));np.save(LOCAL/'mesa-mask.npy',mw.astype(np.float32))
(OUT/'source-transform-proposal.json').write_text(json.dumps({'status':'ANALYTICAL ONLY, no Landscape imported','physical_side_m':SIDE,'global_water_m':0,'mountain_rotation_quarters':1,'mountain_xyz_scale':SIDE/8160,'patches':manifest,'unchanged_outside_patches':bool(np.array_equal(z[(fw==0)&(mw==0)],base[(fw==0)&(mw==0)])),'unchanged_fraction':float(((fw==0)&(mw==0)).mean()),'human_lowlands_broad_flattening':False},indent=2))

# Priority flood gives a drainage tree to existing sea/lakes. No filled terrain
# is exported. We inspect fill-depth and valley paths before selecting rivers.
g=z[::4,::4];N=len(g);cell=SIDE/(N-1);filled=g.copy();seen=np.zeros(g.shape,bool);parent=np.full((N,N,2),-1,np.int16);q=[];order=[];moves=[(-1,0),(1,0),(0,-1),(0,1),(-1,-1),(-1,1),(1,-1),(1,1)]
# Only boundary-connected ocean is an outlet. Enclosed negative basins
# need their real spill level and drainage route, not an imaginary sea outlet.
ocean=np.zeros(g.shape,bool);sea_queue=collections.deque()
for y,x in zip(*np.nonzero(g<=0)):
 if x in (0,N-1) or y in (0,N-1):ocean[y,x]=True;sea_queue.append((int(x),int(y)))
while sea_queue:
 x,y=sea_queue.popleft()
 for dx,dy in moves:
  nx,ny=x+dx,y+dy
  if 0<=nx<N and 0<=ny<N and g[ny,nx]<=0 and not ocean[ny,nx]:ocean[ny,nx]=True;sea_queue.append((nx,ny))
for y,x in zip(*np.nonzero(ocean)):seen[y,x]=True;filled[y,x]=0;heapq.heappush(q,(0.,int(x),int(y)))
while q:
 h,x,y=heapq.heappop(q);order.append((x,y))
 for dx,dy in moves:
  nx,ny=x+dx,y+dy
  if not(0<=nx<N and 0<=ny<N) or seen[ny,nx]:continue
  seen[ny,nx]=True;parent[ny,nx]=[x,y];filled[ny,nx]=max(h,g[ny,nx]);heapq.heappush(q,(filled[ny,nx],nx,ny))
acc=np.ones(g.shape,np.int32)
for x,y in reversed(order):
 px,py=parent[y,x]
 if px>=0:acc[py,px]+=acc[y,x]
np.savez(LOCAL/'drainage-tree.npz',terrain=g,filled=filled,parent=parent,acc=acc,cell=cell,ocean=ocean)
def path(x,y):
 p=[]
 while x>=0:
  p.append([x*cell,y*cell,float(g[y,x]),float(filled[y,x]-g[y,x]),int(acc[y,x]),float(filled[y,x])]);x,y=parent[y,x]
 return p
regions={'heartland':[250,1000,1000,2100],'greenwood':[200,2400,1200,3350],'eastern_run':[2450,1050,3300,2100]};candidates={}
for key,(x0,y0,x1,y1) in regions.items():
 options=[]
 for y in range(round(y0/cell),round(y1/cell),5):
  for x in range(round(x0/cell),round(x1/cell),5):
   if g[y,x]<15 or acc[y,x]<50:continue
   p=path(x,y);length=sum(math.dist(a[:2],b[:2]) for a,b in zip(p[:-1],p[1:]));max_fill=max(a[3] for a in p)
   if length<350 or max_fill>15:continue
   options.append({'start_m':p[0][:3],'mouth_m':p[-1][:3],'length_m':length,'maximum_spill_depth_m':max_fill,'points':p,'score':length-100*max_fill})
 options.sort(key=lambda x:x['score'],reverse=True);chosen=[]
 for p in options:
  if all(math.dist(p['start_m'][:2],a['start_m'][:2])>180 for a in chosen):chosen.append(p)
  if len(chosen)==6:break
 candidates[key]=chosen
(OUT/'drainage-candidates.json').write_text(json.dumps(candidates,indent=2))
dz=z[::2,::2];gy,gx=np.gradient(dz,SIDE/1020);shade=np.clip((.8-.45*gx-.3*gy)/np.sqrt(1+gx*gx+gy*gy),.16,1);h=np.clip(dz/160,0,1);rgb=np.stack([(.27+h*.4)*shade,(.43+h*.25)*shade,(.21+h*.5)*shade],-1);rgb[dz<=0]=[.06,.2,.29]
im=Image.fromarray(np.uint8(np.clip(rgb,0,1)*255));d=ImageDraw.Draw(im);font=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',16)
for v in range(0,3501,250):
 p=v/3500*1020;d.line((p,0,p,1020),fill='#555555');d.line((0,p,1020,p),fill='#555555')
 if v%500==0:d.text((p+2,2),str(v),font=font,fill='white');d.text((2,p+20),str(v),font=font,fill='white')
for key,rows in candidates.items():
 for i,p in enumerate(rows):
  d.line([(x/3500*1020,y/3500*1020) for x,y,*_ in p['points']],fill=['#99ddff','#56a9ed','#f2e182','#e69778','#cc99ff','#f7eeee'][i],width=2);x,y=p['start_m'][:2];d.text((x/3500*1020,y/3500*1020),key+str(i),font=font,fill='white')
im.save(LOCAL/'macro-drainage-proposal.png')
print(json.dumps({k:[{a:b for a,b in v.items() if a!='points'} for v in rows] for k,rows in candidates.items()},indent=2))
