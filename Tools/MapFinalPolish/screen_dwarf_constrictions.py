"""Read-only screen for genuine two-sided constrictions on existing Dwarf roads."""
import sys,json
from pathlib import Path
import numpy as np
R=Path.cwd();E=R/'Evidence/MapFinalPolish-20261007';sys.path.insert(0,str(R/'Tools/MapPolish'));from route_surface import dense,sample
D=json.loads((E/'Local/routes-presentation-final5.json').read_text());hold=np.array(next(a['xy_m'] for a in D['anchors'] if a['id']=='dwarf_hold'));rows=[]
for r in D['routes']:
 if not ('dwarf' in r['a'] or 'dwarf' in r['b']):continue
 for s in r['segments']:
  if s['type']!='road':continue
  p=dense(s['points_m'],2)
  for i in range(10,len(p)-10,5):
   axis=p[i+8]-p[i-8];axis/=max(np.linalg.norm(axis),.001);cross=np.array([-axis[1],axis[0]]);z=float(sample(p[i]));sides={}
   for dist in [15,30,50,80]:sides[str(dist)]=(sample(np.array([p[i]-cross*dist,p[i]+cross*dist]))-z).tolist()
   rows.append(dict(route=[r['a'],r['b']],xy_m=p[i].tolist(),height_m=z,distance_from_hold_m=float(np.linalg.norm(p[i]-hold)),cross_axis=cross.tolist(),flank_rise_m=sides,score=min(sides['30'])))
ranked=sorted(rows,key=lambda x:x['score'],reverse=True);local=[r for r in ranked if r['distance_from_hold_m']<150];candidates=[]
for r in ranked:
 if all(np.linalg.norm(np.array(r['xy_m'])-v['xy_m'])>90 for v in candidates):candidates.append(r)
 if len(candidates)==5:break
(E/'dwarf-existing-constriction-screen.json').write_text(json.dumps(dict(analysis_only=True,not_relocation_approval=True,criteria='Road-center transverse relief on both sides; ranking alone is not a visually qualified canyon or impassable-barrier proof.',local_best=local[:5],regional_candidates=candidates),indent=2));print(json.dumps(dict(local_best=local[:1],regional_best=candidates[:3]),indent=2))
