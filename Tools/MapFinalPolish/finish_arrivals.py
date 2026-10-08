import sys,json
from pathlib import Path
import numpy as np
R=Path.cwd();E=R/'Evidence/MapFinalPolish-20261007';sys.path.insert(0,str(R/'Tools/MapPolish'));from route_surface import assess,dense
D=json.loads((E/'Local/routes-art-r4.json').read_text());route=next(r for r in D['routes'] if {r['a'],r['b']}=={'dwarf_snow_basin','dwarf_hold'});p=dense(route['segments'][0]['points_m'],.2);target=np.array([2751.960784,575.647059]);idx=np.argmin(np.linalg.norm(p-target,axis=1));q=np.array([p[idx],target]);m=assess(q,.1);print(q.tolist(),m)
if m['max_grade_deg']<=22.1 and not m['invalid_samples']:
 D['settlement_arrival_spurs'].append(dict(id='dwarf_engineered_gate',type='settlement approach, not a new legal edge',points_m=q.tolist(),width_m=2.8));(E/'Local/routes-art-final.json').write_text(json.dumps(D));(E/'dwarf-gate-link.json').write_text(json.dumps(dict(points_m=q.tolist(),grade=m,connects='existing snow-basin road to engineered ramp foot',terrain_changed=False),indent=2))
else:(E/'Local/routes-art-final.json').write_text(json.dumps(D))
import route_surface,runpy
route_surface.OUT=E;sys.argv=['prepare_road_control.py','--input','routes-art-final.json','--revision','final3'];runpy.run_path(str(R/'Tools/MapPolish/prepare_road_control.py'),run_name='__main__')
sys.argv=['prepare_native_routes.py','--input','routes-presentation-final3.json'];runpy.run_path(str(R/'Tools/MapPolish/prepare_native_routes.py'),run_name='__main__')
