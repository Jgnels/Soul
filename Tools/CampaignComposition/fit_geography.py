"""Analytical hydrology and physical region fitting; never changes canonical graph."""
from pathlib import Path
import numpy as np,json,math,heapq,collections,time
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'Evidence/ProductionWorldComposition-20261007';LOCAL=OUT/'Local';SIDE=3500.;N=511;CELL=SIDE/(N-1);RES=2041;STEP=SIDE/(RES-1)
z=np.load(LOCAL/'proposed-relief-before-rivers.npy');original=z.copy()
def sample(p,field=None):
 field=z if field is None else field;p=np.clip(np.asarray(p)/SIDE*(len(field)-1),0,len(field)-1.000001);i=p.astype(int);f=p-i;x,y=i[...,0],i[...,1];a,b,c,d=field[y,x],field[y,x+1],field[y+1,x],field[y+1,x+1];fx,fy=f[...,0],f[...,1];return np.where(fx>=fy,a+(b-a)*fx+(d-b)*fy,a+(d-c)*fx+(c-a)*fy)
def smooth(v):v=np.clip(v,0,1);return v*v*(3-2*v)
hydro=np.load(LOCAL/'drainage-tree.npz');filled=hydro['filled'];basin_labels=np.load(LOCAL/'lake-basin-labels.npy');basin_rows=json.loads((OUT/'lake-basin-analysis.json').read_text())['basins'];lake_surface=np.full(z.shape,-999.,np.float32);lakes=[]
for target in [[590,1674],[878,2141]]:
 ix,iy=np.rint(np.array(target)/CELL).astype(int);lab=int(basin_labels[iy,ix]);row=next(r for r in basin_rows if r['id']==lab);level=row['surface_m'];small=basin_labels==lab
 # Extend coarse basin support by one cell; shore itself uses full source relief.
 support=small.copy()
 for dx,dy in [(1,0),(-1,0),(0,1),(0,-1),(1,1),(-1,-1),(-1,1),(1,-1)]:support|=np.roll(np.roll(small,dy,0),dx,1)
 gy,gx=np.mgrid[:RES,:RES];sx=np.clip(np.rint(gx/4).astype(int),0,N-1);sy=np.clip(np.rint(gy/4).astype(int),0,N-1);wet=support[sy,sx]&(z<level);lake_surface[wet]=level;lakes.append(dict(row,role='Human capital lake' if lab==int(basin_labels[round(1674/CELL),round(590/CELL)]) else 'Heart River lower lake'))
np.save(LOCAL/'lake-water.npy',lake_surface)
(OUT/'selected-lakes.json').write_text(json.dumps({'lakes':lakes,'global_ocean_unchanged_m':0,'source':'native closed basins at measured ocean-directed spill elevation; no basin flattening'},indent=2))
candidates=json.loads((OUT/'drainage-candidates.json').read_text());water=np.full(z.shape,-999.,np.float32);river_mask=np.zeros(z.shape,np.float32);rivers=[]
for name,key,idx,width in [('Heart River','greenwood',0,7),('Heartland tributary','heartland',0,4),('Heartland northern tributary','heartland',1,3),('Eastern Run','eastern_run',3,5)]:
 source=np.array(candidates[key][idx]['points']);points=source[:,:2].copy()
 # Restrained corner rounding within the measured valley, no global smoothing.
 for _ in range(2):points[1:-1]=points[1:-1]*.5+(points[:-2]+points[2:])*.25
 dense=[]
 for a,b in zip(points[:-1],points[1:]):dense.extend(a+(b-a)*np.linspace(0,1,max(2,math.ceil(np.linalg.norm(b-a)/1.5)),endpoint=False)[:,None])
 dense=np.array(dense+[points[-1]]);h=sample(dense,filled);h=np.minimum.accumulate(h);h=np.maximum(h,0);xyz=np.column_stack([dense,h]);before=z.copy()
 for x,y,hh in xyz:
  ix,iy=round(x/STEP),round(y/STEP);r=math.ceil((width/2+4)/STEP);x0,x1=max(0,ix-r),min(RES,ix+r+1);y0,y1=max(0,iy-r),min(RES,iy+r+1);gy,gx=np.mgrid[y0:y1,x0:x1];d=np.hypot(gx*STEP-x,gy*STEP-y);weight=smooth((width/2+4-d)/4);bed=hh-.9
  sub=z[y0:y1,x0:x1];z[y0:y1,x0:x1]=np.minimum(sub,sub+(bed-sub)*weight);inside=d<=width/2;water[y0:y1,x0:x1]=np.where(inside,np.maximum(water[y0:y1,x0:x1],hh),water[y0:y1,x0:x1]);river_mask[y0:y1,x0:x1]=np.maximum(river_mask[y0:y1,x0:x1],weight)
 rivers.append({'id':name.lower().replace(' ','_'),'name':name,'width_m':width,'length_m':float(np.linalg.norm(np.diff(xyz[:,:2],axis=0),axis=1).sum()),'points_xyz_m':xyz[::2].tolist()+[xyz[-1].tolist()],'maximum_cut_m':float((before-z).max()),'downstream_rise_m':float(np.maximum(np.diff(h),0).max()),'source':key+' candidate '+str(idx)})
