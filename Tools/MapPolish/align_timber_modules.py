"""Orient the owned module along its actual rail direction (+X), not its long AABB axis."""
import unreal as u,json,math
from pathlib import Path
R=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');E=R/'Evidence/MapPolish-20261007';B=R/'Evidence/ProductionWorldComposition-20261007'
a=u.get_editor_subsystem(u.EditorActorSubsystem);w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert w.get_path_name().startswith('/Game/SoulCampaignComposition/L_Composition_3500_r2')
wood=u.load_asset('/Game/SoulCampaignComposition/Polish/SM_TimberDeck_Collision_r1');assert wood
body=wood.get_editor_property('body_setup');agg=body.get_editor_property('agg_geom');boxes=list(agg.get_editor_property('box_elems'));boxes[0].set_editor_property('x',171.44374);agg.set_editor_property('box_elems',boxes);body.set_editor_property('agg_geom',agg);u.EditorAssetLibrary.save_loaded_asset(wood,False)
for actor in a.get_all_level_actors():
 label=actor.get_actor_label()
 if label.startswith('Polish_Crossing_') and '_timber_' in label or label.startswith('Polish_Landing_') and '_deck_' in label:a.destroy_actor(actor)
rows=[]
for c in json.loads((E/'crossing-hierarchy-r1.json').read_text())['rows']:
 if c.get('type')=='owned timber modules':rows.append(dict(c,prefix='Polish_Crossing_'+c['id']+'_timber_',yaw_deg=next(s['yaw_deg'] for s in json.loads((B/'selected-crossing-sites-r3.json').read_text())['crossings'] if s['gate_id']==c['id'])))
for c in json.loads((E/'ferry-jetty-placement.json').read_text())['jetties']:rows.append(dict(c,prefix='Polish_Landing_'+c['id']+'_deck_',gradient=0))
report=[]
for row in rows:
 span=row['span_m'];count=math.ceil(span/1.7);length=span/count;rot=u.Rotator(pitch=math.degrees(math.atan(row['gradient'])),yaw=row['yaw_deg'],roll=0)
 axis=rot.get_forward_vector();horizontal=u.Vector(math.cos(math.radians(row['yaw_deg'])),math.sin(math.radians(row['yaw_deg'])),0)
 scale=u.Vector(length/1.7144374,row['width_m']/2.45502976,1)
 offset=rot.quaternion().rotate_vector(u.Vector(85.116386*scale.x,-118.751488*scale.y,0))
 for i in range(count):
  along=-span/2+(i+.5)*length;center=u.Vector(row['center_xy_m'][0]*100-175000,row['center_xy_m'][1]*100-175000,(row['deck_z_m']+along*row['gradient'])*100)+horizontal*(along*100)
  actor=a.spawn_actor_from_class(u.StaticMeshActor,center-offset);actor.set_actor_label(row['prefix']+str(i));actor.static_mesh_component.set_static_mesh(wood);actor.set_actor_scale3d(scale);actor.set_actor_rotation(rot,False);actor.static_mesh_component.set_collision_profile_name('BlockAll');actor.set_actor_enable_collision(True)
 report.append(dict(id=row['id'],modules=count,span_m=span,width_m=row['width_m'],longitudinal_native_axis='X',rail_alignment='parallel to route'))
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(E/'timber-orientation-r2.json').write_text(json.dumps({'rejected_iteration':'r1 used longest AABB axis Y; actual rails run X. Render review caught transverse rails.','rows':report,'donors_changed':False,'terrain_changed':False},indent=2))
print('TIMBER_MODULES_ALIGNED',report)
