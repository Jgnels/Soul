"""Place existing licensed-derived miniatures at measured scale in candidate only."""
import unreal as u,json,math
from pathlib import Path
ROOT=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');OUT=ROOT/'Evidence/ProductionWorldComposition-20261007'
a=u.get_editor_subsystem(u.EditorActorSubsystem);w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert w.get_path_name().startswith('/Game/SoulCampaignComposition/')
assert not any(x.get_actor_label().startswith('Composition_Scale_') for x in a.get_all_level_actors())
fit=json.loads((OUT/'route-composition-proposal.json').read_text());anchors={x['id']:x for x in fit['anchors']}
trace_edge_fallbacks=[]
def ground(x,y):
 hit=u.SystemLibrary.line_trace_single(w,u.Vector(x,y,100000),u.Vector(x,y,-100000),u.TraceTypeQuery.ECC_VISIBILITY,True,[],u.DrawDebugTrace.NONE)
 d=hit.to_dict() if hit else {}
 if not d.get('blocking_hit'):
  # Chaos can miss a ray exactly on a heightfield vertex. The analytical
  # placements land on those vertices. Record a 1 mm diagnostic offset.
  hit=u.SystemLibrary.line_trace_single(w,u.Vector(x+.1,y+.1,100000),u.Vector(x+.1,y+.1,-100000),u.TraceTypeQuery.ECC_VISIBILITY,True,[],u.DrawDebugTrace.NONE)
  d=hit.to_dict() if hit else {};trace_edge_fallbacks.append([x,y,.1,.1])
 assert d.get('blocking_hit') and isinstance(d['hit_actor'],u.Landscape),(x,y,d)
 return d['impact_point'].z
def pos(node):
 x,y=anchors[node]['xy_m'];return (x*100-175000,y*100-175000)
records=[]
for node in anchors:
 x,y=pos(node);z=ground(x,y);actor=a.spawn_actor_from_class(u.TargetPoint,u.Vector(x,y,z));actor.set_actor_label('Composition_Anchor_'+node);actor.set_editor_property('tags',[u.Name('SoulCanonicalRegion'),u.Name(node)]);actor.set_actor_hidden_in_game(True);actor.set_is_temporarily_hidden_in_editor(True);records.append({'id':node,'xyz_cm':[x,y,z],'analytical_height_m':anchors[node]['height_m'],'collision_delta_cm':z-anchors[node]['height_m']*100})
def mesh(node,path,scale,dx=0,dy=0,yaw=0,suffix=''):
 asset=u.load_asset(path);assert asset,path
 b=asset.get_bounding_box();x,y=pos(node);x+=dx;y+=dy
 # Preserve the asset's scale and geometry; center its measured XY footprint.
 cx=(b.min.x+b.max.x)*.5*scale;cy=(b.min.y+b.max.y)*.5*scale;t=math.radians(yaw)
 ox=cx*math.cos(t)-cy*math.sin(t);oy=cx*math.sin(t)+cy*math.cos(t)
 z=ground(x,y);actor=a.spawn_actor_from_class(u.StaticMeshActor,u.Vector(x-ox,y-oy,z-b.min.z*scale+10));actor.set_actor_label('Composition_Scale_'+node+suffix);actor.static_mesh_component.set_static_mesh(asset);actor.set_actor_scale3d(u.Vector(scale,scale,scale));actor.set_actor_rotation(u.Rotator(pitch=0,yaw=yaw,roll=0),False);actor.set_actor_enable_collision(False)
 records.append({'role':'scale object, no new gameplay integration','id':node+suffix,'mesh':path,'scale':scale,'yaw':yaw,'center_xy_cm':[x,y],'ground_z_cm':z,'dimensions_m':[(b.max.x-b.min.x)*scale/100,(b.max.y-b.min.y)*scale/100,(b.max.z-b.min.z)*scale/100]})
 return actor
human=mesh('human_capital','/Game/Soul/CampaignProxies/Human/SM_HumanCapital_Base_r7',.24)
# Upgrade mesh shares the base pivot: preserve that exact relationship.
up=a.spawn_actor_from_class(u.StaticMeshActor,human.get_actor_location());up.set_actor_label('Composition_Scale_human_tavern');up.static_mesh_component.set_static_mesh(u.load_asset('/Game/Soul/CampaignProxies/Human/SM_HumanCapital_Upgrade_r7'));up.set_actor_transform(human.get_actor_transform(),False,False);up.set_actor_enable_collision(False)
mesh('dwarf_hold','/Game/Soul/CampaignProxies/Dwarven/SM_DwarfHold_Base_r9',.5)
for i,(dx,dy) in enumerate([(-700,-450),(750,-350),(0,700)]):mesh('northwest_march','/Game/SoulCampaignProxies/SM_Medieval_Building_A_r1',.36,dx,dy,i*95,'_house'+str(i))
mesh('old_quarry','/Game/SoulCampaignProxies/SM_Medieval_Building_B_r1',.6)
for i,(dx,dy) in enumerate([(-550,0),(550,0),(0,900)]):mesh('orc_camp','/Game/Soul/CampaignProxies/Population/SM_Camp_Tent_r1',.45,dx,dy,i*115,'_tent'+str(i))
# Same 2.5 presentation scale as retained campaign hero, with conventional idle.
x,y=pos('human_capital');x+=8000;y+=2500;z=ground(x,y)
hero=a.spawn_actor_from_class(u.SkeletalMeshActor,u.Vector(x,y,z));hero.set_actor_label('Composition_Scale_Army');sk=hero.skeletal_mesh_component;sk.set_skeletal_mesh_asset(u.load_asset('/Game/ParagonAurora/Characters/Heroes/Aurora/Meshes/Aurora'));hero.set_actor_scale3d(u.Vector(2.5,2.5,2.5));hero.set_actor_enable_collision(False);sk.set_animation_mode(u.AnimationMode.ANIMATION_SINGLE_NODE);sk.play_animation(u.load_asset('/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Idle'),True)
records.append({'role':'same retained army hero scale','scale':2.5,'height_reference_m':6,'position_cm':[x,y,z]})
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(OUT/'scale-object-placement-r1.json').write_text(json.dumps({'objects':records,'trace_edge_fallbacks_cm':trace_edge_fallbacks},indent=2));print('FOUNDATION_SCALE_OBJECTS',len(records))
