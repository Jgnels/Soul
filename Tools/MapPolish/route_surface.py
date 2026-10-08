"""Read-only, triangle-exact frozen Landscape / provisional crossing sampler.

Bridge planes are an analytical approximation, not native mesh qualification.
All units below are metres in the composition's [0,3500] XY domain.
"""
from pathlib import Path
import math
import json
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
BEFORE = ROOT / 'Evidence/ProductionWorldComposition-20261007'
OUT = ROOT / 'Evidence/MapPolish-20261007'
# The imported uint16 field is authoritative, including its 7.8125 mm
# quantization; the earlier floating study is not the native collision surface.
Z = (np.asarray(Image.open(ROOT / 'Data/CampaignCompositionLocal/Composition_3500_r2.png'),dtype=float)-32768)/128
WATER = np.maximum(0, np.maximum(np.load(BEFORE / 'Local/river-water.npy'), np.load(BEFORE / 'Local/lake-water.npy')))
SITES = json.loads((BEFORE / 'selected-crossing-sites-r3.json').read_text())['crossings']
DECKS = {d['id']: d for d in json.loads((BEFORE / 'bridge-placement-r2.json').read_text())['bridges']}

def sample(points, field=Z):
    p = np.clip(np.asarray(points) / 3500 * 2040, 0, 2039.999)
    i = p.astype(int); f = p-i; x, y = i[...,0], i[...,1]
    a,b,c,d = field[y,x],field[y,x+1],field[y+1,x],field[y+1,x+1]
    fx,fy=f[...,0],f[...,1]
    return np.where(fx>=fy,a+(b-a)*fx+(d-b)*fy,a+(d-c)*fx+(c-a)*fy)

def surface(points):
    p=np.asarray(points); h=sample(p); valid=h>sample(p,WATER)+.04
    for c in SITES:
        yaw=math.radians(c['yaw_deg']); axis=np.array([math.cos(yaw),math.sin(yaw)])
        q=p-c['center_xy_m']; along=q@axis; across=abs(q@np.array([-axis[1],axis[0]]))
        if c['type']=='ford':
            valid |= (abs(along)<c['span_m']/2+3)&(across<3)&(h>sample(p,WATER)-.3)
        else:
            deck=(abs(along)<=c['span_m']/2)&(across<=2.64)
            d=DECKS[c['gate_id']]; dh=d['deck_z_m']+along*d['deck_gradient']
            h=np.where(deck,np.maximum(h,dh),h); valid|=deck
    return h,valid

def dense(points, spacing=.5):
    result=[]
    for a,b in zip(points[:-1],points[1:]):
        a,b=np.asarray(a)[:2],np.asarray(b)[:2]; dist=np.linalg.norm(b-a)
        if dist>1e-8:
            result.extend(a+(b-a)*np.linspace(0,1,max(1,math.ceil(dist/spacing)),endpoint=False)[:,None])
    return np.asarray(result+[np.asarray(points[-1])[:2]])

def assess(points, spacing=.25):
    p=dense(points,spacing); h,valid=surface(p)
    ds=np.linalg.norm(np.diff(p,axis=0),axis=1)
    grade=np.degrees(np.arctan(abs(np.diff(h))/np.maximum(ds,1e-9)))
    return dict(length_m=float(ds.sum()),max_grade_deg=float(grade.max(initial=0)),
                invalid_samples=int((~valid).sum()),over22_1_samples=int((grade>22.1).sum()))

def good(points, limit=22.0):
    a=assess(points,.25)
    return a['invalid_samples']==0 and a['max_grade_deg']<=limit
