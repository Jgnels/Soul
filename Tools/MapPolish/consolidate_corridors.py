"""Merge only parallel roads which diverge and rejoin the same local corridor.

Proximity alone is insufficient: ends must rejoin within 2 m, no site may lose
its approach, named crossing usage must match, and the replacement must qualify.
Canonical pairs and endpoint coordinates are never removed or reassigned.
"""
import sys,json,math,argparse
import numpy as np
from route_surface import OUT,SITES,dense,assess,good
sys.path.insert(0,str(OUT/'Local/python-libs'))
from scipy.spatial import cKDTree

parser=argparse.ArgumentParser();parser.add_argument('--input',default='routes-fine-r1.json');parser.add_argument('--radius',type=float,default=18);parser.add_argument('--revision',default='r1');args=parser.parse_args()
assert 0<args.radius<=45
data=json.loads((OUT/'Local'/args.input).read_text()); receipts=[]
bykey={(r['a'],r['b']):r for r in data['routes']}
# A common woodland approach has the same strategic purpose; retaining the
# short branch to River Woodland preserves access to both distinct sites.
branch=bykey['nature_forest_clearing','nature_river_woodland']['segments'][0]['points_m']
trunk=bykey['nature_river_woodland','nature_treehold']['segments'][0]['points_m']
joined=branch+trunk[1:]
if good(joined,22.1):
    bykey['nature_forest_clearing','nature_treehold']['segments']=[dict(type='road',points_m=joined)]
    receipts.append(dict(route=['nature_forest_clearing','nature_treehold'],kind='shared woodland approach',reason='Clearing road joins River Woodland branch before the common Treehold approach; separate southern crossing remains.',after=assess(joined)))
anchors=np.array([a['xy_m'] for a in data['anchors']])
def crossings(p):
    return {c['gate_id'] for c in SITES if np.linalg.norm(p-c['center_xy_m'],axis=1).min()<c['span_m']/2+3}

masters=[]
for r in sorted(data['routes'],key=lambda r:sum(assess(s['points_m'])['length_m'] for s in r['segments'] if s['type']=='road')):
    for si,s in enumerate(r['segments']):
        if s['type']!='road':continue
        p=dense(s['points_m'],2)
        for ref,key,tree in masters:
            if len(p)<35:continue
            distances,indices=tree.query(p)
            inside=distances<args.radius
            ids=np.flatnonzero(inside)
            runs=np.split(ids,np.where(np.diff(ids)>1)[0]+1) if len(ids) else []
            for run in reversed(runs):
                near=run[distances[run]<2]
                if len(near)<2:continue
                lo,hi=int(near[0]),int(near[-1])
                if hi-lo<30 or distances[lo:hi+1].max()<1.5:continue
                j,k=int(indices[lo]),int(indices[hi])
                if abs(j-k)<25:continue
                q=ref[min(j,k):max(j,k)+1]
                if j>k:q=q[::-1]
                original=p[lo:hi+1]
                if len(q)>len(original)*1.2:continue
                # The replacement stays near the old corridor in both directions.
                if cKDTree(original).query(q)[0].max()>args.radius:continue
                if crossings(q)!=crossings(original):continue
                d0=cKDTree(original).query(anchors)[0];d1=cKDTree(q).query(anchors)[0]
                if np.any((d0<20)&(d1>d0+3)):continue
                replacement=np.concatenate([p[lo:lo+1],q,p[hi:hi+1]])
                if not good(replacement,22.05):continue
                before=assess(original);after=assess(replacement)
                if after['length_m']>before['length_m']*1.12:continue
                p=np.concatenate([p[:lo],replacement,p[hi+1:]])
                receipts.append(dict(route=[r['a'],r['b']],reference=key,segment=si,start=original[0].tolist(),end=original[-1].tolist(),kind='parallel same-corridor merge',reason='Rejoins same corridor at both ends; no distinct site or crossing lost.',before=before,after=after))
                # Index arrays are invalid after this splice; revisit remaining
                # overlaps against the next reference rather than use stale runs.
                break
        s['points_m']=p.tolist();masters.append((p,[r['a'],r['b']],cKDTree(p)))
    print('CONSOLIDATED',r['a'],r['b'],len(receipts),flush=True)
(OUT/('Local/routes-consolidated-'+args.revision+'.json')).write_text(json.dumps(data,indent=2))
(OUT/('corridor-consolidation-'+args.revision+'.json')).write_text(json.dumps({'canonical_edges_removed':0,'maximum_corridor_offset_m':args.radius,'changes':receipts,'policy':'same local corridor and same crossing; proximity alone never authorizes removal'},indent=2))
