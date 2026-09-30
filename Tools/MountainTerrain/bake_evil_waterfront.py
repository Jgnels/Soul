from pathlib import Path
import numpy as np,json
from PIL import Image,ImageDraw
r=Path('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview');o=r/'TerrainWork';b=np.load(o/'mesa_coast_v2.npy');n=len(b);step=1500/(n-1)
yy,xx=np.mgrid[:n,:n].astype(float);xx*=step;yy*=step
poly=[(.115,.265),(.245,.265),(.28,.295),(.26,.326),(.19,.346),(.125,.335)]
im=Image.new('L',(n,n));ImageDraw.Draw(im).polygon([(round(x*(n-1)),round(y*(n-1))) for x,y in poly],fill=255)
mask=(np.asarray(im)>0)&(b>0);im=Image.fromarray(mask.astype('uint8')*255).copy();ImageDraw.floodfill(im,(round(.17*(n-1)),round(.30*(n-1))),128);inside=np.asarray(im)==128
edge=np.full_like(b,1e9)
for a,c in zip(poly,poly[1:]+poly[:1]):
 ax,ay=np.array(a)*1500;cx,cy=np.array(c)*1500;t=np.clip(((xx-ax)*(cx-ax)+(yy-ay)*(cy-ay))/((cx-ax)**2+(cy-ay)**2),0,1)
 edge=np.minimum(edge,np.hypot(xx-ax-t*(cx-ax),yy-ay-t*(cy-ay)))
def smooth(v):
 v=np.clip(v,0,1);return v*v*(3-2*v)
w=smooth(edge/25)*inside*smooth(b/.6)
# Grade down toward the northern bank without changing any wet/dry classification.
target=np.minimum(b,3+np.maximum(0,(.316-yy/1500))*220)
z=b+(target-b)*w
assert np.array_equal(z[~inside],b[~inside]);assert np.array_equal(z>0,b>0)
np.save(o/'evil_waterfront.npy',z);Image.fromarray(np.round(z*200+32768).astype('uint16')).save(o/'evil_waterfront.png')
(o/'evil-waterfront.json').write_text(json.dumps({'base':'mesa_coast_v2','polygon_uv':poly,'water_mask_unchanged':True,'outside_mask_unchanged':True,'harbor_search_uv':[.19,.31]},indent=2))
print('WATERFRONT_BAKED',int(inside.sum()))
