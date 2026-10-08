import unreal as u,json,math
from pathlib import Path
R=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');E=R/'Evidence/ProductionPush-20261008';a=u.get_editor_subsystem(u.EditorActorSubsystem);w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert w.get_path_name().startswith('/Game/SoulCampaignComposition/')
assert not any(x.get_actor_label().startswith('ProductionPush_Dwarf') for x in a.get_all_level_actors())
ignore=[x for x in a.get_all_level_actors() if not isinstance(x,u.Landscape)]
def ground(x,y):
 h=u.SystemLibrary.line_trace_single(w,u.Vector(x+.1,y+.1,100000),u.Vector(x+.1,y+.1,-100000),u.TraceTypeQuery.ECC_VISIBILITY,False,ignore,u.DrawDebugTrace.NONE).to_dict();assert h['blocking_hit'];return h['impact_point'].z
hidden=[]
for x in a.get_all_level_actors():
 if x.get_actor_label()=='Composition_Scale_dwarf_hold' or x.get_actor_label().startswith('FinalPolish_Dwarf_'):
  x.set_actor_hidden_in_game(True)
  for c in x.get_components_by_class(u.PrimitiveComponent):c.set_visibility(False)
  hidden.append(x.get_actor_label())
x,y=101246.078431,-113235.294118;z=ground(x,y)-15
rows=[]
def spawn(name,mesh,loc,scale,yaw=0):
 ac=a.spawn_actor_from_class(u.StaticMeshActor,u.Vector(*loc),u.Rotator(pitch=0,yaw=yaw));ac.set_actor_label('ProductionPush_Dwarf_'+name);ac.static_mesh_component.set_static_mesh(u.load_asset(mesh));ac.set_actor_scale3d(u.Vector(*scale));ac.set_actor_enable_collision(False);rows.append(dict(label=ac.get_actor_label(),mesh=mesh,location=loc,scale=scale,yaw=yaw));return ac
spawn('Exterior','/Game/SoulCampaignComposition/ProductionPush/SM_DwarfExterior_r1',[x,y,z],[.18,.18,.18])
# Owned native geology backs the portal; this is a local static-mesh outcrop,
# never a Landscape edit or a claimed canyon barrier.
rock='/Game/DwarvenCitadel/Meshes/SM_Rocks_M_03'
spawn('BackingRock',rock,[x+1450,y,z-80],[10,13,19],18)
spawn('NorthRock',rock,[x+650,y-1250,z-160],[7,8,15],-35)
spawn('SouthRock',rock,[x+750,y+1300,z-220],[8,7,13],42)
# Short arrival from unchanged canonical anchor; use native paving, no terrain pad.
mesh='/Game/DwarvenCitadel/Meshes/SM_ExteriorFloor_A_01';m=u.load_asset(mesh);b=m.get_bounding_box();size=b.max-b.min
start=u.Vector(100196.078431,y,ground(100196.078431,y)+8);end=u.Vector(x-150,y,z+12);delta=end-start;pitch=math.degrees(math.atan2(delta.z,delta.x));rot=u.Rotator(pitch=pitch,yaw=0,roll=0);sc=u.Vector(delta.length()/size.x,320/size.y,45/size.z);center=(start+end)*.5;offset=rot.quaternion().rotate_vector((b.min+b.max)*.5*sc)
paver=spawn('Arrival',mesh,[center.x-offset.x,center.y-offset.y,center.z-offset.z],[sc.x,sc.y,sc.z]);paver.set_actor_rotation(rot,False)
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(E/'dwarf-exterior-placement-r1.json').write_text(json.dumps(dict(hidden_rejected_actors=hidden,actors=rows,canonical_anchor_unchanged=True,terrain_unchanged=True,arrival_grade_deg=pitch,status='UNREVIEWED bounded exterior trial'),indent=2))
print('DWARF_EXTERIOR_TRIAL',len(hidden),pitch)
