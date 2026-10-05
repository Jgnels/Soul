"""Bake owned field and multiscale surface masks; no terrain/topology mutation.

Parcel shading runs once here instead of evaluating every parcel per screen
pixel. Native heightfield orientation is retained: image row zero = UE Y min.
"""
from pathlib import Path
import hashlib
import json
import math
import numpy as np
from PIL import Image

DATA = Path('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview/WorldTerrain')
profile = json.loads((DATA / 'presentation.json').read_text())
n = profile['terrain']['resolution']
minimum = profile['terrain']['minimum_xy_cm'][0]
extent = profile['terrain']['extent_xy_cm'][0]
rng = np.random.default_rng(20261005)

def noise(spacing_m):
    size = max(4, round(extent / (spacing_m * 100)))
    source = Image.fromarray(rng.integers(15, 241, (size, size), dtype=np.uint8))
    return np.asarray(source.resize((n, n), Image.Resampling.BICUBIC), dtype=np.float32) / 255

detail = np.stack([noise(24), noise(105), noise(470)], axis=-1)
Image.fromarray(np.uint8(np.clip(detail, 0, 1) * 255)).save(DATA / 'WorldDetail.png')
axis = np.linspace(minimum, minimum + extent, n, dtype=np.float32)
x, y = np.meshgrid(axis, axis)
mask = np.zeros((n, n), dtype=np.float32)
palette = mask.copy()
heading = mask.copy()

def smooth(a, b, value):
    t = np.clip((value-a)/(b-a), 0, 1)
    return t*t*(3-2*t)

for i, (cx, cy, w, h, yaw) in enumerate(profile.get('fields', [])):
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    dx, dy = x-cx, y-cy
    qx, qy = dx*c+dy*s, -dx*s+dy*c
    # Asymmetric weathered boundaries, broad grassy margins, rounded corners.
    warp = (detail[..., 0]-.5)*900 + (detail[..., 1]-.5)*600
    d = np.maximum(np.abs(qx)/(w+warp), np.abs(qy)/(h+warp*.45))
    field = (1-smooth(.80, 1, d)) * (.83 + detail[..., 0]*.17)
    take = field > mask
    mask[take] = field[take]
    palette[take] = (i % 3 + .20 + .45 * detail[..., 1][take]) / 3
    heading[take] = (yaw % 180) / 180

Image.fromarray(np.uint8(np.clip(np.stack([mask, palette, heading], -1), 0, 1)*255)).save(DATA/'WorldCultivation.png')
receipt = {'profile_sha256': hashlib.sha256((DATA/'presentation.json').read_bytes()).hexdigest(),
           'resolution': n, 'orientation': 'row zero UE Y minimum',
           'fields': len(profile.get('fields', [])), 'noise_scales_m': [24, 105, 470],
           'files': {name: hashlib.sha256((DATA/name).read_bytes()).hexdigest()
                     for name in ['WorldDetail.png', 'WorldCultivation.png']}}
(DATA/'surface-bake-receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
print(json.dumps(receipt, indent=2))
