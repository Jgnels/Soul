from pathlib import Path
import numpy as np,json
from PIL import Image,ImageDraw
r=Path('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview');o=r/'TerrainWork';b=np.load(o/'grass_coast_v2.npy');n=len(b);step=1500/(n-1)
a=np.asarray(Image.open(o/'mesa01-2041-rg.png')).astype(np.uint16);raw=a[:,:,0]*256+a[:,:,1]
assert int(raw.max())-int(raw.min())>100
g=np.asarray(Image.fromarray(raw.astype(np.float32)).crop((480,400,1220,1160)).resize((n,n),Image.Resampling.BILINEAR))
lo,hi=np.percentile(g,[1,99]);g=np.clip((g-lo)/(hi-lo),0,1)
yy,xx=np.mgrid[:n,:n].astype(np.float32);xx*=step;yy*=step
poly=[(-.08,.075),(.075,.09),(.16,.13),(.20,.20),(.25,.285),(.18,.325),(.08,.355),(-.08,.34)]
im=Image.new('L',(n,n));ImageDraw.Draw(im).polygon([(round(x*(n-1)),round(y*(n-1))) for x,y in poly],fill=255)
inside=(np.asarray(im)>0)&(b>0);edge=np.full_like(b,1e9)
for a,c in zip(poly,poly[1:]+poly[:1]):
    ax,ay=np.array(a)*1500;cx,cy=np.array(c)*1500;t=np.clip(((xx-ax)*(cx-ax)+(yy-ay)*(cy-ay))/((cx-ax)**2+(cy-ay)**2),0,1)
    edge=np.minimum(edge,np.hypot(xx-ax-t*(cx-ax),yy-ay-t*(cy-ay)))
def smooth(v):
    v=np.clip(v,0,1);return v*v*(3-2*v)
weight=smooth(edge/85)*inside*smooth(b/4)
gu=np.clip(xx/1500/.25,0,1)*(n-1);gv=np.clip((yy/1500-.075)/.28,0,1)*(n-1)
gx=np.minimum(gu.astype(int),n-2);gy=np.minimum(gv.astype(int),n-2);fx=gu-gx;fy=gv-gy
graft=(g[gy,gx]*(1-fx)+g[gy,gx+1]*fx)*(1-fy)+(g[gy+1,gx]*(1-fx)+g[gy+1,gx+1]*fx)*fy
target=9+graft*43;z=b+(target-b)*weight
# A broad descending approach joins the plateau interior to the southern valley.
route=[(.08,.21,31),(.10,.245,25),(.12,.27,20),(.15,.30,15.5)]
nearest=np.full_like(z,1e9);grade=np.zeros_like(z)
for a,c in zip(route,route[1:]):
 ax,ay=np.array(a[:2])*1500;cx,cy=np.array(c[:2])*1500
 t=np.clip(((xx-ax)*(cx-ax)+(yy-ay)*(cy-ay))/((cx-ax)**2+(cy-ay)**2),0,1)
 d=np.hypot(xx-ax-t*(cx-ax),yy-ay-t*(cy-ay));near=d<nearest
 grade=np.where(near,a[2]+t*(c[2]-a[2]),grade);nearest=np.minimum(nearest,d)
ramp=(1-smooth((nearest-12)/24))*weight
z=z+(grade-z)*ramp
assert np.array_equal(z[~inside],b[~inside]);assert np.array_equal(z>0,b>0)
np.save(o/'mesa_coast_v2.npy',z);Image.fromarray(np.round(z*200+32768).clip(0,65535).astype(np.uint16)).save(o/'mesa_coast_v2.png')
# R: biome coverage; G: low canyon lava potential; B: donor relief.
mask=np.stack([weight,smooth((.24-graft)/.18)*smooth((weight-.45)/.45),graft*weight],axis=-1)
Image.fromarray(np.round(mask*255).astype('uint8')).save(o/'mesa-biome.png')
report={'base':'grass_coast_v2','source':'/Game/LandscapePackTwo/Maps/Mesa_01','polygon_uv':poly,'donor_raw_range':[int(raw.min()),int(raw.max())],'water_mask_unchanged':True,'outside_mask_unchanged':True,'edited_area_m2':float(inside.sum()*step**2)}
(o/'mesa-transplant-v2.json').write_text(json.dumps(report,indent=2));print(report)
