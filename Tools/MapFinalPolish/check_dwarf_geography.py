import sys,json
from pathlib import Path
import numpy as np
R=Path.cwd();E=R/'Evidence/MapFinalPolish-20261007';sys.path.insert(0,str(R/'Tools/MapPolish'));from route_surface import sample
center=np.array([2751.960784,600.347059]);rows=[]
for y in [575.647059,600.347059,617.647059,640]:
 offsets=np.arange(-100,101,5);p=np.c_[center[0]+offsets,np.full(len(offsets),y)];z=sample(p);rows.append(dict(y_m=y,offsets_m=offsets.tolist(),height_m=z.tolist(),center_height_m=float(z[20]),left_wall_15m_height_m=float(z[17]),right_wall_15m_height_m=float(z[23])))
(E/'dwarf-geographic-fit.json').write_text(json.dumps(dict(status='REJECTED AS FINISHED CAPITAL/PASS-FORT PRESENTATION',reason='Jeff reviewed the gate/hall cutaway and requires a road through a major canyon with walls closing passage. Current flat-supported freestanding placement does not establish that.',source_limitation='Existing r9 is temporary gate plus cutaway Caravan Hall, explicitly not final capital art.',terrain_changed=False,sections=rows),indent=2));print([(v['y_m'],v['center_height_m'],v['left_wall_15m_height_m'],v['right_wall_15m_height_m']) for v in rows])
