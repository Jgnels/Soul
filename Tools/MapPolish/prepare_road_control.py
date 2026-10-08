"""Owned-surface placement mask only; retain all non-road terrain channels."""
import json,math,hashlib,argparse
import numpy as np
from PIL import Image,ImageDraw,ImageFilter
from route_surface import ROOT,OUT,dense,SITES
parser=argparse.ArgumentParser();parser.add_argument('--input',default='routes-arrivals-r1.json');parser.add_argument('--revision',default='r1');args=parser.parse_args()
data=json.loads((OUT/'Local'/args.input).read_text());anchors={a['id']:a for a in data['anchors']}
factor=2;size=2041*factor;road=Image.new('L',(size,size));draw=ImageDraw.Draw(road);roles=[]
def line(points,width):
    p=dense(points,.7);run=[]
    for q in p:
        # Human internal streets are reviewed against the actual native mesh.
        # Other capitals retain a bounded footprint until their gates are known.
        hidden=any(a['id']!='human_capital' and a['kind']=='capital' and abs(q[0]-a['xy_m'][0])<a['footprint_m'][0]/2+1 and abs(q[1]-a['xy_m'][1])<a['footprint_m'][1]/2+1 for a in data['anchors'])
        if hidden:
            if len(run)>1:draw.line(run,fill=255,width=max(1,round(width/3500*2040*factor)),joint='curve')
            run=[]
        else:run.append(tuple(q/3500*2040*factor))
    if len(run)>1:draw.line(run,fill=255,width=max(1,round(width/3500*2040*factor)),joint='curve')
for r in data['routes']:
    regions={anchors[r[k]]['macro_region'] for k in ['a','b']}
    if all(n.startswith('nature_') for n in [r['a'],r['b']]):role,width='woodland path',2.4
    elif all(n.startswith('dwarf_') for n in [r['a'],r['b']]):role,width='engineered mountain track',2.8
    elif all(n.startswith('orc_') for n in [r['a'],r['b']]):role,width='badlands military road',3.2
    elif any(n in ['human_capital','crossroads','river_ford','southern_crossing','north_pass'] for n in [r['a'],r['b']]):role,width='main campaign road',3.8
    else:role,width='secondary road',3.0
    r['road_hierarchy']=role;r['road_width_m']=width
    for s in r['segments']:
        if s['type']=='road':line(s['points_m'],width)
    roles.append(dict(a=r['a'],b=r['b'],hierarchy=role,width_m=width))
for s in data.get('settlement_arrival_spurs',[]):line(s['points_m'],s['width_m'])
road=road.filter(ImageFilter.GaussianBlur(.5*factor)).resize((2041,2041),Image.Resampling.LANCZOS)
old=Image.open(ROOT/'Data/CampaignCompositionLocal/Composition_Control_r5.png').convert('RGBA');channels=list(old.split());channels[0]=road
out=OUT/('Local/Composition_Polish_Control_'+args.revision+'.png');Image.merge('RGBA',channels).save(out)
(OUT/('Local/routes-presentation-'+args.revision+'.json')).write_text(json.dumps(data,indent=2))
(OUT/('road-hierarchy-'+args.revision+'.json')).write_text(json.dumps({'routes':roles,'old_width_m':3500/2040*3,'new_width_range_m':[2.4,3.8],'heightfield_changed':False,'nonroad_mask_channels_unchanged':True,'control_sha256':hashlib.sha256(out.read_bytes()).hexdigest()},indent=2))
print('POLISH_ROAD_CONTROL',out)
