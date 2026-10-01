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
    (out / 'MesaHeight.r16').write_bytes(raw.tobytes())
    step = 150000 / 2040
    # Require a dry, low-slope corridor including road shoulders (6 m radius).
    # A centered gradient hides sharp triangle edges. Bound both adjacent slopes.
    dx = np.abs(np.diff(z,axis=1))/step
    dy = np.abs(np.diff(z,axis=0))/step
    dx = np.maximum(np.pad(dx,((0,0),(0,1)),mode='edge'),np.pad(dx,((0,0),(1,0)),mode='edge'))
    dy = np.maximum(np.pad(dy,((0,1),(0,0)),mode='edge'),np.pad(dy,((1,0),(0,0)),mode='edge'))
    slope = np.degrees(np.arctan(np.hypot(dx, dy)))
    legal = (z > 180) & (slope < 22)
    clear = legal.copy()
    for oy, ox in [(y, x) for y in range(-8, 9, 2) for x in range(-8, 9, 2)]:
        clear &= np.roll(legal, (oy, ox), (0, 1))
    clear[:9] = clear[-9:] = False
    clear[:, :9] = clear[:, -9:] = False
    valid = clear[::4, ::4]
    grades = slope[::4, ::4]
    size = valid.shape[0]
    # Human peninsula -> dwarf foothills, preserving all nine canonical IDs.
    anchors = dict(human_capital=(.55, .65), crossroads=(.62, .63),
                   river_ford=(.68, .54), orc_watch=(.77, .54),
                   orc_camp=(.84, .67), forest_edge=(.68, .70),
                   north_pass=(.77, .75), old_quarry=(.54, .77),
                   ancient_shrine=(.62, .84))
    yy, xx = np.nonzero(valid)
    i = np.argmin((xx-.55*(size-1))**2 + (yy-.65*(size-1))**2)
    start = (int(yy[i]), int(xx[i]))
    connected = np.zeros_like(valid)
    connected[start] = True
    queue = deque([start])
    while queue:
        y, x = queue.popleft()
        for oy, ox in [(0,1),(0,-1),(1,0),(-1,0)]:
            v = (y+oy, x+ox)
            if 0 <= v[0] < size and 0 <= v[1] < size and valid[v] and not connected[v]:
                connected[v] = True
                queue.append(v)
    valid &= connected
    yy, xx = np.nonzero(valid)
    nodes = {}
    for name, (u, v) in anchors.items():
        i = np.argmin((xx-u*(size-1))**2 + (yy-v*(size-1))**2)
        nodes[name] = (int(yy[i]), int(xx[i]))

    def xy(node):
        y, x = node
        return [round(-75000 + x*4*step, 5), round(-75000 + y*4*step, 5)]

    def route(start, end):
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
    target = ROOT/'Data/CampaignMesa/presentation.json'
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
    im.save(out/'campaign-routes.png')
    print(json.dumps({'profile':str(target),'external_height':str(out/'MesaHeight.r16'),'regions':profile['regions'],'routes':len(profile['routes'])},indent=2))


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--preview',type=Path,default=Path('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview'))
    p.add_argument('--terrain', choices=('evil_waterfront','mesa_coast_v2'), default='evil_waterfront')
    args = p.parse_args()
    build(args.preview, args.terrain)
