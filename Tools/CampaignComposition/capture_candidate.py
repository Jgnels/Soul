"""Actual Unreal views; no image generation/compositing of player-facing content."""
import unreal as u,time,json,traceback
from pathlib import Path
ROOT=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');revision=globals().get('CAPTURE_REVISION','r2');out=ROOT/('Evidence/ProductionWorldComposition-20261007/Local/candidate-'+revision+'-captures');out.mkdir(parents=True,exist_ok=True)
a=u.get_editor_subsystem(u.EditorActorSubsystem);w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert w.get_path_name().startswith('/Game/SoulCampaignComposition/')
cam=next(x for x in a.get_all_level_actors() if x.get_actor_label()=='Composition_ReviewCamera');u.get_editor_subsystem(u.LevelEditorSubsystem).pilot_level_actor(cam)
fit=json.loads((ROOT/'Evidence/ProductionWorldComposition-20261007/route-composition-proposal.json').read_text());anchors={x['id']:x for x in fit['anchors']}
def focus(node):
 f=anchors[node];return (f['xy_m'][0]*100-175000,f['xy_m'][1]*100-175000,f['height_m']*100+1000)
views=[('whole_labels_minimized',(0,0,5000),-60,-90,750000),('ordinary_heartland',focus('human_capital'),-38,0,85000),('human_capital_scale',focus('human_capital'),-35,0,30000),('dwarf_mountains',focus('dwarf_hold'),-35,180,100000),('northern_coast',focus('viking_harbour'),-40,0,130000),('orc_eastern_terrain',focus('orc_camp'),-40,180,110000),('nature_terrain',focus('nature_treehold'),-40,-45,115000),('dark_southern_terrain',focus('dark_fortress'),-38,180,110000),('pass_study',focus('north_pass'),-35,0,65000),('ford_site_unresolved',focus('river_ford'),-40,0,80000),('minor_settlement_scale',focus('northwest_march'),-36,0,35000),('close_ground_road',focus('old_quarry'),-25,20,20000)]
crossings=json.loads((ROOT/'Evidence/ProductionWorldComposition-20261007/selected-crossing-sites-r3.json').read_text())['crossings']
bridge=next(c for c in crossings if c['gate_id']=='human_west_bridge')
views.extend([('major_river',(bridge['center_xy_m'][0]*100-175000,bridge['center_xy_m'][1]*100-175000,bridge['water_z_m']*100),-45,100,65000),('bridge_bank_inspection',(bridge['center_xy_m'][0]*100-175000,bridge['center_xy_m'][1]*100-175000,bridge['water_z_m']*100+200),-25,75,11000)])
views=[(name.replace('ford_site_unresolved','ford_candidate'),*rest) for name,*rest in views]
views=views[:3] if globals().get('EARLY_REVIEW',False) else views
state={'i':0,'phase':0,'next':time.monotonic()+3,'shots':[]}
def tick(dt):
 if time.monotonic()<state['next']:return
 state['next']=float('inf')
 try:
  if state['i']>=len(views):
   (out/'capture-receipt-r1.json').write_text(json.dumps({'map':w.get_path_name(),'fps_cap':12,'views':state['shots'],'editor_only':True,'gameplay_not_qualified':True},indent=2));u.unregister_slate_post_tick_callback(handle);print('FOUNDATION_CAPTURE_COMPLETE');return
  name,target,pitch,yaw,distance=views[state['i']]
  if state['phase']==0:
   rot=u.Rotator(pitch=pitch,yaw=yaw,roll=0);cam.set_actor_location_and_rotation(u.Vector(*target)-rot.get_forward_vector()*distance,rot,False,False);state['phase']=1;state['next']=time.monotonic()+8
  elif state['phase']==1:
   path=out/(name+'.png');state['task']=u.AutomationLibrary.take_high_res_screenshot(1920,1080,str(path),camera=cam,delay=2);state['shots'].append({'name':name,'focus_cm':target,'pitch':pitch,'yaw':yaw,'distance_cm':distance,'path':str(path)});state['phase']=2;state['next']=time.monotonic()+3
  elif state['task'].is_task_done():state['i']+=1;state['phase']=0;state['next']=time.monotonic()+1
  else:state['next']=time.monotonic()+2
 except Exception:
  (out/'capture-error.txt').write_text(traceback.format_exc());u.unregister_slate_post_tick_callback(handle)
handle=u.register_slate_post_tick_callback(tick)
print('FOUNDATION_CAPTURE_QUEUED',len(views))
