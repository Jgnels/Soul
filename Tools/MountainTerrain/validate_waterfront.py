from pathlib import Path
import numpy as np,json,heapq
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parents[1];p=ROOT/'TerrainWork'
b=np.load(p/'mesa_coast_v2.npy');z=np.load(p/'evil_waterfront.npy');N=z.shape[0];step=1500/(N-1)
meta=json.loads((p/'evil-waterfront.json').read_text());mask=Image.new('L',(N,N));ImageDraw.Draw(mask).polygon([(round(x*(N-1)),round(y*(N-1))) for x,y in meta['polygon_uv']],fill=255)
inside=np.asarray(mask)>0
assert np.array_equal(b[~inside],z[~inside])
assert np.array_equal(b[b<=0],z[b<=0])
encoded=np.asarray(Image.open(p/'evil_waterfront.png')).astype(float);assert np.max(abs((encoded-32768)*.005-z))<.00251
q=z[::4,::4];dy,dx=np.gradient(q,step*4);slope=np.degrees(np.arctan(np.hypot(dx,dy)));valid=(q>1.5)&(slope<22)
ny,nx=q.shape
def nearest(uv):
    y,x=np.nonzero(valid);i=np.argmin((x-uv[0]*(nx-1))**2+(y-uv[1]*(ny-1))**2);return int(y[i]),int(x[i])
def route(a,b):
    start,end=nearest(a),nearest(b);todo=[(0,start)];cost={start:0};prev={}
    while todo:
        c,u=heapq.heappop(todo)
        if c!=cost[u]:continue
        if u==end:
            path=[u]
            while path[-1]!=start:path.append(prev[path[-1]])
            return path[::-1]
        for oy,ox in [(0,1),(0,-1),(1,0),(-1,0),(1,1),(-1,-1),(1,-1),(-1,1)]:
            v=(u[0]+oy,u[1]+ox)
            if not(0<=v[0]<ny and 0<=v[1]<nx) or not valid[v]:continue
            if ox and oy and (not valid[u[0],v[1]] or not valid[v[0],u[1]]):continue
            nc=c+np.hypot(ox,oy)*(1+slope[v]/8)
            if nc<cost.get(v,1e20):cost[v]=nc;prev[v]=u;heapq.heappush(todo,(nc,v))
    return []
pairs={'mesa_to_coastal_valley':((.08,.21),(.19,.312)),'mesa_to_south':((.08,.21),(.15,.39)),'plains_to_north_coast':((.58,.65),(.62,.40)),'plains_to_south_foothills':((.58,.65),(.64,.88)),'plains_to_east_foothills':((.58,.65),(.82,.66)),'north_lake_west_to_east':((.52,.11),(.90,.12))}
report={'outside_edit_unchanged':True,'shoreline_unchanged':True,'quantization_error_m':float(np.max(abs((encoded-32768)*.005-z))),'slope_limit_degrees':22,'routes':{}}
im=Image.open(p/'evil_waterfront.png').convert('RGB');draw=ImageDraw.Draw(im)
for name,(a,b) in pairs.items():
    path=route(a,b);r={'pass':bool(path),'points':len(path)}
    if path:
        r['max_slope_degrees']=float(max(slope[v] for v in path));r['length_m']=float(sum(np.hypot(v[0]-u[0],v[1]-u[1])*step*4 for u,v in zip(path,path[1:])))
        draw.line([(x*4,y*4) for y,x in path],fill=(245,190,70),width=5)
    report['routes'][name]=r
(p/'waterfront-validation.json').write_text(json.dumps(report,indent=2));im.save(p/'waterfront-travel-study.png');print(json.dumps(report,indent=2))
assert all(r['pass'] for r in report['routes'].values()),'Terrain route missing'
