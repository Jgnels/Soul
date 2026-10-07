"""Find short bridge opportunities on unchanged relief at the native water datum.

This is an analytical shortlist, not permission to add gameplay edges, a bridge
asset, a river or a traversable route. Mountain access still needs dense fitting.
"""
from pathlib import Path
from collections import deque
import json,math
import numpy as np
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'Evidence/TerrainFoundation-20261007'
raw=np.load('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview/TerrainWork/mountain05-height.npy').astype(float)
z=np.rot90(((raw-32768)*300/128+100+63390)/100+5,1)*3500/8160
g=z[::4,::4];N=len(g);cell=3500/(N-1);water=5*3500/8160
dry=g>water+1.2;labels=np.full(g.shape,-1,np.int32);areas=[]
moves=[(1,0),(-1,0),(0,1),(0,-1)]
for y,x in zip(*np.nonzero(dry)):
 if labels[y,x]>=0:continue
 label=len(areas);labels[y,x]=label;q=deque([(x,y)]);count=0
 while q:
  xx,yy=q.popleft();count+=1
  for dx,dy in moves:
   nx,ny=xx+dx,yy+dy
   if 0<=nx<N and 0<=ny<N and dry[ny,nx] and labels[ny,nx]<0:labels[ny,nx]=label;q.append((nx,ny))
 areas.append(count*cell*cell)
def sample(p):
 p=np.clip(np.array(p)/3500*2040,0,2039.999);i=p.astype(int);f=p-i;x,y=i[:,0],i[:,1];fx,fy=f[:,0],f[:,1]
 a,b,c,d=z[y,x],z[y,x+1],z[y+1,x],z[y+1,x+1]
 return np.where(fx>=fy,a+(b-a)*fx+(d-b)*fy,a+(d-c)*fx+(c-a)*fy)
boundary=dry.copy()
inside=np.zeros_like(dry);inside[1:-1,1:-1]=dry[:-2,1:-1]&dry[2:,1:-1]&dry[1:-1,:-2]&dry[1:-1,2:]
boundary&=~inside
candidates=[]
directions=[(dx,dy) for dx,dy in [(1,0),(0,1),(1,1),(1,-1),(2,1),(2,-1),(1,2),(1,-2)]]
for y,x in zip(*np.nonzero(boundary)):
 label=int(labels[y,x])
 if areas[label]<5000:continue
 for dx,dy in directions:
  step=cell*math.hypot(dx,dy)
  for n in range(1,int(140/step)+1):
   xx,yy=x+dx*n,y+dy*n
   if not(1<=xx<N-1 and 1<=yy<N-1):break
   other=int(labels[yy,xx])
   if other<0:continue
   if other==label or areas[other]<5000:break
   a=np.array([x,y])*cell;b=np.array([xx,yy])*cell;length=np.linalg.norm(b-a)
   if length<15:break
   p=a+(b-a)*np.linspace(0,1,math.ceil(length/.8)+1)[:,None];h=sample(p)
   deck=np.linspace(h[0]+.5,h[-1]+.5,len(p));grade=math.degrees(math.atan(abs(h[-1]-h[0])/length))
   # Reject hidden land ridges, deep-water viaducts and steep bridge decks.
   if h.min()>water-.1 or np.max(deck-h)>8 or np.min(deck-h)<.1 or grade>8:break
   candidates.append({'banks_xy_m':[a.tolist(),b.tolist()],'center_xy_m':((a+b)*.5).tolist(),'dry_components':[label,other],'span_m':float(length),'deck_grade_deg':grade,'maximum_deck_height_above_ground_m':float((deck-h).max()),'minimum_deck_above_water_m':float(deck.min()-water),'maximum_water_depth_m':float(water-h.min())});break
# Keep physically distinct short opportunities instead of hundreds of nearby rays.
short=[]
for c in sorted(candidates,key=lambda c:c['span_m']+c['maximum_deck_height_above_ground_m']*3):
 if any(math.dist(c['center_xy_m'],s['center_xy_m'])<100 for s in short):continue
 short.append(c)
fit=json.loads((OUT/'dense-route-fit-r5.json').read_text());anchors={a['id']:a for a in fit['anchors']}
named=[]
for name in ['river_ford','southern_crossing','orc_broken_bridge']:
 closest=sorted(short,key=lambda c:math.dist(c['center_xy_m'],anchors[name]['xy_m']))[:3]
 named.append({'id':name,'current_xy_m':anchors[name]['xy_m'],'nearest_opportunities':[dict(c,anchor_distance_m=math.dist(c['center_xy_m'],anchors[name]['xy_m'])) for c in closest]})
receipt={'heightfield_modified':False,'water_candidate_z_m':water,'dry_bank_clearance_m':1.2,'dry_components_over_5000m2':sum(a>=5000 for a in areas),'opportunities':short,'named_sites':named,'limitations':['Shoreline component test ignores mountain-road access.','Bridge decks are analytical straight profiles, not licensed bridge assets or construction plans.','A bridge over a sea inlet does not satisfy the Heart River/Ford design.','No canonical connection is added or removed; no world/asset is modified.']}
(OUT/'crossing-opportunities.json').write_text(json.dumps(receipt,indent=2))
dy,dx=np.gradient(g,cell);shade=np.clip((.85-.4*dx-.3*dy)/np.sqrt(1+dx*dx+dy*dy),.2,1)
rgb=np.stack([shade*.44,shade*.5,shade*.31],-1);rgb[g<=water]=[.08,.25,.35]
im=Image.fromarray(np.uint8(rgb*255)).resize((1022,1022));d=ImageDraw.Draw(im);font=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',18)
for i,c in enumerate(short):
 a,b=np.array(c['banks_xy_m'])/3500*1022;d.line([tuple(a),tuple(b)],fill='#ffb347',width=4);d.text(tuple((a+b)*.5),(f'{i}: {c["span_m"]:.0f}m'),font=font,fill='white')
for name in named:
 p=np.array(name['current_xy_m'])/3500*1022;d.ellipse((*tuple(p-5),*tuple(p+5)),fill='#dd6688');d.text(tuple(p+[6,6]),name['id'].replace('_',' '),font=font,fill='white')
im.save(OUT/'crossing-opportunities.png');print(json.dumps({'opportunities':len(short),'named_sites':[(n['id'],[round(c['anchor_distance_m']) for c in n['nearest_opportunities']]) for n in named]}))
