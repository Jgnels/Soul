"""Read-only complex collision probes for the companion street; no saves."""
import unreal as u,json
from pathlib import Path
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert w.get_name()=='L_HumanCapital_Authored'
out=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))/'Evidence/HumanHeartlandDepth-20261010/Local/companion-street-probes.json'
rows=[]
for x,y in [(250,120),(450,120),(600,100),(400,350),(0,350),(-200,350)]:
 p=u.Vector(-4350+x,2700+y,300)
 h=u.SystemLibrary.line_trace_single_for_objects(w,p+u.Vector(0,0,400),p-u.Vector(0,0,800),[u.ObjectTypeQuery.OBJECT_TYPE_QUERY1],True,[],u.DrawDebugTrace.NONE);d=h.to_dict() if h else {};row={'offset':[x,y],'hit':bool(d.get('blocking_hit'))}
 if row['hit']:
  q=d['impact_point'];row.update(ground=[q.x,q.y,q.z],normal=d['impact_normal'].z,actor=d['hit_actor'].get_name())
  b=u.SystemLibrary.sphere_trace_single_for_objects(w,q+u.Vector(0,0,55),q+u.Vector(0,0,160),45,[u.ObjectTypeQuery.OBJECT_TYPE_QUERY1],True,[],u.DrawDebugTrace.NONE);v=b.to_dict() if b else {};row['blocked']=bool(v.get('blocking_hit'))
  if row['blocked']:row['blocker']=v['hit_actor'].get_name()
 rows.append(row)
out.write_text(json.dumps(rows,indent=2));print('SOUL_COMPANION_STREET_PROBES_DONE')
