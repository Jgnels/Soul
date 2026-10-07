"""Candidate crossing scale proof from owned stone bridge; no donor mutation."""
import unreal as u,json,math
from pathlib import Path
ROOT=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');E=ROOT/'Evidence/ProductionWorldComposition-20261007'
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();a=u.get_editor_subsystem(u.EditorActorSubsystem)
assert w.get_path_name().startswith('/Game/SoulCampaignComposition/L_Composition_3500_r2')
assert not any(x.get_actor_label().startswith('Composition_Bridge_') for x in a.get_all_level_actors())
asset=u.load_asset('/Game/Kingdom_Capital/Meshes/Bridge/SM_arch_bridge_01');assert asset
def ground(p):
 x,y=p[0]*100-175000,p[1]*100-175000
 h=u.SystemLibrary.line_trace_single(w,u.Vector(x+.1,y+.1,100000),u.Vector(x+.1,y+.1,-100000),u.TraceTypeQuery.ECC_VISIBILITY,False,[],u.DrawDebugTrace.NONE).to_dict()
 assert h.get('blocking_hit') and isinstance(h['hit_actor'],u.Landscape),p
 return h['impact_point'].z/100
records=[]
for c in json.loads((E/'selected-crossing-sites-r3.json').read_text())['crossings']:
 if c['type']=='ford':continue
 banks=[ground(p) for p in c['bank_endpoints_xy_m']];sx=c['span_m']/19.8;sy=.8;sz=.7;t=math.radians(c['yaw_deg']);deck=max(banks)+.15
 cx,cy=c['center_xy_m'];ox=990*sx;oy=293.9635*sy
 actor=a.spawn_actor_from_class(u.StaticMeshActor,u.Vector(cx*100-175000+ox*math.cos(t)-oy*math.sin(t),cy*100-175000+ox*math.sin(t)+oy*math.cos(t),deck*100));actor.set_actor_label('Composition_Bridge_'+c['gate_id']);actor.static_mesh_component.set_static_mesh(asset);actor.set_actor_scale3d(u.Vector(sx,sy,sz));actor.set_actor_rotation(u.Rotator(pitch=0,yaw=c['yaw_deg'],roll=0),False)
 actor.set_actor_enable_collision(False)
 records.append(dict(id=c['gate_id'],mesh=asset.get_path_name(),bank_z_m=banks,deck_z_m=deck,span_m=c['span_m'],yaw=c['yaw_deg'],scale=[sx,sy,sz],status='provisional crossing presentation; road/deck join needs rendered review; broken bridge uses intact asset temporarily'))
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(E/'bridge-placement-r1.json').write_text(json.dumps({'bridges':records,'donor_modified':False,'new_owned_wood_pack':'not yet locally inspected; no synthetic substitute'},indent=2));print('BRIDGES_PLACED',len(records))
