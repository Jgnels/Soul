"""Match runtime reserve corridor probes in the native woodland; no saves."""
import unreal as u,json,time
from pathlib import Path
r=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));out=r/'Evidence/HumanHeartlandDepth-20261010/Local/woodland-entry-probes.json'
assert u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/Soul/Maps/Battles/L_Heartland_Woodland')
at=time.monotonic()+15
def tick(dt):
 if time.monotonic()<at:return
 u.unregister_slate_post_tick_callback(woodland_entry_handle)
 w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();rows=[]
 for side in (0,1):
  for y in (0,-650,650):
   prev=None;probes=[]
   for i in range(4):
    x=-9000+(-2000+350*i/3 if side==0 else 2000-350*i/3);yy=-9000+y
    h=u.SystemLibrary.line_trace_single_for_objects(w,u.Vector(x,yy,512.925),u.Vector(x,yy,-687.075),[u.ObjectTypeQuery.OBJECT_TYPE_QUERY1],False,[],u.DrawDebugTrace.NONE)
    d=h.to_dict() if h else {};row={'x':x,'y':yy,'ground_hit':bool(d.get('blocking_hit'))}
    if d.get('blocking_hit'):
     p=d['impact_point'];row.update(z=p.z,normal=d['impact_normal'].z,ground_actor=d['hit_actor'].get_path_name() if d.get('hit_actor') else '')
     now=p+u.Vector(0,0,110)
     if prev:
      hit=u.SystemLibrary.sphere_trace_single_for_objects(w,prev,now,65,[u.ObjectTypeQuery.OBJECT_TYPE_QUERY1],False,[],u.DrawDebugTrace.NONE);b=hit.to_dict() if hit else {}
      row['sweep_blocked']=bool(b.get('blocking_hit'));row['step']=now.z-prev.z
      if b.get('blocking_hit'):row['blocker']=b['hit_actor'].get_path_name() if b.get('hit_actor') else ''
     prev=now
    probes.append(row)
   rows.append({'side':side,'lane':y,'probes':probes})
 out.write_text(json.dumps(rows,indent=2));print('SOUL_WOODLAND_ENTRY_PROBES_DONE')
u.EditorPythonScripting.set_keep_python_script_alive(True)
woodland_entry_handle=u.register_slate_post_tick_callback(tick)
