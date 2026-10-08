"""Fixed before/after Unreal camera views. Run live with CAPTURE_STAGE set."""
import unreal as u
import time,json,traceback
from pathlib import Path
R=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929')
E=R/'Evidence/ProductionContinuation-20261008'; stage=globals().get('CAPTURE_STAGE','before')
out=E/'Local'/('captures-'+stage);out.mkdir(parents=True,exist_ok=True)
a=u.get_editor_subsystem(u.EditorActorSubsystem);w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert w.get_path_name().startswith('/Game/SoulCampaignComposition/L_Composition_3500_r2')
cam=next(x for x in a.get_all_level_actors() if x.get_actor_label()=='Composition_ReviewCamera')
u.get_editor_subsystem(u.LevelEditorSubsystem).pilot_level_actor(cam)
def world(x,y,z):return (x*100-175000,y*100-175000,z*100)
views=[
 ('whole_labels_minimized',(0,0,5000),-60,-90,750000),
 ('ordinary_campaign',world(1200,1800,50),-42,0,190000),
 ('human_roads',world(750,1750,15),-55,0,110000),
 ('human_capital_approach',world(727.45,1660.78,15),-35,0,24000),
 ('river_ford',world(775.67,2196.26,5),-38,15,17000),
 ('major_stone_bridge',world(641.47,1797.92,5),-30,75,12000),
 ('minor_bridge',world(473.53,2809.61,50),-35,75,14000),
 ('broken_bridge',world(3052.98,1821.37,24),-35,75,16000),
 ('ferry_landing',world(1098.04,1303.92,3),-40,-35,40000),
 ('dwarf_approach',world(2752,617.6,30),-35,180,35000),
 ('mountain_pass',world(2182.4,1434.3,60),-40,0,65000),
 ('orc_routes',world(2750,1540,70),-50,180,125000),
 ('nature_paths',world(550,2730,65),-50,-45,105000),
 ('close_road',world(878.4,1379.4,15),-25,20,20000),
 ('viking_coast',world(750,430,25),-42,10,125000),
 ('dark_region',world(2370,2950,55),-45,180,130000),
 ('broken_bridge_close',world(3052.98,1821.37,24),-25,100,6500)]
state={'i':0,'phase':0,'next':time.monotonic()+3,'shots':[]}
def tick(dt):
 if time.monotonic()<state['next']:return
 state['next']=float('inf')
 try:
  if state['i']>=len(views):
   (out/'receipt.json').write_text(json.dumps({'map':w.get_path_name(),'fps_cap':12,'views':state['shots'],'editor_only':True},indent=2));u.unregister_slate_post_tick_callback(handle);print('POLISH_CAPTURE_COMPLETE',stage);return
  name,target,pitch,yaw,distance=views[state['i']]
  if state['phase']==0:
   rot=u.Rotator(pitch=pitch,yaw=yaw,roll=0);cam.set_actor_location_and_rotation(u.Vector(*target)-rot.get_forward_vector()*distance,rot,False,False);state['phase']=1;state['next']=time.monotonic()+8
  elif state['phase']==1:
   path=out/(name+'.png');state['task']=u.AutomationLibrary.take_high_res_screenshot(1920,1080,str(path),camera=cam,delay=2);state['shots'].append(dict(name=name,focus_cm=target,pitch=pitch,yaw=yaw,distance_cm=distance,path=str(path)));state['phase']=2;state['next']=time.monotonic()+3
  elif state['task'].is_task_done():state['i']+=1;state['phase']=0;state['next']=time.monotonic()+1
  else:state['next']=time.monotonic()+2
 except Exception:
  (out/'error.txt').write_text(traceback.format_exc());u.unregister_slate_post_tick_callback(handle)
handle=u.register_slate_post_tick_callback(tick)
print('POLISH_CAPTURE_QUEUED',stage,len(views))
