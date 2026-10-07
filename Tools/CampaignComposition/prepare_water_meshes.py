"""Deterministic water surfaces from the measured drainage and native basins."""
from pathlib import Path
import json, math
import numpy as np

ROOT=Path(__file__).resolve().parents[2]
E=ROOT/'Evidence/ProductionWorldComposition-20261007'
L=E/'Local'
lake=np.load(L/'lake-water.npy')
meshes=[]
for river in json.loads((E/'river-skeleton.json').read_text())['rivers']:
    pts=river['points_xyz_m']; vertices=[]; uv=[]; triangles=[]; run=0.
    for i,p in enumerate(pts):
        before=pts[max(0,i-1)]; after=pts[min(len(pts)-1,i+1)]
        dx,dy=after[0]-before[0],after[1]-before[1]
        length=math.hypot(dx,dy)
        if length<1e-6: dx,dy,length=1.,0.,1.
        nx,ny=-dy/length,dx/length
        if i: run+=math.dist(p[:2],pts[i-1][:2])
        for sign in [-1,1]:
            vertices.append([(p[0]+sign*nx*river['width_m']/2)*100-175000,(p[1]+sign*ny*river['width_m']/2)*100-175000,p[2]*100+2])
            uv.append([(sign+1)*river['width_m']/8,run/4])
        if i:
            b=i*2; triangles.extend([[b-2,b-1,b],[b-1,b+1,b]])
    # Avoid translucent double surfaces through lakes and the ocean outlet.
    kept=[]
    for tri in triangles:
        center=np.mean([vertices[i] for i in tri],axis=0);ix,iy=np.clip(np.rint((center[:2]+175000)/350000*2040).astype(int),0,2040)
        if center[2]<=2.01 or (lake[iy,ix]>-900 and abs(center[2]/100-lake[iy,ix])<.2):continue
        kept.append(tri)
    meshes.append(dict(id=river['id'],vertices_cm=vertices,uv=uv,triangles=kept))

lake=np.load(L/'lake-water.npy'); ground=np.load(L/'composed-height.npy'); step=350000/2040
for index,level in enumerate(sorted(float(v) for v in np.unique(lake) if v>-900)):
    wet=lake==level; support=wet.copy()
    for dy,dx in [(1,0),(-1,0),(0,1),(0,-1),(1,1),(-1,-1),(-1,1),(1,-1)]:support|=np.roll(np.roll(wet,dy,0),dx,1)
    ids={}; vertices=[]; triangles=[]; uv=[]
    def vertex(y,x):
        key=(round(float(y),6),round(float(x),6))
        if key not in ids:
            ids[key]=len(vertices); vertices.append([x*step-175000,y*step-175000,level*100+2]); uv.append([x*step/400,y*step/400])
        return ids[key]
    # Clip the actual Landscape triangle planes at water level. This avoids the
    # discarded iteration's staircase caused by requiring whole wet triangles.
    for y,x in zip(*np.where(support[:-1,:-1])):
        for corners in [[(y,x),(y,x+1),(y+1,x+1)],[(y,x),(y+1,x+1),(y+1,x)]]:
            poly=[]
            for a,b in zip(corners,corners[1:]+corners[:1]):
                ha,hb=ground[a]-level,ground[b]-level
                if ha<=0:poly.append(a)
                if (ha<0)!=(hb<0):
                    t=ha/(ha-hb);poly.append((a[0]+(b[0]-a[0])*t,a[1]+(b[1]-a[1])*t))
            for j in range(1,len(poly)-1):triangles.append([vertex(*poly[0]),vertex(*poly[j]),vertex(*poly[j+1])])
    meshes.append(dict(id='native_lake_'+str(index),vertices_cm=vertices,uv=uv,triangles=triangles,water_level_m=level))
(L/'water-mesh-buffers-r2.json').write_text(json.dumps({'meshes':meshes},separators=(',',':')))
print([(m['id'],len(m['vertices_cm']),len(m['triangles'])) for m in meshes])
