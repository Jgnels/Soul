"""Deterministic, project-owned heightfield bake; no external art generator.

Run with CPython + numpy. Never reads/writes donor packages. Coordinates in the
design below are presentation units, multiplied by 20 into UE centimeters.
"""
from pathlib import Path
import hashlib
import json
import struct
import zlib
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'SourceArt/SoulCampaignTerrain'
DATA = ROOT / 'Data/CampaignTerrainV2'
N, EXTENT, SCALE = 1009, 14000., 20.
PLACES = dict(human_capital=(-3000, 0), crossroads=(-1600, 0),
              old_quarry=(-700, -1500), river_ford=(-200, 1200),
              forest_edge=(200, -500), ancient_shrine=(700, -2100),
              orc_watch=(1600, 800), north_pass=(1700, -1300), orc_camp=(3100, 0))
FIELDS = [(-3400,-680,240,180,.2),(-2860,-730,230,190,-.18),(-2350,-680,190,200,.25),(-3470,620,210,180,-.12),(-2940,750,230,195,.15),(-2450,660,190,160,.25),(-3380,-1110,240,155,.15),(-2830,-1190,230,175,-.12),(-2330,-1130,180,180,.2),(-3400,1110,210,190,-.12),(-2890,1210,230,175,.15),(-2390,1080,190,190,.2)]
# Curated approach controls. These do not add or remove strategic edges.
CONTROLS = [
 ('human_capital','crossroads',[(-2700,170),(-2300,230),(-1930,80)]),
 ('crossroads','old_quarry',[(-1480,-400),(-1180,-820),(-950,-1120)]),
 ('crossroads','river_ford',[(-1270,250),(-970,780),(-550,1150)]),
 ('crossroads','forest_edge',[(-1250,-220),(-810,-450),(-440,-540)]),
 ('old_quarry','ancient_shrine',[(-220,-1540),(240,-1790),(450,-2130)]),
 ('river_ford','orc_watch',[(220,1240),(670,1110),(1140,850)]),
 ('forest_edge','orc_watch',[(510,-240),(840,170),(1100,540)]),
 ('forest_edge','north_pass',[(680,-680),(1130,-1040),(1490,-1340)]),
 ('north_pass','orc_camp',[(1900,-1030),(2280,-660),(2670,-210)]),
 ('orc_watch','orc_camp',[(2040,650),(2440,390),(2820,160)])]

def smooth(a, b, x):
 t=np.clip((x-a)/(b-a),0,1); return t*t*(3-2*t)

def noise(x,y,period,seed):
 rng=np.random.default_rng(seed)
 grid=rng.random((128,128)).astype(np.float32)*2-1
 u=x/period+60; v=y/period+60
 ix=np.floor(u).astype(int); iy=np.floor(v).astype(int)
 fx=smooth(0,1,u-ix); fy=smooth(0,1,v-iy)
 return (grid[iy%128,ix%128]*(1-fx)+grid[iy%128,(ix+1)%128]*fx)*(1-fy)+(grid[(iy+1)%128,ix%128]*(1-fx)+grid[(iy+1)%128,(ix+1)%128]*fx)*fy

def curve(points, steps=16):
 p=np.array([points[0]]+points+[points[-1]],dtype=float); out=[]
 for i in range(1,len(p)-2):
  a,b,c,d=p[i-1:i+3]
  for t in np.linspace(0,1,steps,endpoint=False):
   out.append(.5*((2*b)+(-a+c)*t+(2*a-5*b+4*c-d)*t*t+(-a+3*b-3*c+d)*t**3))
 return np.array(out+[p[-1]])

def distance(x,y,points):
 best=np.full_like(x,1e9); along=np.zeros_like(x)
 for a,b in zip(points[:-1],points[1:]):
  dx,dy=b-a; t=np.clip(((x-a[0])*dx+(y-a[1])*dy)/(dx*dx+dy*dy),0,1)
  dd=np.hypot(x-a[0]-t*dx,y-a[1]-t*dy)
  along=np.where(dd<best,a[1]+t*dy,along); best=np.minimum(best,dd)
 return best,along

