"""Coherent woodland masks; deterministic placement of owned trees, no new art."""
from pathlib import Path
import json,math,collections
import numpy as np
from PIL import Image,ImageFilter
ROOT=Path(__file__).resolve().parents[2];E=ROOT/'Evidence/ProductionWorldComposition-20261007';L=E/'Local'
z=np.load(L/'composed-height.npy');lake=np.load(L/'lake-water.npy');river=np.load(L/'river-mask.npy');step=3500/2040
dy,dx=np.gradient(z,step);slope=np.degrees(np.arctan(np.hypot(dx,dy)))
road=Image.open(ROOT/'Data/CampaignCompositionLocal/Composition_Control_r3.png').getchannel('R').filter(ImageFilter.MaxFilter(15));road=np.array(road)
anchors=json.loads((E/'route-composition-proposal.json').read_text())['anchors'];rng=np.random.default_rng(7100703);points=[];counts=collections.Counter()
def ellipse(x,y,cx,cy,rx,ry):return ((x-cx)/rx)**2+((y-cy)/ry)**2
for y0 in np.arange(30,3470,8.5):
 for x0 in np.arange(30,3470,8.5):
  x,y=np.array([x0,y0])+rng.uniform(-3.5,3.5,2);ix,iy=np.rint([x/step,y/step]).astype(int)
  if z[iy,ix]<2.5 or slope[iy,ix]>33 or lake[iy,ix]>-900 or river[iy,ix]>.02 or road[iy,ix]>5:continue
  green=min(ellipse(x,y,430,2940,430,510),ellipse(x,y,925,3100,400,345),ellipse(x,y,250,2480,220,280))
  north=min(ellipse(x,y,640,460,520,300),ellipse(x,y,1430,490,450,230),ellipse(x,y,570,1120,180,250))
  human=min(ellipse(x,y,320,1520,180,230),ellipse(x,y,985,1790,160,240))
  boundary=1+.13*math.sin(x/66)+.11*math.sin(y/51)+.08*math.sin((x+y)/31)
  mass='greenwood' if green<boundary else 'boreal' if north<boundary and z[iy,ix]<90 else 'heartland_woodlot' if human<boundary else None
  if not mass:continue
  if min(ellipse(x,y,570,2960,125,110),ellipse(x,y,920,3260,130,85),ellipse(x,y,900,420,110,65))<1:continue
  if any(math.hypot(x-a['xy_m'][0],y-a['xy_m'][1])<(95 if a['kind']=='capital' else 30) for a in anchors):continue
  if rng.random()<.12:continue
  points.append(dict(xy_m=[float(x),float(y)],height_m=float(z[iy,ix]),tree_height_m=float(rng.uniform(12,20)),yaw=float(rng.uniform(0,360)),variant=int(rng.integers(0,2)),mass=mass));counts[mass]+=1
p=ROOT/'Data/CampaignCompositionLocal/forest-masses-r1.json';p.write_text(json.dumps(dict(seed=7100703,counts=dict(counts),points=points),separators=(',',':')))
print('FOREST_MASSES',len(points),dict(counts))
