"""Derive a bounded campaign terrain edit from the licensed source; never writes donor packages."""
from pathlib import Path
import json,hashlib
import numpy as np
from PIL import Image,ImageDraw
ROOT=Path('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview'); OUT=ROOT/'TerrainWork'
raw=np.load(OUT/'mountain05-height.npy')
assert raw.shape==(2041,2041) and raw.max()>raw.min()+10000
N=raw.shape[0]; WIDTH=1500.; STEP=WIDTH/(N-1)
height=((raw.astype(np.float32)-32768)*300/128+100+63390)/100*(WIDTH/8160)
# Project Jeff's red sketch boundary through its ground-plane quadrilateral.
src=np.array([[184,44],[1008,44],[1170,745],[28,745]],float)
dst=np.array([[0,0],[1,0],[1,1],[0,1]],float)
A=[];B=[]
for (x,y),(u,v) in zip(src,dst):
    A.extend([[x,y,1,0,0,0,-u*x,-u*y],[0,0,0,x,y,1,-v*x,-v*y]]);B.extend([u,v])
H=np.r_[np.linalg.solve(A,B),1].reshape(3,3)
points=[[809,294],[687,283],[564,326],[491,391],[484,456],[483,499],[530,566],[565,606],[657,617],[789,579],[936,582],[881,462],[871,414],[809,363]]
poly=[]
for p in points:
    v=H@np.r_[p,1];poly.append((v[:2]/v[2]).tolist())
maskimage=Image.new('L',(N,N));ImageDraw.Draw(maskimage).polygon([(round(x*(N-1)),round(y*(N-1))) for x,y in poly],fill=255)
inside=np.asarray(maskimage)>0
yy,xx=np.mgrid[:N,:N].astype(np.float32);xx*=STEP;yy*=STEP
dist=np.full_like(height,1e9)
for a,b in zip(poly,poly[1:]+poly[:1]):
    ax,ay=np.array(a)*WIDTH;bx,by=np.array(b)*WIDTH
    t=np.clip(((xx-ax)*(bx-ax)+(yy-ay)*(by-ay))/((bx-ax)**2+(by-ay)**2),0,1)
    dist=np.minimum(dist,np.hypot(xx-(ax+t*(bx-ax)),yy-(ay+t*(by-ay))))
def smooth(x):
    t=np.clip(x,0,1);return t*t*(3-2*t)
weight=smooth(dist/120)*inside*smooth((height-1.5)/5)
freq=np.fft.fftfreq(N); kernel=np.exp(-2*np.pi**2*12**2*(freq[:,None]**2+freq[None,:]**2))
soft=np.fft.ifft2(np.fft.fft2(height)*kernel).real.astype(np.float32)
rolling=2+soft*.20+.6*np.sin(xx/113)*np.cos(yy/151)
target=np.minimum(height,rolling)
after=height+(target-height)*weight
# Preserve submerged ground and the entire unedited domain exactly at bake precision.
assert np.array_equal(after[~inside],height[~inside])
assert np.array_equal(after[height<=0],height[height<=0])
for name,z in [('before',height),('plains_v2',after)]:
    encoded=np.round(z*200+32768).clip(0,65535).astype(np.uint16)
    Image.fromarray(encoded).save(OUT/(name+'.png'))
    np.save(OUT/(name+'.npy'),z)
    dy,dx=np.gradient(z,STEP);slope=np.degrees(np.arctan(np.hypot(dx,dy)))
    light=np.clip((.7-.45*dx-.3*dy)/np.sqrt(1+dx*dx+dy*dy),.12,1)
    rgb=np.stack([light*.59,light*.68,light*.40],axis=-1)
    rgb[z<=0]=[.09,.29,.37]
    Image.fromarray((rgb*255).astype('uint8')).save(OUT/(name+'-relief.png'))
    if name=='plains_v2':
        stats={'plains_area_m2':int(inside.sum()*STEP*STEP),'core_area_m2':int((weight>.95).sum()*STEP*STEP),'core_slope_p95_degrees':float(np.percentile(slope[weight>.95],95)),'core_slope_max_degrees':float(slope[weight>.95].max()),'max_cut_m':float((height-after).max()),'water_unchanged':bool(np.array_equal(after[height<=0],height[height<=0]))}
meta={'width_m':WIDTH,'resolution':N,'xy_scale_cm':WIDTH*100/(N-1),'z_scale':64,'location_cm':[-WIDTH*50,-WIDTH*50,0],'human_polygon_uv':poly,'relief_multiplier':1,'height_decode_m':'(raw16 - 32768) * 0.005','source_map':'/Game/LandscapePackOne/Maps/Mountain_05','stats':stats}
(OUT/'terrain-plan-v2.json').write_text(json.dumps(meta,indent=2))
print(json.dumps(meta,indent=2))
