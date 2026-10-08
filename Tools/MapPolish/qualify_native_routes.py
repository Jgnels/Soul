"""Trace the actual native routes in bounded editor ticks; no gameplay mutation."""
import unreal as u,json,time,math,traceback
from pathlib import Path
R=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');E=R/'Evidence/MapPolish-20261007'
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert w.get_path_name().startswith('/Game/SoulCampaignComposition/L_Composition_3500_r2')
a=u.get_editor_subsystem(u.EditorActorSubsystem);ignore=[x for x in a.get_all_level_actors() if x.get_actor_label().startswith('Polish_Inactive_')]
d=json.loads((E/'Local/native-route-input.json').read_text());todo=[]
for r in d['routes']:
 for s in r['segments']:
  s['hits']=[]
  for p in s.pop('points'):todo.append((s,p))
state={'index':0,'started':time.monotonic()}
def tick(dt):
 try:
  for s,(x,y,expected,station) in todo[state['index']:state['index']+2000]:
   result=u.SystemLibrary.line_trace_single(w,u.Vector(x*100-175000+.1,y*100-175000+.1,100000),u.Vector(x*100-175000+.1,y*100-175000+.1,-100000),u.TraceTypeQuery.ECC_VISIBILITY,False,ignore,u.DrawDebugTrace.NONE)
   hit=result.to_dict() if result else {};fallback=False
   if not hit.get('blocking_hit'):
    result=u.SystemLibrary.line_trace_single(w,u.Vector(x*100-175000+.37,y*100-175000+.23,100000),u.Vector(x*100-175000+.37,y*100-175000+.23,-100000),u.TraceTypeQuery.ECC_VISIBILITY,False,ignore,u.DrawDebugTrace.NONE)
    hit=result.to_dict() if result else {};fallback=True
   s['hits'].append(dict(xy_m=[x,y],station_m=station,expected_landscape_z_m=expected,z_m=hit['impact_point'].z/100 if hit.get('blocking_hit') else None,actor=hit['hit_actor'].get_actor_label() if hit.get('blocking_hit') else None,landscape=isinstance(hit.get('hit_actor'),u.Landscape),millimetre_edge_retry=fallback))
  state['index']+=2000
  if state['index']>=len(todo):
   (E/('Local/native-route-hits-'+globals().get('QUALIFY_REVISION','r1')+'.json')).write_text(json.dumps(d));u.unregister_slate_post_tick_callback(handle);print('POLISH_NATIVE_ROUTES_COMPLETE',len(todo),time.monotonic()-state['started'])
 except Exception:
  (E/'Local/native-route-error.txt').write_text(traceback.format_exc());u.unregister_slate_post_tick_callback(handle)
handle=u.register_slate_post_tick_callback(tick)
print('POLISH_NATIVE_ROUTES_QUEUED',len(todo))