np.save(LOCAL/'composed-height.npy',z);np.save(LOCAL/'river-water.npy',water);np.save(LOCAL/'river-mask.npy',river_mask)
(OUT/'river-skeleton.json').write_text(json.dumps({'ocean_z_m':0,'rivers':rivers,'terrain_changed_m2':float((original!=z).sum()*STEP**2),'max_cut_m':float((original-z).max()),'note':'Heart River flows north down the native Greenwood valley, through its natural spill-level lake and onward to the central ocean-connected inlet. Physical drainage overrides the old coarse north-to-south sketch; IDs/adjacency untouched.'},indent=2))
# Crossing windows are selected on actual drainage lines, not arbitrary wet roads.
# Block the rest of each channel and its shoulder from road searches.
gates=[];gate_mask=np.zeros(z.shape,bool)
for name,ri,target,kind in [('river_ford',0,[803,2169],'ford'),('southern_crossing',0,[652,2320],'bridge'),('woodland_bridge',0,[476,2820],'bridge'),('human_west_bridge',1,[612,1764],'bridge'),('human_north_bridge',2,[714,1584],'bridge'),('orc_broken_bridge',3,[3054,1853],'bridge')]:
 p=np.array(rivers[ri]['points_xyz_m']);j=np.linalg.norm(p[:,:2]-target,axis=1).argmin();center=p[j,:2];tangent=p[min(len(p)-1,j+8),:2]-p[max(0,j-8),:2];tangent/=np.linalg.norm(tangent);normal=np.array([-tangent[1],tangent[0]]);half=rivers[ri]['width_m']/2+15
 selected=next(r for r in json.loads((OUT/'selected-crossing-sites-r3.json').read_text())['crossings'] if r['gate_id']==name);center=np.array(selected['center_xy_m']);angle=math.radians(selected['yaw_deg']);normal=np.array([math.cos(angle),math.sin(angle)]);tangent=np.array([-normal[1],normal[0]]);half=selected['span_m']/2+7
 iy,ix=np.mgrid[:RES,:RES];delta=np.stack([ix*STEP-center[0],iy*STEP-center[1]],-1);gate_mask |= (np.abs(delta@normal)<half)&(np.abs(delta@tangent)<8)
 gates.append({'id':name,'type':kind,'river_id':rivers[ri]['id'],'center_xy_m':center.tolist(),'bank_endpoints_xy_m':[(center-normal*half).tolist(),(center+normal*half).tolist()],'crossing_direction_xy':normal.tolist(),'water_z_m':selected['water_z_m'],'half_span_m':half})
# A bounded shallow ford sill, confined to the selected crossing's wet channel.
ford=next(g for g in gates if g['type']=='ford');n=np.array(ford['crossing_direction_xy']);t=np.array([-n[1],n[0]]);iy,ix=np.mgrid[:RES,:RES];delta=np.stack([ix*STEP-ford['center_xy_m'][0],iy*STEP-ford['center_xy_m'][1]],-1);fw=smooth((6-np.abs(delta@t))/3)*(np.abs(delta@n)<ford['half_span_m'])*(water>-900);before_ford=z.copy();z=np.maximum(z,z+(water-.25-z)*fw);np.save(LOCAL/'composed-height.npy',z);(OUT/'ford-sill-edit.json').write_text(json.dumps({'max_raise_m':float((z-before_ford).max()),'area_m2':float((z!=before_ford).sum()*STEP**2),'depth_at_full_weight_m':.25,'purpose':'single shallow physical ford, no basin flattening'},indent=2))
exclusion=((river_mask>.10)|(lake_surface>-900))&~gate_mask;np.save(LOCAL/'road-water-exclusion.npy',exclusion)
(OUT/'designed-crossing-gates.json').write_text(json.dumps({'gates':gates,'rule':'Only these narrow cross-river windows may interrupt the dry road exclusion. No water-parallel road corridors.'},indent=2))
def excluded(points):
 i=np.clip(np.rint(np.asarray(points)/SIDE*2040).astype(int),0,2040);return exclusion[i[...,1],i[...,0]]

