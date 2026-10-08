"""Bounded route-only repair with exact endpoint connections; never edits terrain."""
import json, heapq, math
import numpy as np
from route_surface import BEFORE, OUT, surface, dense, assess, good

def search(start, end, spacing=.5, margin=22, grade_limit=21.95):
    origin=np.minimum(start,end)-margin
    size=np.ceil((np.abs(end-start)+2*margin)/spacing).astype(int)+1
    assert size.max()<520, 'Bounded local repair only'
    yy,xx=np.mgrid[:size[1],:size[0]]; pts=np.stack([xx,yy],-1)*spacing+origin
    h,valid=surface(pts)
    moves=[(1,0),(-1,0),(0,1),(0,-1),(1,1),(-1,1),(1,-1),(-1,-1)]
    cost=[]
    for dx,dy in moves:
        delta=np.array([dx,dy])*spacing; prev=h; ok=valid.copy(); maximum=np.zeros(h.shape)
        for t in [.25,.5,.75,1]:
            hh,v=surface(pts+delta*t); g=abs(hh-prev)/(.25*np.linalg.norm(delta)); maximum=np.maximum(maximum,g); ok&=v; prev=hh
        cost.append(np.where(ok&(maximum<math.tan(math.radians(grade_limit))),np.linalg.norm(delta)*(1+2*maximum**2),np.inf))
    def links(p):
        cell=np.rint((p-origin)/spacing).astype(int); result={}
        for dx in range(-5,6):
            for dy in range(-5,6):
                q=cell+[dx,dy]
                if np.any(q<0) or np.any(q>=size):continue
                n=tuple(q); pos=pts[q[1],q[0]]
                if good([p,pos],grade_limit):result[n]=float(np.linalg.norm(p-pos))
        return result
    starts=links(start); ends=links(end); queue=[]; parents={}; dist=dict(starts); closed=set()
    for n,d in starts.items():heapq.heappush(queue,(d+np.linalg.norm(pts[n[1],n[0]]-end),n))
    while queue:
        _,p=heapq.heappop(queue)
        if p in closed:continue
        if p in ends:
            path=[p]
            while p in parents:p=parents[p];path.append(p)
            path=np.array([start]+[pts[n[1],n[0]] for n in reversed(path)]+[end])
            # Remove grid stair steps only where the exact terrain accepts the chord.
            result=[path[0]]; i=0
            while i<len(path)-1:
                j=min(i+24,len(path)-1)
                while j>i+1 and not good([path[i],path[j]],grade_limit):j-=1
                result.append(path[j]);i=j
            return np.array(result) if good(result,22) else None
        closed.add(p);x,y=p
        for k,(dx,dy) in enumerate(moves):
            n=(x+dx,y+dy)
            if not (0<=n[0]<size[0] and 0<=n[1]<size[1]) or n in closed:continue
            new=dist[p]+cost[k][y,x]
            if new<dist.get(n,np.inf):
                dist[n]=new;parents[n]=p;heapq.heappush(queue,(new+np.linalg.norm(pts[n[1],n[0]]-end),n))
    return None

if __name__=='__main__':
    data=json.loads((BEFORE/'route-local-repair-study-r7.json').read_text())
    r=next(r for r in data['routes'] if r['a']=='crossroads' and r['b']=='river_ford')
    seg=r['segments'][-1]; p=dense(seg['points_m'],.5); before=assess(p)
    receipt={'route':[r['a'],r['b']],'before':before,'attempts':[],'terrain_edited':False}
    for distance in [25,40,60]:
        lo=max(0,len(p)-1-int(distance/.5)); repair=search(p[lo],p[-1])
        receipt['attempts'].append({'last_m':distance,'found':repair is not None})
        if repair is not None:
            candidate=np.concatenate([p[:lo],repair]); result=assess(candidate)
            if result['max_grade_deg']<=22.1 and not result['invalid_samples']:
                seg['points_m']=candidate.tolist();receipt['after']=result;receipt['accepted']=True;break
    else:receipt['accepted']=False
    (OUT/'ford-route-repair.json').write_text(json.dumps(receipt,indent=2))
    (OUT/'Local/routes-ford-r1.json').write_text(json.dumps(data,indent=2))
    print(json.dumps(receipt,indent=2))
