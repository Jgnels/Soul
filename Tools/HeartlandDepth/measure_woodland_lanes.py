"""Measure native woodland deployment lanes; no terrain or donor saves."""
import unreal as u,json,math
from pathlib import Path
r=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert w.get_name()=='L_showcase_level'
ignored=[a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors() if isinstance(a,u.Volume)]
def sample(x,y):
 h=u.SystemLibrary.line_trace_single_for_objects(w,u.Vector(x,y,5000),u.Vector(x,y,-3000),[u.ObjectTypeQuery.OBJECT_TYPE_QUERY1],False,ignored,u.DrawDebugTrace.NONE)
 d=h.to_dict() if h else {}
 if not d.get('blocking_hit'):return None
 return {'x':x,'y':y,'z':d['impact_point'].z,'normal':d['impact_normal'].z,'actor':d['hit_actor'].get_name() if d.get('hit_actor') else ''}
rows=[]
for cx in range(-15000,18001,3000):
 for cy in range(-18000,18001,3000):
  points=[sample(cx+x,cy+y) for x in (-2200,-1500,0,1500,2200) for y in (-650,0,650)]
  if any(p is None for p in points):continue
  norm=min(p['normal'] for p in points);span=max(p['z'] for p in points)-min(p['z'] for p in points)
  clear=0
  for y in (-650,0,650):
   prev=None;ok=True
   for x in range(-2200,2201,200):
    p=sample(cx+x,cy+y)
    if not p or p['normal']<.7:ok=False;break
    now=u.Vector(p['x'],p['y'],p['z']+100)
    if prev:
     h=u.SystemLibrary.sphere_trace_single_for_objects(w,prev,now,50,[u.ObjectTypeQuery.OBJECT_TYPE_QUERY1],False,ignored,u.DrawDebugTrace.NONE)
     if abs(now.z-prev.z)>110 or (h and h.to_dict().get('blocking_hit')):ok=False;break
    prev=now
   clear+=int(ok)
  rows.append({'origin':[cx,cy,points[7]['z']],'min_normal':norm,'height_range':span,'crossing_lanes_clear':clear,'samples':points})
rows.sort(key=lambda x:(-x['crossing_lanes_clear'],x['min_normal']<.7,x['height_range']))
(r/'Evidence/HumanHeartlandDepth-20261010/Local/woodland-lanes-r2.json').write_text(json.dumps(rows,indent=2))
print(json.dumps([{k:v for k,v in x.items() if k!='samples'} for x in rows[:12]]))