# Cost field: road corridors, not direct adjacency artwork.
yy,xx=np.mgrid[:N,:N];grid=np.stack([xx,yy],-1)*CELL;g=sample(grid);sea=g<=0
# Label boundary-connected ocean distinctly from enclosed water.
ocean=np.zeros((N,N),bool);todo=collections.deque()
for y,x in zip(*np.nonzero(sea)):
 if x in (0,N-1) or y in (0,N-1):ocean[y,x]=True;todo.append((x,y))
MOVES=[(1,0),(-1,0),(0,1),(0,-1),(1,1),(1,-1),(-1,1),(-1,-1)]
while todo:
 x,y=todo.popleft()
 for dx,dy in MOVES:
  nx,ny=x+dx,y+dy
  if 0<=nx<N and 0<=ny<N and sea[ny,nx] and not ocean[ny,nx]:ocean[ny,nx]=True;todo.append((nx,ny))
np.save(LOCAL/'ocean-connected-mask.npy',ocean)
# Plan on pre-channel ground: shallow river crossings get explicit bridge/ford
# decks rather than forcing the road to climb the carved riverbanks.
costs=[]
for dx,dy in MOVES:
 delta=np.array([dx,dy])*CELL;length=np.linalg.norm(delta);steps=math.ceil(length/.6);h=np.stack([sample(grid+delta*t/steps,original) for t in range(steps+1)]);grade=np.abs(np.diff(h,axis=0))/(length/steps)
 normal=np.array([-dy,dx])/math.hypot(dx,dy);bank=np.abs(sample(grid+normal*1.5,original)-sample(grid-normal*1.5,original))/3
 wet_block=np.stack([excluded(grid+delta*t/steps) for t in range(steps+1)]).any(0)
 valid=(~wet_block)&(h.min(0)>1.2)&(grade.max(0)<=math.tan(math.radians(21.2)))&(bank<math.tan(math.radians(28)))&(xx+dx>1)&(xx+dx<N-2)&(yy+dy>1)&(yy+dy<N-2)
 cost=length*(1+14*np.mean(grade**2,axis=0)+4*bank**2+.0008*np.maximum(h.mean(0),0));cost[~valid]=np.inf;costs.append(cost)
costs=np.array(costs);viable=np.isfinite(costs).sum(0)>=3
# Components let placement avoid tiny, isolated shelves without flattening.
labels=np.zeros((N,N),np.int16);sizes=[]
for y,x in zip(*np.nonzero(viable)):
 if labels[y,x]:continue
 label=len(sizes)+1;labels[y,x]=label;todo=collections.deque([(x,y)]);count=0
 while todo:
  ax,ay=todo.popleft();count+=1
  for k,(dx,dy) in enumerate(MOVES):
   nx,ny=ax+dx,ay+dy
   if 0<nx<N-1 and 0<ny<N-1 and viable[ny,nx] and not labels[ny,nx] and np.isfinite(costs[k,ay,ax]):labels[ny,nx]=label;todo.append((nx,ny))
 sizes.append(count)
large=np.isin(labels,np.argsort(sizes)[-2:]+1)&viable
# Manually directed geographic targets; final points choose existing usable terrain.
targets={
'viking_harbour':(610,1030),'viking_fjord_ridge':(580,250),'viking_forest_track':(560,710),'viking_snow_pass':(850,470),
'mountain_shrine':(1780,860),'northwest_march':(700,1200),'dwarf_high_quarry':(2050,490),'dwarf_snow_basin':(2540,260),'dwarf_forge_approach':(2560,830),'dwarf_hold':(2800,610),'dwarf_mountain_pass':(2100,960),
'human_capital':(780,1610),'crossroads':(1000,1840),'old_quarry':(880,1380),'river_ford':(775.7,2196.3),'forest_edge':(1040,1570),'ancient_shrine':(1000,1120),
'orc_watch':(2280,1830),'north_pass':(2190,1400),'orc_camp':(2780,1640),'orc_badlands':(3090,1230),'orc_war_camp':(2880,1040),'orc_ruined_field':(3160,1650),'orc_broken_bridge':(3053,1821.4),
'coastal_ruins':(250,1900),'nature_shrine':(330,2360),'nature_forest_clearing':(320,2710),'nature_treehold':(710,2990),'nature_river_woodland':(500,2820),'nature_grassland_edge':(870,2530),'southern_crossing':(624.5,2347.1),
'dark_corrupted_valley':(1630,2680),'dark_ruined_causeway':(2100,2510),'dark_ash_plain':(2250,3040),'dark_castle_approach':(2630,3110),'dark_fortress':(2920,3260)}
world=json.loads((ROOT/'Data/soul_world_overmap_v1_20260922.json').read_text());anchors=[];occupied=[]
# Shore distance approximates docks; ford/bridge sites chosen directly along rivers.
def pick(target,radius=180,footprint=None,extra=None):
 dist=np.linalg.norm(grid-target,axis=-1);valid=large&(dist<radius)
 if extra is not None:valid &=extra
 score=dist.copy()
 if footprint:
  wx,wy=footprint;hs=np.stack([sample(grid+[dx*wx/2,dy*wy/2],original) for dx in [-1,0,1] for dy in [-1,0,1]]);relief=hs.max(0)-hs.min(0);score+=relief*45;valid &=hs.min(0)>2
 if occupied:
  for p in occupied:valid &=np.linalg.norm(grid-p,axis=-1)>105
 score[~valid]=np.inf
 if not np.isfinite(score).any():raise RuntimeError('no site '+str(target))
 y,x=np.unravel_index(score.argmin(),score.shape);return [float(x*CELL),float(y*CELL)]
