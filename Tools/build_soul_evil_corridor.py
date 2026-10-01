"""Build Soul's presentation profile from the approved read-only terrain.

No donor or derived Unreal package is written. Licensed height data and review
images go to the external preview's SoulIntegration folder, never Git.
Requires numpy and Pillow. Runtime terrain does not determine strategic legality.
"""
import argparse
import hashlib
import heapq
import json
from collections import deque
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]


def build(preview, terrain='evil_waterfront'):
    out = preview / 'SoulIntegration'
    out.mkdir(exist_ok=True)
    source = preview / f'TerrainWork/{terrain}.png'
    raw = np.asarray(Image.open(source)).astype('<u2')
    assert raw.shape == (2041, 2041)
    z = (raw.astype(float) - 32768) * .5  # exact imported cm
    assert (out / 'MesaHeight.r16').read_bytes() == raw.tobytes()
    step = 150000 / 2040
    # Require a dry, low-slope corridor including road shoulders (3 m radius), above the actual sea at Z=0.
    # A centered gradient hides sharp triangle edges. Bound both adjacent slopes.
    dx = np.abs(np.diff(z,axis=1))/step
    dy = np.abs(np.diff(z,axis=0))/step
    dx = np.maximum(np.pad(dx,((0,0),(0,1)),mode='edge'),np.pad(dx,((0,0),(1,0)),mode='edge'))
    dy = np.maximum(np.pad(dy,((0,1),(0,0)),mode='edge'),np.pad(dy,((1,0),(0,0)),mode='edge'))
    slope = np.degrees(np.arctan(np.hypot(dx, dy)))
    legal = (z > 30) & (slope < 22)
    clear = (z > 30).copy()
    for oy, ox in [(y, x) for y in range(-4, 5, 1) for x in range(-4, 5, 1)]:
        clear &= np.roll(z > 30, (oy, ox), (0, 1))
    clear[:9] = clear[-9:] = False
    clear[:, :9] = clear[:, -9:] = False
    valid = clear[::4, ::4]
    sy,sx = np.gradient(z[::4,::4],step*4)
    grades = np.degrees(np.arctan(np.hypot(sx,sy)))
    valid &= grades < 22
    size = valid.shape[0]
    # Preserve canonical IDs; only the optional corridor's presentation moves.
    anchors = dict(human_capital=(.582,.639), crossroads=(.48,.62),
                   river_ford=(.425,.55), orc_watch=(.195,.295),
                   orc_camp=(.08,.208), forest_edge=(.43,.68),
                   north_pass=(.15,.275), old_quarry=(.54,.77),
                   ancient_shrine=(.615,.825))
    def component(u,v):
        yy,xx=np.nonzero(valid)
        i=np.argmin((xx-u*510)**2+(yy-v*510)**2)
        start=(int(yy[i]),int(xx[i]));seen=np.zeros_like(valid);seen[start]=True;queue=deque([start])
        while queue:
            y,x=queue.popleft()
            for dy,dx in [(1,0),(-1,0),(0,1),(0,-1)]:
                t=y+dy,x+dx
                if 0<=t[0]<size and 0<=t[1]<size and valid[t] and not seen[t]:seen[t]=True;queue.append(t)
        return seen
    human=component(.582,.639);evil=component(.195,.312)
    assert not np.any(human & evil), 'Expected disconnected shore components'
    def boundary(mask):
        return np.column_stack(np.nonzero(mask & ~(np.roll(mask,1,0)&np.roll(mask,-1,0)&np.roll(mask,1,1)&np.roll(mask,-1,1))))
    island=component(.345,.513)
    assert not np.any(island & (human|evil)), 'Crossing shoal must be a separate dry landmass'
    def crossing(first,second):
        aa,bb=boundary(first),boundary(second)
        # Keep the approved western crossing; do not move it to another continent's neck.
        aa=aa[(aa[:,1]>130)&(aa[:,1]<230)&(aa[:,0]>220)&(aa[:,0]<300)]
        bb=bb[(bb[:,1]>130)&(bb[:,1]<230)&(bb[:,0]>220)&(bb[:,0]<300)]
        best=(1e30,None,None)
        for chunk in np.array_split(aa,100):
            if len(chunk)==0:continue
            ds=np.sum((chunk[:,None,:]-bb[None,:,:])**2,axis=2)
            i,j=np.unravel_index(ds.argmin(),ds.shape)
            if ds[i,j]<best[0]:best=(ds[i,j],tuple(chunk[i]),tuple(bb[j]))
        assert best[1] is not None
        print('Short crossing UV',np.array(best[1])[::-1]/510,np.array(best[2])[::-1]/510,'span m',np.sqrt(best[0])*1500/510)
        return best[1],best[2]
    crossings=[crossing(human,island),crossing(island,evil)]
    valid=human|evil|island
    yy,xx=np.nonzero(valid)
    nodes = {}
    for name, (u, v) in anchors.items():
        yy,xx=np.nonzero(evil if name in ('orc_watch','orc_camp','north_pass') else human)
        i = np.argmin((xx-u*(size-1))**2 + (yy-v*(size-1))**2)
        assert np.hypot(xx[i]/510-u,yy[i]/510-v)<.06, (name,'no nearby dry anchor')
        nodes[name] = (int(yy[i]), int(xx[i]))

    def xy(node):
        y, x = node
        return [round(-75000 + x*4*step, 5), round(-75000 + y*4*step, 5)]

    def land_route(start, end):
        queue = [(0., start)]
        cost, prev = {start: 0.}, {}
        while queue:
            _, u = heapq.heappop(queue)
            if u == end:
                path = [u]
                while path[-1] != start:
                    path.append(prev[path[-1]])
                return path[::-1]
            for oy, ox in [(0,1),(0,-1),(1,0),(-1,0),(1,1),(-1,-1),(1,-1),(-1,1)]:
                v = (u[0]+oy, u[1]+ox)
                if not (0 <= v[0] < size and 0 <= v[1] < size) or not valid[v]:
                    continue
                if ox and oy and (not valid[u[0], v[1]] or not valid[v[0], u[1]]):
                    continue
                nc = cost[u] + np.hypot(oy, ox) * (1 + grades[v]/12)
                if nc < cost.get(v, float('inf')):
                    cost[v], prev[v] = nc, u
                    heapq.heappush(queue, (nc + np.hypot(v[0]-end[0], v[1]-end[1]), v))
        raise RuntimeError(f'No traversable corridor {start} -> {end}')

    # Roads traverse the exposed shoal and its dry neck; only actual water gets arches.
    def line(a,b):
        return [tuple(n) for n in np.linspace(a,b,int(np.linalg.norm(np.array(b)-a)*3)+1)]
    sa,ia=crossings[0];ib,sb=crossings[1]
    middle=line(sa,ia)[:-1]+land_route(ia,ib)[:-1]+line(ib,sb)
    def route(start,end):
        if human[start]==human[end]:return land_route(start,end)
        path=land_route(start,sa)[:-1]+middle+land_route(sb,end)[1:] if human[start] else land_route(start,sb)[:-1]+middle[::-1]+land_route(sa,end)[1:]
        return path

    original = json.loads((ROOT/'Data/CampaignTerrainV2/presentation.json').read_text())
    profile = dict(schema=1, map=f'/Game/SoulCampaignMountain/L_{terrain}',
                   resolution=2041, extent_cm=150000, minimum_xy_cm=-75000,
                   height_unit_cm=.5, scale=10, region_scale=2.5,
                   height_sha256=hashlib.sha256(raw.tobytes()).hexdigest(),
                   source_png_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                   regions={}, routes=[], fields=[], bridges=[], waterlines=[])
    for name, node in nodes.items():
        profile['regions'][name] = xy(node) + [float(z[node[0]*4, node[1]*4])]
    for edge in original['routes']:
        a, b = edge['a'], edge['b']
        path = route(nodes[a], nodes[b])
        # Keep every 2.94 m sample; avoid smoothing across steep or wet corners.
        profile['routes'].append(dict(a=a, b=b, points=[xy(n) for n in path]))
    for shore_a,shore_b in crossings:
        ca,cb=np.array(xy(shore_a)),np.array(xy(shore_b));center=(ca+cb)/2;delta=cb-ca
        # Low dry necks may fail the road shoulder clearance, but are not water.
        samples=np.linspace(ca,cb,2001);uv=(samples+75000)/step;ij=uv.astype(int);fx,fy=(uv-ij).T;x,y=ij.T
        a0,b0,c0,d0=z[y,x],z[y,x+1],z[y+1,x],z[y+1,x+1]
        heights=np.where(fx>=fy,a0+(b0-a0)*fx+(d0-b0)*fy,a0+(d0-c0)*fx+(c0-a0)*fy)
        wet=np.flatnonzero(heights<=0)
        if not len(wet):continue
        direction=delta/np.linalg.norm(delta)
        ca=samples[wet[0]]-direction*250;cb=samples[wet[-1]]+direction*250
        center=(ca+cb)/2;delta=cb-ca
        deck=float(max(z[shore_a[0]*4,shore_a[1]*4],z[shore_b[0]*4,shore_b[1]*4])+100)
        for a,b in [('river_ford','orc_watch'),('forest_edge','orc_watch'),('forest_edge','north_pass')]:
            profile['bridges'].append(dict(a=a,b=b,center=[*center,deck],yaw=float(np.degrees(np.arctan2(delta[1],delta[0]))),half_span=float(np.linalg.norm(delta)/2),ford=False))
    profile['display_names']={'human_capital':'Crownstead','crossroads':'Western Road','river_ford':'Bridgeward','orc_watch':'Ashport','orc_camp':'Cinder Crown','north_pass':'Black Gate','forest_edge':'Westwood','old_quarry':'Old Quarry','ancient_shrine':'Ancient Shrine'}
    target = ROOT/'Data/CampaignEvilCorridor/presentation.json'
    target.parent.mkdir(exist_ok=True)
    target.write_text(json.dumps(profile, indent=2)+'\n')
    image = np.repeat(np.clip(z[:,:,None]/100*1.5+65, 0, 255), 3, axis=2).astype('uint8')
    image[z <= 0] = [25,60,90]
    im = Image.fromarray(image).resize((1020,1020))
    draw = ImageDraw.Draw(im)
    for edge in profile['routes']:
        draw.line([((x+75000)/150000*1020,(y+75000)/150000*1020) for x,y in edge['points']], fill=(240,190,50), width=3)
    for name, (y,x) in nodes.items():
        draw.ellipse((x*2-5,y*2-5,x*2+5,y*2+5), fill='red')
        draw.text((x*2+6,y*2), name, fill='white')
    im.save(out/'evil-corridor-routes.png')
    print(json.dumps({'profile':str(target),'external_height':str(out/'MesaHeight.r16'),'regions':profile['regions'],'routes':len(profile['routes'])},indent=2))


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--preview',type=Path,default=Path('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview'))
    p.add_argument('--terrain', choices=('evil_waterfront','mesa_coast_v2'), default='evil_waterfront')
    args = p.parse_args()
    build(args.preview, args.terrain)
