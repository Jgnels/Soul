import numpy as np,json,collections
from pathlib import Path
E=Path('Evidence/ProductionWorldComposition-20261007');d=np.load(E/'Local/drainage-tree.npz');z=d['terrain'];f=d['filled'];o=d['ocean'];cell=float(d['cell']);mask=(f-z>.12)&~o;labels=np.zeros(z.shape,np.int16);rows=[]
for y,x in zip(*np.nonzero(mask)):
 if labels[y,x]:continue
 lab=len(rows)+1;queue=collections.deque([(x,y)]);labels[y,x]=lab;cells=[]
 while queue:
  a,b=queue.popleft();cells.append([a,b])
  for dx,dy in [(1,0),(-1,0),(0,1),(0,-1)]:
   aa,bb=a+dx,b+dy
   if 0<=aa<len(z) and 0<=bb<len(z) and mask[bb,aa] and not labels[bb,aa]:labels[bb,aa]=lab;queue.append((aa,bb))
 p=np.array(cells);rows.append({'id':lab,'area_m2':len(p)*cell**2,'bounds_m':[*(p.min(0)*cell).tolist(),*(p.max(0)*cell).tolist()],'center_m':(p.mean(0)*cell).tolist(),'surface_m':float(np.median(f[p[:,1],p[:,0]])),'max_depth_m':float((f-z)[p[:,1],p[:,0]].max())})
for target in [[590,1674],[878,2141]]:
 ix,iy=np.rint(np.array(target)/cell).astype(int);lab=labels[iy,ix];print('TARGET',target,'label',lab,'spill',f[iy,ix]);print(rows[lab-1] if lab else None)
print('LARGEST',json.dumps(sorted(rows,key=lambda r:r['area_m2'],reverse=True)[:8]));(E/'lake-basin-analysis.json').write_text(json.dumps({'basins':rows},indent=2));np.save(E/'Local/lake-basin-labels.npy',labels)
