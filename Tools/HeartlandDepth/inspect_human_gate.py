"""Read-only native Human Capital gate clearance and review. Never saves packages."""
import unreal as u,json,time,traceback
from pathlib import Path
r=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
out=r/'Evidence/HumanHeartlandDepth-20261010/Local/human-gate-inspection';out.mkdir(exist_ok=True)
assert u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/Soul/Maps/Settlements/L_HumanCapital_Authored')
s=u.get_editor_subsystem(u.EditorActorSubsystem)
state={'at':time.monotonic()+35}
def xyz(v):return [v.x,v.y,v.z]
def inspect(dt):
 if time.monotonic()<state['at']:return
 state['at']=float('inf')
 try:
  rows=[]
  for a in s.get_all_level_actors():
   label=a.get_actor_label();p=a.get_actor_location();c,e=a.get_actor_bounds(False)
   if 'gate' in label.lower() or ((p.x-5160)**2+(p.y-980)**2<9000**2 and ('wall' in label.lower() or 'cobble' in label.lower())):
    meshes=[]
    for m in a.get_components_by_class(u.StaticMeshComponent):
     if m.static_mesh:meshes.append({'mesh':m.static_mesh.get_path_name(),'transform':str(m.get_world_transform()),'collision':str(m.get_collision_enabled())})
    rows.append({'name':a.get_name(),'label':label,'class':a.get_class().get_name(),'location':xyz(p),'rotation':str(a.get_actor_rotation()),'bounds_center':xyz(c),'bounds_extent':xyz(e),'meshes':meshes})
  (out/'actors.json').write_text(json.dumps(rows,indent=2))
  print('SOUL_GATE_ACTORS',len(rows))
 except Exception:(out/'error.txt').write_text(traceback.format_exc())
 u.unregister_slate_post_tick_callback(soul_gate_handle)
u.EditorPythonScripting.set_keep_python_script_alive(True)
soul_gate_handle=u.register_slate_post_tick_callback(inspect)
