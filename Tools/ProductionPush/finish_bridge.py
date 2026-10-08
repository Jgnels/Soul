"""One bounded cosmetic pass on the existing repaired bridge; collision untouched."""
import unreal as u,json,math
from pathlib import Path
R=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');E=R/'Evidence/ProductionPush-20261008';a=u.get_editor_subsystem(u.EditorActorSubsystem);w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert w.get_path_name().startswith('/Game/SoulCampaignComposition/L_Composition_3500_r2')
assert not any(x.get_actor_label().startswith('ProductionPush_BrokenRemnant') for x in a.get_all_level_actors()),'Run once only'
mesh=u.load_asset('/Game/SoulCampaignComposition/FinalPolish/SM_MasonryRubble_r1');assert mesh
B=R/'Evidence/ProductionWorldComposition-20261007';site=next(v for v in json.loads((B/'selected-crossing-sites-r3.json').read_text())['crossings'] if v['gate_id']=='orc_broken_bridge');deck=next(v for v in json.loads((B/'bridge-placement-r2.json').read_text())['bridges'] if v['id']=='orc_broken_bridge')['deck_z_m']*100
rot=u.Rotator(yaw=site['yaw_deg']);axis=rot.get_forward_vector();side=rot.get_right_vector();center=u.Vector(site['center_xy_m'][0]*100-175000,site['center_xy_m'][1]*100-175000,deck)
# Deliberately asymmetric remnants, always outside the 3.2m passage plus margin.
# All geometry is the existing reviewed owned masonry derivative, never generated art.
fragments=[(-340,270,30,130,90,95,13,11),(-420,315,-30,155,120,80,39,-17),(-520,255,-55,85,80,65,72,25),(355,-270,12,100,85,125,-21,18),(470,-310,-50,170,105,70,41,-11),(620,-260,-65,110,95,55,80,21),(390,285,-25,85,70,80,-38,14),(-365,-295,-30,120,90,55,17,-23)]
box=mesh.get_bounding_box();size=box.max-box.min;rows=[]
for i,(along,across,z,dx,dy,dz,yaw,roll) in enumerate(fragments):
 assert abs(across)-max(dx,dy)/2>180
 q=u.Rotator(pitch=0,yaw=site['yaw_deg']+yaw,roll=roll);scale=u.Vector(dx/size.x,dy/size.y,dz/size.z);p=center+axis*along+side*across+u.Vector(0,0,z);offset=q.quaternion().rotate_vector((box.min+box.max)*.5*scale)
 actor=a.spawn_actor_from_class(u.StaticMeshActor,p-offset);actor.set_actor_label('ProductionPush_BrokenRemnant_'+str(i));actor.static_mesh_component.set_static_mesh(mesh);actor.set_actor_scale3d(scale);actor.set_actor_rotation(q,False);actor.set_actor_enable_collision(False)
 rows.append(dict(label=actor.get_actor_label(),mesh=mesh.get_path_name(),center_cm=[p.x,p.y,p.z],dimensions_cm=[dx,dy,dz],yaw=q.yaw,roll=q.roll,collision=False))
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(E/'bridge-finish.json').write_text(json.dumps(dict(actors=rows,terrain_changed=False,route_points_changed=False,travel_collision_unchanged=True,status='PENDING_RENDER_REVIEW'),indent=2));print('BRIDGE_REMNANTS',len(rows))