def sample(h,x,y):
 u=np.clip((np.asarray(x)+EXTENT)/(2*EXTENT)*(N-1),0,N-1.001)
 v=np.clip((np.asarray(y)+EXTENT)/(2*EXTENT)*(N-1),0,N-1.001)
 ix=u.astype(int); iy=v.astype(int); fx=u-ix; fy=v-iy
 return (h[iy,ix]*(1-fx)+h[iy,ix+1]*fx)*(1-fy)+(h[iy+1,ix]*(1-fx)+h[iy+1,ix+1]*fx)*fy

def png16(path, a):
 def chunk(k,b): return struct.pack('!I',len(b))+k+b+struct.pack('!I',zlib.crc32(k+b))
 raw=b''.join(b'\0'+row.astype('>u2').tobytes() for row in a)
 path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('!2I5B',a.shape[1],a.shape[0],16,0,0,0,0))+chunk(b'IDAT',zlib.compress(raw,9))+chunk(b'IEND',b''))

def tga(path,a):
 rgb=np.uint8(np.clip(a,0,1)*255)
 header=struct.pack('<BBBHHBHHHHBB',0,0,2,0,0,0,0,0,a.shape[1],a.shape[0],24,32)
 path.write_bytes(header+rgb[:,:,::-1].tobytes())