for node in world['nodes']:
 key=node['id'];fp=(69.7,51.4) if key=='human_capital' else (30.5,38.7) if key=='dwarf_hold' else (25,25) if node['kind']=='capital' else None
 if key in ('river_ford','southern_crossing','nature_river_woodland','orc_broken_bridge'):
  river=rivers[3] if key=='orc_broken_bridge' else rivers[0];p=np.array(river['points_xyz_m']);j=np.linalg.norm(p[:,:2]-targets[key],axis=1).argmin();target=p[j,:2];xy=pick(target,80,None,(river_mask[::4,::4]<.05) if key=='nature_river_woodland' else None)
 elif key=='viking_harbour':
  # Directly require a nearby boundary-connected ocean sample.
  shore=np.zeros((N,N),bool)
  for dy in range(-10,11):
   for dx in range(-10,11):
    if dx*dx+dy*dy<=100:shore |= np.roll(np.roll(ocean,dy,0),dx,1)
  xy=pick(targets[key],380,fp,shore)
 else:xy=pick(targets[key],220 if fp else 150,fp)
 occupied.append(xy);x,y=np.rint(np.array(xy)/CELL).astype(int);row={'id':key,'macro_region':node['macro_region'],'kind':node['kind'],'target_xy_m':targets[key],'xy_m':xy,'height_m':float(sample(xy)),'road_component':int(labels[y,x])}
 if fp:
  hs=sample(np.array(xy)+np.array([[dx*fp[0]/2,dy*fp[1]/2] for dx in [-1,0,1] for dy in [-1,0,1]]));row.update(footprint_m=fp,footprint_relief_m=float(hs.max()-hs.min()))
 anchors.append(row)
(OUT/'region-placement-proposal.json').write_text(json.dumps({'side_m':SIDE,'anchors':anchors,'road_component_sizes':sorted(sizes,reverse=True)[:15],'limitations':['analytical sites; actual native collision/miniature seating not qualified','minor site targets can move within macro relationships','no terrain flattening for placements']},indent=2))
np.savez(LOCAL/'routing-grid.npz',costs=costs,labels=labels,grid=grid,original=original,terrain=z)
# Plot actual physical locations and river skeleton before fitting edges.
dz=z[::2,::2];gy,gx=np.gradient(dz,SIDE/1020);shade=np.clip((.8-.45*gx-.3*gy)/np.sqrt(1+gx*gx+gy*gy),.16,1);h=np.clip(dz/160,0,1);rgb=np.stack([(.27+h*.4)*shade,(.43+h*.25)*shade,(.21+h*.5)*shade],-1);rgb[dz<=0]=[.06,.2,.29];im=Image.fromarray(np.uint8(np.clip(rgb,0,1)*255));d=ImageDraw.Draw(im);font=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',13)
for r in rivers:d.line([(x/3500*1020,y/3500*1020) for x,y,_ in r['points_xyz_m']],fill='#9dcde0',width=2)
colors=dict(northern_fjords='#e7ecef',crownspine='#e7a63d',heartland='#d3df79',eastern_badlands='#e88450',greenwood='#84d294',ashen_south='#be94dd')
for a in anchors:
 x,y=np.array(a['xy_m'])/3500*1020;col=colors[a['macro_region']];d.ellipse((x-4,y-4,x+4,y+4),fill=col);d.text((x+5,y-6),a['id'],fill=col,font=font)
im.save(LOCAL/'region-composition-proposal.png');print(json.dumps({'anchors':anchors,'rivers':[{k:v for k,v in r.items() if k!='points_xyz_m'} for r in rivers]},indent=2))
