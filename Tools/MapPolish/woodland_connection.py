"""Use the existing woodland crossing for the two local Treehold connections."""
import json
import numpy as np
from route_surface import OUT,SITES,assess,good
from repair_ford import search

source=OUT/'Local/routes-fine-r1.json'
if not source.exists():source=OUT/'Local/routes-ford-r1.json'
data=json.loads(source.read_text())
routes={(r['a'],r['b']):r for r in data['routes']}
r=routes['nature_river_woodland','nature_treehold']; old=r['segments'][0]['points_m']
site=next(c for c in SITES if c['gate_id']=='woodland_bridge')
banks=sorted(site['bank_endpoints_xy_m'],key=lambda p:p[0])
left=search(np.array(old[0]),np.array(banks[0]),spacing=1,margin=24)
right=search(np.array(banks[1]),np.array(old[-1]),spacing=1,margin=100)
receipt={'crossing':'woodland_bridge','terrain_edited':False,'before':assess(old),'left_found':left is not None,'right_found':right is not None,'accepted':False}
if left is not None and right is not None:
    p=np.concatenate([left,right]); result=assess(p)
    if good(p,22.1):
        r['segments']=[{'type':'road','points_m':p.tolist()}]
        # The clearing branch joins the same road at River Woodland. This is a
        # presentation junction, not an extra campaign edge or movement rule.
        branch=routes['nature_forest_clearing','nature_river_woodland']['segments'][0]['points_m']
        routes['nature_forest_clearing','nature_treehold']['segments']=[{'type':'road','points_m':branch+p.tolist()[1:]}]
        receipt.update(accepted=True,after=result,reason='Two Treehold connections share the nearby woodland crossing; separate approach to the clearing remains. Southern Crossing retains all of its own legal connections.')
(OUT/'woodland-route-repair.json').write_text(json.dumps(receipt,indent=2))
(OUT/'Local/routes-woodland-r1.json').write_text(json.dumps(data,indent=2))
print(json.dumps(receipt,indent=2))
