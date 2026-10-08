import sys,json
from pathlib import Path
import numpy as np
R=Path.cwd();E=R/'Evidence/MapFinalPolish-20261007';sys.path.insert(0,str(R/'Tools/MapPolish'));from route_surface import assess
D=json.loads((E/'Local/routes-art-r3.json').read_text());p=np.array([[727.45098,1660.78431],[738,1657.5],[750.55,1652.66]]);m=assess(p,.1);assert m['max_grade_deg']<=22.1 and not m['invalid_samples']
D['settlement_arrival_spurs'][0]['points_m']=p.tolist();D['settlement_arrival_spurs'][0]['reason']='Keep doorway base located from calibrated final camera pixel against native Landscape. Outer authored island gate is not represented.'
(E/'Local/routes-art-r4.json').write_text(json.dumps(D));(E/'human-keep-arrival-r2.json').write_text(json.dumps(dict(points_m=p.tolist(),grade=m,method='Calibrated 50-degree FOV native view: doorway-base image coordinate (897,507), ray against Landscape; final render review required.',authoritative_outer_gate_correspondence=False),indent=2));print(m)
import route_surface,runpy
route_surface.OUT=E;sys.argv=['prepare_road_control.py','--input','routes-art-r4.json','--revision','final2'];runpy.run_path(str(R/'Tools/MapPolish/prepare_road_control.py'),run_name='__main__')
