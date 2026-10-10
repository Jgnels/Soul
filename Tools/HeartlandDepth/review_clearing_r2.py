"""Transient cameras only: inspect native woodland geography, never save donor."""
import unreal as u,json,time,traceback
from pathlib import Path
r=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));out=r/'Evidence/HumanHeartlandDepth-20261010/Local/woodland-clearing-r2';out.mkdir(exist_ok=True)
s=u.get_editor_subsystem(u.EditorActorSubsystem);w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert w.get_outermost().get_name()=='/Game/Soul/Maps/Battles/L_Heartland_Woodland'
cam=s.spawn_actor_from_class(u.CameraActor,u.Vector());cam.get_component_by_class(u.CameraComponent).set_field_of_view(60)
cc=cam.get_component_by_class(u.CameraComponent)
pp=cc.get_editor_property('post_process_settings')
pp.set_editor_property('override_auto_exposure_min_brightness',True);pp.set_editor_property('override_auto_exposure_max_brightness',True)
pp.set_editor_property('auto_exposure_min_brightness',1.0);pp.set_editor_property('auto_exposure_max_brightness',1.0)
pp.set_editor_property('override_auto_exposure_bias',True);pp.set_editor_property('auto_exposure_bias',0.0)
cc.set_editor_property('post_process_settings',pp);cc.set_editor_property('post_process_blend_weight',1.0)
u.get_editor_subsystem(u.LevelEditorSubsystem).pilot_level_actor(cam)
views=[('clearing',(-9000,-9000,0),-55,90,9500),('deployment',(-9000,-9000,0),-25,20,5000)]
state={'i':0,'phase':0,'next':time.monotonic()+15,'shots':[]}
def soul_depth_capture_tick(dt):
 if time.monotonic()<state['next']:return
 state['next']=float('inf')
 try:
  if state['i']>=len(views):
   (out/'receipt.json').write_text(json.dumps({'map':w.get_outermost().get_name(),'donor_saved':False,'views':state['shots']},indent=2));u.unregister_slate_post_tick_callback(soul_depth_capture_handle);print('SOUL_WOODLAND_CAPTURE_DONE');return
  name,focus,pitch,yaw,distance=views[state['i']]
  if state['phase']==0:
   rot=u.Rotator(pitch=pitch,yaw=yaw,roll=0);cam.set_actor_location_and_rotation(u.Vector(*focus)-rot.get_forward_vector()*distance,rot,False,False);state['phase']=1;state['next']=time.monotonic()+7
  elif state['phase']==1:
   path=out/(name+'.png');state['task']=u.AutomationLibrary.take_high_res_screenshot(1280,800,str(path),camera=cam,delay=1);state['shots'].append({'name':name,'focus':focus,'distance':distance,'path':str(path)});state['phase']=2;state['next']=time.monotonic()+3
  elif state['task'].is_task_done():state['i']+=1;state['phase']=0;state['next']=time.monotonic()+1
  else:state['next']=time.monotonic()+2
 except Exception:
  (out/'error.txt').write_text(traceback.format_exc());u.unregister_slate_post_tick_callback(soul_depth_capture_handle)
u.EditorPythonScripting.set_keep_python_script_alive(True)
soul_depth_capture_handle=u.register_slate_post_tick_callback(soul_depth_capture_tick)
print('SOUL_WOODLAND_CAPTURE_QUEUED')
