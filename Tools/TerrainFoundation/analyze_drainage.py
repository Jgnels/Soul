"""Read-only hydrology/shoreline diagnostics, not a terrain or river authoring tool."""
from pathlib import Path
from collections import deque
import heapq,json,math
import numpy as np
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'Evidence/TerrainFoundation-20261007'
raw=np.load('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview/TerrainWork/mountain05-height.npy').astype(float)
z=np.rot90(((raw-32768)*300/128+100+63390)/100+5,1)*3500/8160
grid=z[::4,::4];N=len(grid);cell=3500/(N-1);moves=[(1,0),(-1,0),(0,1),(0,-1),(1,1),(1,-1),(-1,1),(-1,-1)]
def components(mask):
 labels=np.full(mask.shape,-1,dtype=np.int32);rows=[]
 for y,x in zip(*np.nonzero(mask)):
  if labels[y,x]>=0:continue
  idx=len(rows);q=deque([(x,y)]);labels[y,x]=idx;points=[];edge=False
  while q:
   xx,yy=q.popleft();points.append((xx,yy));edge|=xx in (0,N-1) or yy in (0,N-1)
   for dx,dy in moves[:4]:
    nx,ny=xx+dx,yy+dy
    if 0<=nx<N and 0<=ny<N and mask[ny,nx] and labels[ny,nx]<0:labels[ny,nx]=idx;q.append((nx,ny))
  ar=np.array(points);rows.append({'id':idx,'area_m2':len(points)*cell*cell,'touches_boundary':bool(edge),'centroid_m':(ar.mean(axis=0)*cell).tolist(),'points':points})
 return labels,rows
# Priority flood estimates spill elevations only. Its filled array is NEVER
# exported to Unreal. The candidate retains every original source sample.
filled=grid.copy();seen=np.zeros((N,N),bool);parent=np.full((N,N,2),-1,dtype=np.int16);order=[];q=[]
for y,x in zip(*np.indices((N,N))[:,np.logical_or.reduce([np.indices((N,N))[0]==0,np.indices((N,N))[0]==N-1,np.indices((N,N))[1]==0,np.indices((N,N))[1]==N-1])]):
 if not seen[y,x]:seen[y,x]=True;heapq.heappush(q,(grid[y,x],int(x),int(y)))
while q:
 h,x,y=heapq.heappop(q);order.append((x,y))
 for dx,dy in moves:
  nx,ny=x+dx,y+dy
  if not(0<=nx<N and 0<=ny<N) or seen[ny,nx]:continue
  seen[ny,nx]=True;parent[ny,nx]=[x,y];filled[ny,nx]=max(h,grid[ny,nx]);heapq.heappush(q,(filled[ny,nx],nx,ny))
acc=np.ones((N,N),dtype=np.int64)
for x,y in reversed(order):
 px,py=parent[y,x]
 if px>=0:acc[py,px]+=acc[y,x]
report={'grid_spacing_m':cell,'method':'priority-flood spill analysis on downsampled original relief; parent-tree accumulation is a drainage possibility, not an authored river','terrain_modified':False,'scenarios':[]}
for name,datum in [('candidate_lower_datum',0),('original_native_datum',5*3500/8160)]:
 labels,rows=components(grid<=datum);major=[]
 for r in rows:
  if r['area_m2']<3000:continue
  pts=np.array(r.pop('points'));r['spill_elevation_candidate_m']=float(filled[pts[:,1],pts[:,0]].max());r['water_elevation_candidate_m']=datum;r['rise_to_spill_m']=max(0,r['spill_elevation_candidate_m']-datum);major.append(r)
 report['scenarios'].append({'name':name,'water_z_m':datum,'water_fraction':float(np.mean(z<=datum)),'major_water_bodies':sorted(major,key=lambda r:r['area_m2'],reverse=True)})
# Evaluate how many existing dry-study paths would be affected by restoring the
# original water datum. Do not mislabel a sea-level choice as crossing proof.
routes=json.loads((OUT/'dense-route-fit-r5.json').read_text());wet=[];datum=5*3500/8160
for r in routes['routes']:
 pts=np.array(r['dense_points_xyz_m']);under=pts[:,2]<=datum;longest=current=0.;total=0.
 for i in range(1,len(pts)):
  length=float(np.linalg.norm(pts[i,:2]-pts[i-1,:2]))
  if under[i] or under[i-1]:current+=length;total+=length;longest=max(longest,current)
  else:current=0
 if total:wet.append({'a':r['a'],'b':r['b'],'submerged_length_m':total,'longest_contiguous_submerged_m':longest,'maximum_depth_m':float(max(0,datum-pts[:,2].min()))})
report['routes_affected_at_original_water_datum']=wet
report['named_water_sites']=[]
for a in routes['anchors']:
 if not any(s in a['id'] for s in ['ford','crossing','river','harbour','bridge']):continue
 x,y=np.rint(np.array(a['xy_m'])/cell).astype(int)
 wy,wx=np.nonzero(grid<=datum);distance=(wx-x)**2+(wy-y)**2;j=int(distance.argmin());body=int(labels[wy[j],wx[j]])
 report['named_water_sites'].append({'id':a['id'],'height_candidate_m':float(grid[y,x]),'height_above_native_water_m':float(grid[y,x]-datum),'upstream_parent_tree_area_m2':int(acc[y,x])*cell*cell,'nearest_water_distance_m':float(math.sqrt(distance[j])*cell),'nearest_water_body_id':body,'nearest_water_connects_to_boundary':rows[body]['touches_boundary'],'note':'name does not establish a physical river/crossing; a low coastal-looking site can face an enclosed lake'})
(OUT/'drainage-study.json').write_text(json.dumps(report,indent=2))
dy,dx=np.gradient(grid,cell);shade=np.clip((.85-.4*dx-.3*dy)/np.sqrt(1+dx*dx+dy*dy),.2,1);rgb=np.stack([shade*.44,shade*.5,shade*.31],-1);rgb[grid<=datum]=[.08,.25,.35];rgb[(grid>0)&(grid<=datum)]=[.75,.49,.16]
im=Image.fromarray(np.uint8(rgb*255)).resize((1022,1022));d=ImageDraw.Draw(im);font=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',20)
for a in report['named_water_sites']:
 n=next(n for n in routes['anchors'] if n['id']==a['id']);x,y=np.array(n['xy_m'])/3500*1022;d.ellipse((x-4,y-4,x+4,y+4),fill='white');d.text((x+6,y),a['id'].replace('_',' '),font=font,fill='white')
im.save(OUT/'shoreline-sensitivity.png');print(json.dumps({'major_water_bodies':[len(s['major_water_bodies']) for s in report['scenarios']],'routes_affected':len(wet),'largest_lake_spill_rises':[r['rise_to_spill_m'] for r in report['scenarios'][1]['major_water_bodies'][:6]]}))
