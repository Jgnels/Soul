"""Serialize the frozen candidate for the existing campaign presentation adapter."""
from pathlib import Path
import sys,json,hashlib,numpy as np
from PIL import Image
R=Path.cwd();E=R/'Evidence/ProductionPush-20261008';O=R/'Data/CampaignComposition';O.mkdir(exist_ok=True)
sys.path.insert(0,str(R/'Tools/MapPolish'));from route_surface import dense,surface,sample
src=E/'Local/routes-final.json'
if not src.exists():src=E/'Local/routes-baseline.json'
d=json.loads(src.read_text());world=json.loads((R/'Data/soul_world_overmap_v1_20260922.json').read_text());starts=json.loads((R/'Data/soul_campaign_start_states_v1_20260922.json').read_text())
profile=dict(schema=1,status='OPT_IN_RUNTIME_CANDIDATE_NOT_PROMOTED',terrain=dict(map='/Game/SoulCampaignComposition/L_Composition_3500_r2',resolution=2041,height_unit_cm=100/128,minimum_xy_cm=[-175000,-175000],extent_xy_cm=[350000,350000]),regions={},display_names={n['id']:n['name'] for n in world['nodes']},routes=[])
for a in d['anchors']:profile['regions'][a['id']]=[round(v*100-175000,5) for v in a['xy_m']]+[round(float(sample(a['xy_m']))*100,5)]
for r in d['routes']:
 points=[];ferry=[];arc=0.
 for s in r['segments']:
  q=dense(s['points_m'],1.0);z=surface(q)[0] if s['type']=='road' else np.zeros(len(q));start=arc
  for xy,h in zip(q,z):
   p=[round(xy[0]*100-175000,4),round(xy[1]*100-175000,4),round(float(h)*100+22,4)]
   if points:
    step=np.linalg.norm(np.array(p[:2])-points[-1][:2])
    if step<.001:continue
    arc+=float(step)
   points.append(p)
  if s['type']=='ferry':ferry.append(dict(id=s['id'],start_cm=start,end_cm=arc))
 profile['routes'].append(dict(a=r['a'],b=r['b'],points=points,ferries=ferry))
actors=json.loads((E/'candidate-scale-actors.json').read_text());human=next(a for a in actors if a['label']=='Composition_Scale_human_capital')
profile['miniatures']={'human_capital':{k:human[k] for k in ('location','rotation','scale')}}
profile['source_routes_sha256']=hashlib.sha256(src.read_bytes()).hexdigest();profile['height_png_sha256']=hashlib.sha256((R/'Data/CampaignCompositionLocal/Composition_3500_r2.png').read_bytes()).hexdigest()
(O/'presentation.json').write_text(json.dumps(profile,separators=(',',':')))
height=np.asarray(Image.open(R/'Data/CampaignCompositionLocal/Composition_3500_r2.png'),dtype='<u2');(R/'Data/CampaignCompositionLocal/Composition_3500_r2.r16').write_bytes(height.tobytes())
# Qualification fixtures reuse the existing founder combat and authored Human setup.
# They are not final six-faction balance or a different campaign authority.
for human in [False,True]:
 cfg=json.loads((R/('Data/SettlementEnvironments/HumanCapitalRuntimeProof.json' if human else 'Data/soul_vertical_scenario_20260925.json')).read_text())
 old=cfg.get('start_state',starts['scenarios']['founder_human_orc_micro']);state=dict(old);state['region_ids']=[n['id'] for n in world['nodes']];state['owners']={n['id']:'' for n in world['nodes']};state['owners'].update(old['owners']);cfg['start_state']=state;cfg['scenario_id']='composition_3500_'+('human_authored_qualification' if human else 'founder_runtime_qualification');cfg['status']='OPT_IN_QUALIFICATION_FIXTURE; canonical graph/rules unchanged; retained founder ownership overlay; outer regions neutral in this qualification fixture because current campaign save/combat authority is two-faction. Six-faction gameplay is NOT claimed.'
 (O/('HumanRuntimeProof.json' if human else 'RuntimeProof.json')).write_text(json.dumps(cfg,indent=2))
(E/'runtime-data-receipt.json').write_text(json.dumps(dict(regions=len(profile['regions']),routes=len(profile['routes']),ferry_route_uses=sum(bool(r['ferries']) for r in profile['routes']),ferry_passages=sorted({f['id'] for r in profile['routes'] for f in r['ferries']}),heightfield_transformed=False,height_serialization='lossless uint16 PNG to little-endian R16 only',source_routes_sha256=profile['source_routes_sha256'],height_png_sha256=profile['height_png_sha256']),indent=2))
print('PROFILE',len(profile['regions']),len(profile['routes']),'points',sum(len(r['points']) for r in profile['routes']))
