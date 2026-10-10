import unreal as u,json,math,time,traceback
from pathlib import Path
r=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));out=r/'Evidence/HumanCapitalSiege-20261010/Local/gate-ground';out.mkdir(exist_ok=True)
s=u.get_editor_subsystem(u.EditorActorSubsystem);w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=s.get_all_level_actors();gate=next(a for a in actors if a.get_actor_label()=='SM_Portcullis');c=gate.get_component_by_class(u.StaticMeshComponent);tr=c.get_world_transform();center=u.MathLibrary.transform_location(tr,u.Vector(201,0,0));yaw=c.get_world_rotation().yaw;rad=math.radians(yaw);f=u.Vector(-math.sin(rad),math.cos(rad),0);side=u.Vector(math.cos(rad),math.sin(rad),0);ignore=[a for a in actors if isinstance(a,u.Volume)]
def xyz(v):return [v.x,v.y,v.z]
rows=[]
for lane in (-110,0,110):
 samples=[]
 for dist in range(-2500,2501,100):
  p=center+f*dist+side*lane;hits=u.SystemLibrary.line_trace_multi_for_objects(w,u.Vector(p.x,p.y,center.z+180),u.Vector(p.x,p.y,-2000),[u.ObjectTypeQuery.OBJECT_TYPE_QUERY1],False,ignore,u.DrawDebugTrace.NONE)
  row=[]
  for h in hits or []:
   d=h.to_dict();a=d.get('hit_actor');row.append({'p':xyz(d['impact_point']),'normal':xyz(d['impact_normal']),'actor':a.get_name() if a else '', 'label':a.get_actor_label() if a else ''})
  samples.append({'dist':dist,'hits':row})
 rows.append({'lane':lane,'samples':samples})
(out/'ground.json').write_text(json.dumps({'gate_base':xyz(center),'forward':xyz(f),'yaw':yaw,'rows':rows},indent=2))
cam=s.spawn_actor_from_class(u.CameraActor,u.Vector());cam.get_component_by_class(u.CameraComponent).set_field_of_view(60);u.get_editor_subsystem(u.LevelEditorSubsystem).pilot_level_actor(cam)
views=[('gate_outside',center+u.Vector(0,0,180),-8,yaw+90,1800),('gate_inside',center+u.Vector(0,0,180),-8,yaw-90,1800),('gate_ground_context',center,-38,yaw+45,6500)]
st={'i':0,'phase':0,'at':time.monotonic()+2}
def tick(dt):
 if time.monotonic()<st['at']:return
 st['at']=float('inf')
 try:
  if st['i']>=len(views):u.unregister_slate_post_tick_callback(soul_siege_ground_tick);print('SIEGE_GROUND_DONE');return
  name,focus,pitch,angle,dist=views[st['i']]
  if st['phase']==0:
   rot=u.Rotator(pitch=pitch,yaw=angle,roll=0);cam.set_actor_location_and_rotation(focus-rot.get_forward_vector()*dist,rot,False,False);st['phase']=1;st['at']=time.monotonic()+4
  elif st['phase']==1:st['task']=u.AutomationLibrary.take_high_res_screenshot(1280,800,str(out/(name+'.png')),camera=cam,delay=1);st['phase']=2;st['at']=time.monotonic()+2
  elif st['task'].is_task_done():st['i']+=1;st['phase']=0;st['at']=time.monotonic()+1
  else:st['at']=time.monotonic()+2
 except Exception:(out/'error.txt').write_text(traceback.format_exc());u.unregister_slate_post_tick_callback(soul_siege_ground_tick)
u.EditorPythonScripting.set_keep_python_script_alive(True);soul_siege_ground_tick=u.register_slate_post_tick_callback(tick)
