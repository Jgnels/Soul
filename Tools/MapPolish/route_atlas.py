"""Review all 51 legal connections over the frozen owned heightfield.

These annotated analysis plates are evidence only, never player-facing art.
"""
import json,math,html,argparse
import numpy as np
from PIL import Image,ImageDraw,ImageFont
from route_surface import ROOT,OUT,BEFORE,Z,SITES,dense,assess
parser=argparse.ArgumentParser();parser.add_argument('--input',default='routes-presentation-r4.json');args=parser.parse_args()
d=json.loads((OUT/'Local'/args.input).read_text());old=json.loads((BEFORE/'route-local-repair-study-r7.json').read_text());world=json.loads((ROOT/'Data/soul_world_overmap_v1_20260922.json').read_text())
assert {a['id'] for a in d['anchors']}=={n['id'] for n in world['nodes']}
assert {tuple(sorted([r['a'],r['b']])) for r in d['routes']}=={tuple(sorted([r['a'],r['b']])) for r in world['edges']}
assert len(d['routes'])==51
assert {a['id']:a['xy_m'] for a in d['anchors']}=={a['id']:a['xy_m'] for a in old['anchors']}
anchors={a['id']:a for a in d['anchors']};out=OUT/'Local/route-atlas';out.mkdir(exist_ok=True)
font=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',20);small=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',14)
gy,gx=np.gradient(Z,3500/2040);light=np.clip((.8-gx*.45-gy*.5)/np.sqrt(1+gx*gx+gy*gy),.15,1)
rgb=np.stack([light*95+25,light*110+30,light*75+24],-1);rgb[Z<0]=[36,71,84]
base=Image.fromarray(rgb.astype(np.uint8)).resize((1400,1400),Image.Resampling.LANCZOS);b=ImageDraw.Draw(base)
for r in d['routes']:
 for s in r['segments']:
  if s['type']=='road':b.line([(x*.4,y*.4) for x,y in s['points_m']],fill='#949481',width=1)
for a in d['anchors']:
 x,y=np.array(a['xy_m'])*.4;b.ellipse((x-2,y-2,x+2,y+2),fill='#d3dbca')
metrics=[];cards=[]
for index,r in enumerate(d['routes']):
 before=old['routes'][index];assert [r['a'],r['b']]==[before['a'],before['b']]
 points=np.concatenate([np.array(s['points_m']) for s in r['segments']]);low=np.maximum(0,points.min(0)-90);high=np.minimum(3500,points.max(0)+90);extent=np.maximum(high-low,400);center=(high+low)/2;extent=np.array([max(extent[0],extent[1]*1.25),max(extent[1],extent[0]/1.25)])
 low=center-extent/2;high=center+extent/2
 crop=base.crop(tuple((np.r_[low,high]*.4).astype(int))).resize((1000,800),Image.Resampling.LANCZOS);draw=ImageDraw.Draw(crop)
 def xy(p):return tuple((np.asarray(p)-low)/extent*[1000,800])
 crossing=[]
 for s in r['segments']:
  pp=dense(s['points_m'],2)
  color='#e8ba70' if s['type']=='road' else '#b1c8f3'
  if s['type']=='road':draw.line([xy(p) for p in pp],fill=color,width=3)
  else:
   crossing.append(s['id']+' (ferry)')
   for i in range(0,len(pp)-1,8):draw.line([xy(p) for p in pp[i:i+5]],fill=color,width=3)
  for c in SITES:
   if c['gate_id']=='woodland_bridge':continue
   if np.linalg.norm(pp-c['center_xy_m'],axis=1).min()<2:
    kind={'river_ford':'ford','human_north_bridge':'timber','orc_broken_bridge':'ruined stone / timber repair'}.get(c['gate_id'],'stone')
    label=c['gate_id']+' ('+kind+')'
    if label not in crossing:crossing.append(label)
 for key in [r['a'],r['b']]:
  px,py=xy(anchors[key]['xy_m']);draw.ellipse((px-5,py-5,px+5,py+5),fill='#f7ebe0');draw.text((max(4,min(780,px+8)),max(30,min(760,py))),key,font=small,fill='white',stroke_width=2,stroke_fill='#17252c')
 draw.rectangle((0,0,1000,29),fill='#142127');draw.text((9,2),f'{index+1:02d}  {r["a"]} → {r["b"]}',font=font,fill='white')
 path=out/f'{index+1:02d}.png';crop.save(path)
 ms=[assess(s['points_m']) for s in r['segments'] if s['type']=='road'];quarter=max(m['max_grade_deg'] for m in ms)
 rowsum=dict(index=index+1,a=r['a'],b=r['b'],length_m=sum(math.dist(p,q) for s in r['segments'] for p,q in zip(s['points_m'][:-1],s['points_m'][1:])),max_grade_025m_deg=quarter,max_grade_1m_deg=max(assess(s['points_m'],1)['max_grade_deg'] for s in r['segments'] if s['type']=='road'),invalid_surface_samples=sum(m['invalid_samples'] for m in ms),hierarchy=r['road_hierarchy'],width_m=r['road_width_m'],crossings=crossing,legal_pair_preserved=True,plate=str(path.relative_to(OUT)).replace('\\','/'))
 metrics.append(rowsum);cards.append(f'<section><h2>{index+1:02d} {html.escape(r["a"])} → {html.escape(r["b"])}</h2><p>{html.escape(r["road_hierarchy"])} · {quarter:.2f}° analytical maximum · {html.escape(", ".join(crossing) or "land route")}</p><img loading="lazy" src="{rowsum["plate"]}"></section>')
(OUT/'route-inventory.json').write_text(json.dumps({'canonical_regions':36,'canonical_connections':51,'all_endpoints_unchanged':True,'analytical_passes':sum(m['max_grade_025m_deg']<=22.1 and not m['invalid_surface_samples'] for m in metrics),'native_qualification_separate':True,'routes':metrics},indent=2))
(OUT/'route-atlas.html').write_text('<!doctype html><meta charset="utf-8"><title>Soul 51-route audit</title><style>body{background:#172127;color:#e5e5dc;font:16px system-ui;max-width:1100px;margin:auto;padding:24px}img{width:100%}section{margin-bottom:36px}a{color:#9ed1e7}</style><h1>51 legal connections · frozen 3.5 km geography</h1><p>Analytical evidence plates, not Unreal screenshots. Gold: road. Blue dashes: ferry. Native collision receipts and rendered before/after views are separate acceptance gates.</p>'+''.join(cards),encoding='utf-8')
for page in range(4):
 sheet=Image.new('RGB',(1200,1200),'#172127')
 for k in range(13):
  index=page*13+k
  if index>=51:break
  img=Image.open(out/f'{index+1:02d}.png').resize((300,240));sheet.paste(img,((k%4)*300,(k//4)*300))
 sheet.save(out/f'contact-{page+1}.png')
print('ATLAS_COMPLETE',len(metrics),'quarter_passes',sum(m['max_grade_025m_deg']<=22.1 for m in metrics))
