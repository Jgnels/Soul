from nwiro_client import call
from pathlib import Path
import time,json
p=Path(__file__).with_name('capture_waterfront.py')
print(call('execute_python',{'code':"import unreal\na=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)\ncam=next(x for x in a.get_all_level_actors() if x.get_actor_label()=='Study_ReviewCamera')\nunreal.EditorLevelLibrary.pilot_level_actor(cam)"}))
print(call('execute_python',{'code':p.read_text(encoding='utf-8-sig')}))
for i in range(30):
    time.sleep(5)
    r=call('take_screenshot',{})
    print('viewport draw',i,flush=True)
print('Capture draw cycle complete')
