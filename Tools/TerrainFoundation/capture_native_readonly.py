"""Read-only donor captures; camera changes are transient and NEVER saved.

Use a separately guarded source-only editor. Do not load city proxies. Do not
change source material/light/landscape/foliage actors. Capture the actual authored
source as it loads, including any visibility/material limitations.
"""
import unreal as u,json,time,traceback
from pathlib import Path
ROOT=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');OUT=ROOT/'Evidence/TerrainFoundation-20261007/Local/native-source-review-r2';OUT.mkdir(exist_ok=True)
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert w.get_path_name().startswith('/Game/LandscapePackOne/Maps/Mountain_05.')
a=u.get_editor_subsystem(u.EditorActorSubsystem);actors=a.get_all_level_actors();land=next(x for x in actors if isinstance(x,u.Landscape));inventory=[]
for x in actors:inventory.append({'name':x.get_name(),'label':x.get_actor_label(),'class':x.get_class().get_name()})
(OUT/'source-actors.json').write_text(json.dumps(inventory,indent=2))
cam=a.spawn_actor_from_class(u.CameraActor,u.Vector(0,0,0));cam.set_actor_label('Transient_UnSaved_SourceReviewCamera');cc=cam.get_component_by_class(u.CameraComponent);cc.set_field_of_view(52)
# Leave authored exposure/lighting intact. Only the camera and review cap change.
u.get_editor_subsystem(u.LevelEditorSubsystem).pilot_level_actor(cam)
fit=json.loads((ROOT/'Evidence/TerrainFoundation-20261007/dense-route-fit-r5.json').read_text());anchors={x['id']:x for x in fit['anchors']};factor=3500/8160
def native_focus(name):
 p=anchors[name];x,y=p['xy_m'];return ((8160-y/factor-4080)*100,(x/factor-4080)*100,(p['height_m']/factor-5-633.9)*100)
views=[('native_whole',(0,0,-50000),-78,-90,1632000),('native_human_basin',native_focus('human_capital'),-42,0,32000/factor),('native_dwarf_relief',native_focus('dwarf_hold'),-35,90,80000/factor),('native_close_relief',native_focus('northwest_march'),-30,0,18000/factor)]
state={'i':0,'phase':0,'next':time.monotonic()+4,'shots':[]}
def native_tick(dt):
 if time.monotonic()<state['next']:return
 state['next']=float('inf')
 try:
  if state['i']>=len(views):
   (OUT/'capture-receipt.json').write_text(json.dumps({'source_map':w.get_path_name(),'source_saved':False,'source_actor_material_light_edits':False,'temporary_camera_only':True,'fps_cap':5,'views':state['shots']},indent=2));u.unregister_slate_post_tick_callback(native_handle);print('NATIVE_SOURCE_CAPTURE_DONE');return
  name,focus,pitch,yaw,distance=views[state['i']]
  if state['phase']==0:
   rot=u.Rotator(pitch=pitch,yaw=yaw,roll=0);cam.set_actor_location_and_rotation(u.Vector(*focus)-rot.get_forward_vector()*distance,rot,False,False);state['phase']=1;state['next']=time.monotonic()+8
  elif state['phase']==1:
   path=OUT/(name+'.png');state['task']=u.AutomationLibrary.take_high_res_screenshot(1920,1080,str(path),camera=cam,delay=3);state['shots'].append({'name':name,'focus_cm':focus,'distance_cm':distance,'pitch':pitch,'yaw':yaw,'path':str(path)});state['phase']=2;state['next']=time.monotonic()+4
  elif state['task'].is_task_done():state['i']+=1;state['phase']=0;state['next']=time.monotonic()+1
  else:state['next']=time.monotonic()+2
 except Exception:
  (OUT/'capture-error.txt').write_text(traceback.format_exc());u.unregister_slate_post_tick_callback(native_handle)
native_handle=u.register_slate_post_tick_callback(native_tick);print('NATIVE_SOURCE_CAPTURE_QUEUED')
