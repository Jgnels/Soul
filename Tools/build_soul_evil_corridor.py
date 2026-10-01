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
    # Require a dry, low-slope corridor including road shoulders (6 m radius).
    # A centered gradient hides sharp triangle edges. Bound both adjacent slopes.
    dx = np.abs(np.diff(z,axis=1))/step
    dy = np.abs(np.diff(z,axis=0))/step
    dx = np.maximum(np.pad(dx,((0,0),(0,1)),mode='edge'),np.pad(dx,((0,0),(1,0)),mode='edge'))
    dy = np.maximum(np.pad(dy,((0,1),(0,0)),mode='edge'),np.pad(dy,((1,0),(0,0)),mode='edge'))
    slope = np.degrees(np.arctan(np.hypot(dx, dy)))
    legal = (z > 180) & (slope < 22)
    clear = (z > 180).copy()
    for oy, ox in [(y, x) for y in range(-4, 5, 1) for x in range(-4, 5, 1)]:
        clear &= np.roll(z > 180, (oy, ox), (0, 1))
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
    aa,bb=boundary(human),boundary(evil);best=(1e30,None,None)
    for chunk in np.array_split(aa,100):
        if len(chunk)==0:continue
        ds=np.sum((chunk[:,None,:]-bb[None,:,:])**2,axis=2)
        i,j=np.unravel_index(ds.argmin(),ds.shape)
        if ds[i,j]<best[0]:best=(ds[i,j],chunk[i],bb[j])
    shore_a,shore_b=best[1],best[2]
    print('Bridge endpoints UV',shore_a[::-1]/510,shore_b[::-1]/510,'span m',np.sqrt(best[0])*1500/510)
    # Connect dry components across an explicit bridge only, not invisible water paths.
    bridge_nodes=np.rint(np.linspace(shore_a,shore_b,int(np.sqrt(best[0])*3)+1)).astype(int)
    valid=human|evil
    for y,x in bridge_nodes:valid[max(0,y-1):y+2,max(0,x-1):x+2]=True;grades[y,x]=0
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

    # Bridge centerline is explicit; land routes never search through water.
    valid=human|evil
    sa,sb=tuple(shore_a),tuple(shore_b)
    def route(start,end):
        if human[start]==human[end]:return land_route(start,end)
        a,b=(sa,sb) if human[start] else (sb,sa)
        middle=[tuple(n) for n in np.linspace(a,b,int(np.linalg.norm(shore_b-shore_a)*3)+1)]
        return land_route(start,a)[:-1]+middle+land_route(b,end)[1:]

    original = json.loads((ROOT/'Data/CampaignTerrainV2/presentation.json').read_text())
    profile = dict(schema=1, map=f'/Game/SoulCampaignMountain/L_{terrain}',
                   resolution=2041, extent_cm=150000, minimum_xy_cm=-75000,
                   height_unit_cm=.5, scale=10, region_scale=5,
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
    ca,cb=np.array(xy(shore_a)),np.array(xy(shore_b));center=(ca+cb)/2;delta=cb-ca
    deck=float(max(z[shore_a[0]*4,shore_a[1]*4],z[shore_b[0]*4,shore_b[1]*4])+100)
    for a,b in [('river_ford','orc_watch'),('forest_edge','orc_watch'),('forest_edge','north_pass')]:
        profile['bridges'].append(dict(a=a,b=b,center=[*center,deck],yaw=float(np.degrees(np.arctan2(delta[1],delta[0]))),half_span=float(np.linalg.norm(delta)/2+150),ford=False))
    profile['display_names']={'human_capital':'Crownstead','crossroads':'Western Road','river_ford':'Bridgeward','orc_watch':'Ashport','orc_camp':'Cinder Crown','north_pass':'Black Gate','forest_edge':'Westwood','old_quarry':'Old Quarry','ancient_shrine':'Ancient Shrine'}
    target = ROOT/'Data/CampaignEvilCorridor/presentation.json'
    target.parent.mkdir(exist_ok=True)
    target.write_text(json.dumps(profile, indent=2)+'\n')
    image = np.repeat(np.clip(z[:,:,None]/100*1.5+65, 0, 255), 3, axis=2).astype('uint8')
    image[z <= 180] = [25,60,90]
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
