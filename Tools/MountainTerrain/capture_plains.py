import unreal as u,time,json,traceback
from pathlib import Path
ROOT=Path('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview');OUT=ROOT/'TerrainWork/Captures';OUT.mkdir(exist_ok=True)
a=u.get_editor_subsystem(u.EditorActorSubsystem)
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();tag=w.get_name()
cam=next(x for x in a.get_all_level_actors() if x.get_actor_label()=='Study_ReviewCamera')
views=[('overhead',(0,0,2000),-90,-90,322500),('campaign',(0,0,2000),-45,-90,280000),('human_plains',(10000,20000,1000),-38,-90,90000),('dwarf_foothills',(25000,35000,1700),-32,-25,65000),('northern_lake',(15000,-49000,700),-40,-90,115000)]
state={'i':0,'phase':0,'next':time.monotonic()+6,'shots':[]}
def tick(dt):
    if time.monotonic()<state['next']:return
    state['next']=float('inf')
    try:
        if state['i']>=len(views):
            (OUT/(tag+'-receipt.json')).write_text(json.dumps(state['shots'],indent=2));u.unregister_slate_post_tick_callback(handle);print('PLAINS_CAPTURE_COMPLETE '+tag);return
        name,focus,pitch,yaw,distance=views[state['i']]
        if state['phase']==0:
            rot=u.Rotator(pitch=pitch,yaw=yaw,roll=0);cam.set_actor_location_and_rotation(u.Vector(*focus)-rot.get_forward_vector()*distance,rot,False,False)
            state['next']=time.monotonic()+5;state['phase']=1
        elif state['phase']==1:
            u.AutomationLibrary.finish_loading_before_screenshot();p=OUT/(tag+'_'+name+'.png')
            state['task']=u.AutomationLibrary.take_high_res_screenshot(1920,1080,str(p),camera=cam,delay=2)
            state['shots'].append({'file':str(p),'focus':focus,'distance':distance,'pitch':pitch,'yaw':yaw});state['phase']=2;state['next']=time.monotonic()+4
        elif state['task'].is_task_done():state['i']+=1;state['phase']=0;state['next']=time.monotonic()+2
        else:state['next']=time.monotonic()+1
    except Exception:
        u.log_error('PLAINS_CAPTURE_FAILED '+traceback.format_exc());u.unregister_slate_post_tick_callback(handle)
handle=u.register_slate_post_tick_callback(tick)