def main():
 OUT.mkdir(parents=True,exist_ok=True); DATA.mkdir(parents=True,exist_ok=True)
 line=np.linspace(-EXTENT,EXTENT,N,dtype=np.float32); x,y=np.meshgrid(line,line)
 broad=noise(x,y,1600,73); medium=noise(x,y,440,21); fine=noise(x,y,100,91)
 h=110+70*broad+28*medium+5*fine
 # Broken enclosing ridgelines rather than a periodic sine mountain wall.
 for points,width,height in [([(-7300,-4900),(-2600,-3900),(0,-3200),(2300,-3700),(6300,-2200)],630,1100),
  ([(-6500,3500),(-2700,2600),(1000,3700),(5700,2100)],950,950),
  ([(3700,-4700),(4100,-1800),(4650,600),(3900,3600)],720,1200),
  ([(850,-2400),(1130,-1750),(1150,-1450)],250,510),
  ([(2350,-2200),(2220,-1500),(2300,-1180)],290,680)]:
  d,_=distance(x,y,np.array(points)); h+=height*np.exp(-(d/width)**1.7)*(1+.22*medium+.13*fine)
 h+=290*np.exp(-((x-700)/690)**2-((y+2100)/520)**2)
 h+=270*np.exp(-((x+980)/680)**2-((y+1570)/570)**2)
 # Reproducible thermal talus relaxation of steep slopes, before corridor locks.
 for _ in range(18):
  delta=np.zeros_like(h)
  for axis in (0,1):
   for shift in (-1,1):
    other=np.roll(h,shift,axis); transfer=np.maximum(h-other-21,0)*.10
    delta-=transfer; delta+=np.roll(transfer,-shift,axis)
  h+=delta
 # Continuous main drainage; tributaries join below their source elevations.
 river=curve([(-700,-14000),(-100,-6000),(-800,-4000),(-550,-2700),(-170,-1600),
              (-430,-850),(-370,-500),(-620,150),(-450,700),(-200,1200),
              (80,1800),(-160,2350),(380,3200),(680,4800),(130,7000),(-600,10000),(450,14000)])
 rd,_=distance(x,y,river)
 riverbed=-24+.004*(1200-y)
 flood=1-smooth(150,620,rd)
 h=h*(1-flood)+(riverbed+48+13*medium)*flood
 bed=1-smooth(65,148,rd); h=h*(1-bed)+riverbed*bed
 tributaries=[curve([(2600,-2800),(1860,-2300),(1390,-1910),(870,-1280),(360,-880),(float(np.interp(-700,river[:,1],river[:,0])),-700)],12),
              curve([(-3800,1850),(-2800,1500),(-1900,1630),(-1000,1710),(float(np.interp(1840,river[:,1],river[:,0])),1840)],12)]
 waterlines=[[[float(px*SCALE),float(py*SCALE),(-8+.004*(1200-py))*SCALE] for px,py in river]]
 for tr in tributaries:
  d,_=distance(x,y,tr); channel=1-smooth(22,90,d)
  # A graded bed falls toward the confluence; surrounding drainage swale is softened.
  confluence=tr[-1]; progress=np.clip(np.hypot(x-confluence[0],y-confluence[1])/np.linalg.norm(tr[0]-confluence),0,1)
  outlet=-24+.004*(1200-confluence[1]); th=outlet+progress*100
  h=np.minimum(h,h*(1-channel)+th*channel)
  waterlines.append([[float(px*SCALE),float(py*SCALE),(outlet+np.linalg.norm(np.array([px,py])-confluence)/np.linalg.norm(tr[0]-confluence)*100+16)*SCALE] for px,py in tr])
 # Terrace the capital, frontier outposts and shrine without flattening entire regions.
 for name,(px,py) in PLACES.items():
  if name=='river_ford': continue
  target=float(sample(h,px,py)); radius=250 if name in ('human_capital','orc_camp') else 145
  w=1-smooth(radius,radius+210,np.hypot(x-px,y-py))
  h=h*(1-w)+target*w
 # Quarry benches and the exposed cut are deliberately asymmetric.
 q=np.hypot((x+860)*.85,y+1680); qw=1-smooth(220,470,q)
 bench=np.floor(np.clip(q/85,0,5))*26+float(sample(h,-700,-1500))-45
 h=h*(1-qw)+bench*qw
 routes=[]
 for a,b,controls in CONTROLS:
  p=curve([PLACES[a]]+controls+[PLACES[b]],12)
  # Relax the grade along the authored path; lock only a narrow corridor.
  z=sample(h,p[:,0],p[:,1]); z=np.convolve(np.pad(z,(3,3),mode='edge'),np.ones(7)/7,mode='valid')
  for (px,py),pz in zip(p,z):
   w=(1-smooth(40,100,np.hypot(x-px,y-py)))*.25
   h=h*(1-w)+pz*w
  routes.append(dict(a=a,b=b,points=p.tolist()))
 # Re-cut river after road grading. Bridges are presentation geometry, not dams.
 h=h*(1-bed)+riverbed*bed
 # Post-carve talus ceiling: tributaries must not leave near-vertical sawtooth
 # banks through a ridge. Lower steep neighbours without filling channel beds.
 # Converge rather than stopping at an arbitrary iteration wavefront, which
 # would merely move the cliff. A 17-unit axial limit bounds diagonal grades.
 for _ in range(512):
  ceiling=np.minimum.reduce([h[:-2,1:-1],h[2:,1:-1],h[1:-1,:-2],h[1:-1,2:]])+17
  new=np.minimum(h[1:-1,1:-1],ceiling)
  change=np.max(h[1:-1,1:-1]-new);h[1:-1,1:-1]=new
  if change<.001:break
 else:raise RuntimeError('Bank relaxation failed to converge')
 for _ in range(3):
  soft=(h[1:-1,1:-1]*4+h[:-2,1:-1]+h[2:,1:-1]+h[1:-1,:-2]+h[1:-1,2:])/8
  h[1:-1,1:-1]=np.minimum(h[1:-1,1:-1],soft)
 quant=np.uint16(np.clip(np.rint(h*8+32768),0,65535)); h=(quant.astype(float)-32768)/8
 # Refit tributary water to the final bed after bank relaxation, then enforce
 # downhill flow into the exact main-river outlet. Do not leave an old ribbon
 # suspended above newly softened terrain.
 for wi,tr in enumerate(tributaries,1):
  profile=sample(h,tr[:,0],tr[:,1])+6
  profile[-1]=-8+.004*(1200-tr[-1,1])
  profile=np.maximum.accumulate(profile[::-1])[::-1]
  waterlines[wi]=[[float(px*SCALE),float(py*SCALE),float(z*SCALE)] for (px,py),z in zip(tr,profile)]
 png16(OUT/'FounderHeight.png',quant); (DATA/'FounderHeight.r16').write_bytes(quant.astype('<u2').tobytes())
 gy,gx=np.gradient(h,2*EXTENT/(N-1)); slope=np.hypot(gx,gy)
 rock=smooth(.28,.85,slope)*.88+smooth(430,1100,h)*.6; rock=np.clip(rock,0,1)
 green=np.array([.13,.22,.065]); dry=np.array([.29,.28,.12]); stone=np.array([.34,.35,.30])
 macro=green+(dry-green)*((broad+1)*.5)[...,None]
 macro=macro*(1-rock[...,None])+stone*rock[...,None]
 bank=1-smooth(110,185,rd); macro=macro*(1-bank[...,None])+np.array([.31,.29,.23])*bank[...,None]
 # Irregular cultivated parcels surrounding the capital, with narrow hedgerow gaps.
 for i,(px,py,w,d,angle) in enumerate(FIELDS):
  u=(x-px)*np.cos(angle)+(y-py)*np.sin(angle); v=-(x-px)*np.sin(angle)+(y-py)*np.cos(angle)
  field=(1-smooth(w-20,w,np.abs(u)))*(1-smooth(d-15,d,np.abs(v)))
  furrow=.90+.10*np.cos(v/11)
  fc=[np.array([.34,.27,.11]),np.array([.16,.23,.08]),np.array([.25,.19,.10])][i%3]
  macro=macro*(1-field[...,None])+fc*furrow[...,None]*field[...,None]
 tga(OUT/'FounderMacro.tga',macro)
 tga(OUT/'FounderBiome.tga',np.stack([rock,slope.clip(0,1),1-smooth(90,200,rd)],axis=-1))
 # Presentation crossings are derived from authored routes/drainage, never edges
 # in the campaign simulation. Decks and travelling parties share these values.
 bridges=[]
 def cross(a,b):return a[0]*b[1]-a[1]*b[0]
 for route in routes:
  for a,b in zip(route['points'][:-1],route['points'][1:]):
   a,b=np.array(a),np.array(b);d=b-a
   for wi,water in enumerate([river]+tributaries):
    for c,e in zip(water[:-1],water[1:]):
     v=e-c;den=cross(d,v)
     if abs(den)<1e-8:continue
     t,s=cross(c-a,v)/den,cross(c-a,d)/den
     if not (0<=t<1 and 0<=s<1):continue
     p=a+t*d;direction=d/np.linalg.norm(d)
     ford=np.linalg.norm(p-np.array(PLACES['river_ford']))<80
     if ford:p=np.array(PLACES['river_ford'],dtype=float);direction=np.array([1.,0.])
     if any(np.linalg.norm(p-np.array(q['center'][:2])/SCALE)<35 for q in bridges):continue
     sine=abs(cross(direction,v/np.linalg.norm(v)))
     half=210 if ford else min(350,(160 if wi==0 else 85)/max(sine,.35))
     deck=18 if ford else max(float(sample(h,*(p-direction*half))),float(sample(h,*(p+direction*half))))+8
     bridges.append(dict(a=route['a'],b=route['b'],center=[float(p[0]*SCALE),float(p[1]*SCALE),float(deck*SCALE)],
       yaw=float(np.degrees(np.arctan2(direction[1],direction[0]))),half_span=float(half*SCALE),ford=bool(ford)))
 manifest=dict(schema=1,resolution=N,half_extent=EXTENT,scale=SCALE,height_encoding='u16le; local_z=(value-32768)/8',
  regions={k:[px*SCALE,py*SCALE,(10 if k=='river_ford' else float(sample(h,px,py)))*SCALE] for k,(px,py) in PLACES.items()},
  routes=[dict(a=r['a'],b=r['b'],points=[[px*SCALE,py*SCALE] for px,py in r['points']]) for r in routes],
  river=[[float(px*SCALE),float(py*SCALE)] for px,py in river],waterlines=waterlines,bridges=bridges,fields=[[px*SCALE,py*SCALE,w*SCALE,d*SCALE,float(np.degrees(angle))] for px,py,w,d,angle in FIELDS],
  height_sha256=hashlib.sha256((DATA/'FounderHeight.r16').read_bytes()).hexdigest())
 (DATA/'presentation.json').write_text(json.dumps(manifest,indent=2)+'\n')
 (OUT/'bake_receipt.json').write_text(json.dumps(dict(resolution=N,min_height=float(h.min()),max_height=float(h.max()),height_sha256=manifest['height_sha256']),indent=2)+'\n')
 print('BAKE_COMPLETE',N,manifest['height_sha256'])

if __name__=='__main__': main()
