"""Native gate views and measured floor strips. Transient cameras, no package saves."""
import unreal as u,json,time,math,traceback
from pathlib import Path
r=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));out=r/'Evidence/HumanHeartlandDepth-20261010/Local/human-gate-inspection'
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert w.get_name()=='L_HumanCapital_Authored'
s=u.get_editor_subsystem(u.EditorActorSubsystem);ignored=[a for a in s.get_all_level_actors() if isinstance(a,u.Volume)]
def floor(x,y):
 h=u.SystemLibrary.line_trace_single_for_objects(w,u.Vector(x,y,1900),u.Vector(x,y,-2000),[u.ObjectTypeQuery.OBJECT_TYPE_QUERY1],False,ignored,u.DrawDebugTrace.NONE)
 d=h.to_dict() if h else {}
 if not d.get('blocking_hit'):return None
 return {'x':x,'y':y,'z':d['impact_point'].z,'normal':d['impact_normal'].z,'actor':d['hit_actor'].get_name() if d.get('hit_actor') else ''}
rows=[]
for yaw in (0,45,90,135):
 rad=math.radians(yaw)
 for lane in (-400,0,400):
  samples=[];prior=None;blocks=[]
  for d in range(-3000,3001,200):
   x=5160+d*math.cos(rad)-lane*math.sin(rad);y=980+d*math.sin(rad)+lane*math.cos(rad);p=floor(x,y);samples.append(p)
   if p:
    now=u.Vector(x,y,p['z']+100)
    if prior:
     hit=u.SystemLibrary.sphere_trace_single_for_objects(w,prior,now,45,[u.ObjectTypeQuery.OBJECT_TYPE_QUERY1],False,ignored,u.DrawDebugTrace.NONE)
     hd=hit.to_dict() if hit else {}
     if abs(now.z-prior.z)>90 or hd.get('blocking_hit'):blocks.append({'at':d,'step':now.z-prior.z,'actor':hd['hit_actor'].get_name() if hd.get('hit_actor') else ''})
    prior=now
   else:prior=None;blocks.append({'at':d,'missing':True})
  rows.append({'yaw':yaw,'lane':lane,'samples':samples,'blocks':blocks})
(out/'floor-strips.json').write_text(json.dumps(rows,indent=2))
cam=s.spawn_actor_from_class(u.CameraActor,u.Vector());cam.get_component_by_class(u.CameraComponent).set_field_of_view(60)
u.get_editor_subsystem(u.LevelEditorSubsystem).pilot_level_actor(cam)
views=[('gate_context',(5160,980,1100),-42,-135,10000),('gate_outside',(5160,980,1100),-15,45,4200),('gate_inside',(5160,980,1100),-15,225,4200)]
st={'i':0,'phase':0,'at':time.monotonic()+5}
def tick(dt):
 if time.monotonic()<st['at']:return
 st['at']=float('inf')
 try:
  if st['i']>=len(views):u.unregister_slate_post_tick_callback(soul_gate_review);print('SOUL_GATE_REVIEW_DONE');return
  name,f,pitch,yaw,dist=views[st['i']]
  if st['phase']==0:
   rot=u.Rotator(pitch=pitch,yaw=yaw,roll=0);cam.set_actor_location_and_rotation(u.Vector(*f)-rot.get_forward_vector()*dist,rot,False,False);st['phase']=1;st['at']=time.monotonic()+7
  elif st['phase']==1:st['task']=u.AutomationLibrary.take_high_res_screenshot(1280,800,str(out/(name+'.png')),camera=cam,delay=1);st['phase']=2;st['at']=time.monotonic()+3
  elif st['task'].is_task_done():st['i']+=1;st['phase']=0;st['at']=time.monotonic()+1
  else:st['at']=time.monotonic()+2
 except Exception:(out/'review-error.txt').write_text(traceback.format_exc());u.unregister_slate_post_tick_callback(soul_gate_review)
u.EditorPythonScripting.set_keep_python_script_alive(True)
soul_gate_review=u.register_slate_post_tick_callback(tick)
