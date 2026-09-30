from pathlib import Path
import numpy as np,json
from PIL import Image,ImageDraw
ROOT=Path('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview');out=ROOT/'TerrainWork'
b=np.load(out/'before.npy');n=b.shape[0];step=1500/(n-1)
a=np.asarray(Image.open(out/'grassland02-2041-rg.png')).astype(np.uint16);raw=a[:,:,0]*256+a[:,:,1]
print('donor_range',int(raw.min()),int(raw.max()));assert raw.max()-raw.min()>100
# Use an interior patch, retaining the donor's connected hill/valley forms.
g=Image.fromarray(raw.astype(np.float32)).crop((160,220,1860,1870)).resize((n,n),Image.Resampling.BILINEAR)
g=np.asarray(g);lo,hi=np.percentile(g,[1,99]);g=np.clip((g-lo)/(hi-lo),0,1)
yy,xx=np.mgrid[:n,:n].astype(np.float32);xx*=step;yy*=step
poly=[(.30,.43),(.49,.34),(.70,.35),(.74,.41),(.735,.48),(.78,.54),(.80,.65),(.82,.80),(.78,.85),(.66,.88),(.45,.87),(.32,.70)]
im=Image.new('L',(n,n));ImageDraw.Draw(im).polygon([(round(x*(n-1)),round(y*(n-1))) for x,y in poly],fill=255)
mask=(np.asarray(im)>0)&(b>0)
component=Image.fromarray(mask.astype('uint8')*255).copy();ImageDraw.floodfill(component,(int(.58*(n-1)),int(.65*(n-1))),128)
inside=(np.asarray(component)==128).copy()
print('mask',inside.sum(),float(b[int(.65*(n-1)),int(.58*(n-1))]))
# Coarse shoreline distance; cap at120m because only the coastal falloff needs it.
land=b[::4,::4]>0;dist=np.where(land,120.,0.).astype(np.float32)
for _ in range(48):
    pad=np.pad(dist,1,constant_values=120)
    dist=np.minimum(dist,np.minimum.reduce([pad[:-2,1:-1],pad[2:,1:-1],pad[1:-1,:-2],pad[1:-1,2:]])+step*4)
shore=np.asarray(Image.fromarray(dist).resize((n,n),Image.Resampling.BILINEAR))
edge=np.full_like(b,1e9)
for a,c in zip(poly,poly[1:]+poly[:1]):
    ax,ay=np.array(a)*1500;cx,cy=np.array(c)*1500;t=np.clip(((xx-ax)*(cx-ax)+(yy-ay)*(cy-ay))/((cx-ax)**2+(cy-ay)**2),0,1)
    edge=np.minimum(edge,np.hypot(xx-ax-t*(cx-ax),yy-ay-t*(cy-ay)))
def smooth(v):
    v=np.clip(v,0,1);return v*v*(3-2*v)
weight=smooth(edge/105)*inside*smooth(b/.8)
print('weight',float(weight.max()),float(edge.max()),float(b.max()))
gu=np.clip((xx/1500-.36)/.44,0,1)*(n-1);gv=np.clip((yy/1500-.36)/.51,0,1)*(n-1)
gx=np.minimum(gu.astype(int),n-2);gy=np.minimum(gv.astype(int),n-2);fx=gu-gx;fy=gv-gy
graft=(g[gy,gx]*(1-fx)+g[gy,gx+1]*fx)*(1-fy)+(g[gy+1,gx]*(1-fx)+g[gy+1,gx+1]*fx)*fy
target=(1.5+graft*23)*(1-np.exp(-shore/36))+.15
z=b+(target-b)*weight
assert np.array_equal(z[~inside],b[~inside]);assert np.array_equal(z[b<=0],b[b<=0]);assert np.array_equal(z>0,b>0)
np.save(out/'grass_coast_v2.npy',z);Image.fromarray(np.round(z*200+32768).clip(0,65535).astype(np.uint16)).save(out/'grass_coast_v2.png')
dy,dx=np.gradient(z,step);slope=np.degrees(np.arctan(np.hypot(dx,dy)));light=np.clip((.7-.45*dx-.3*dy)/np.sqrt(1+dx*dx+dy*dy),.12,1)
rgb=np.stack([light*.59,light*.68,light*.40],axis=-1);rgb[z<=0]=[.09,.29,.37];Image.fromarray((rgb*255).astype('uint8')).save(out/'grass_coast_v2-relief.png')
report={'source_map':'/Game/LandscapePackTwo/Maps/Grassland_02','base_map':'/Game/LandscapePackOne/Maps/Mountain_05','polygon_uv':poly,'donor_raw_range':[int(raw.min()),int(raw.max())],'core_slope_p95_degrees':float(np.percentile(slope[weight>.95],95)),'edited_area_m2':float(inside.sum()*step**2),'water_mask_unchanged':True,'outside_mask_unchanged':True,'shore_weighted_samples':int(((shore<25)&(weight>.9)).sum())}
(out/'grass-transplant-v2.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
