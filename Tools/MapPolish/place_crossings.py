"""Candidate-only crossing hierarchy from owned meshes; no Landscape edits."""
import unreal as u,json,math
from pathlib import Path
R=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');E=R/'Evidence/MapPolish-20261007';B=R/'Evidence/ProductionWorldComposition-20261007'
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert w.get_path_name().startswith('/Game/SoulCampaignComposition/L_Composition_3500_r2')
a=u.get_editor_subsystem(u.EditorActorSubsystem);actors={x.get_actor_label():x for x in a.get_all_level_actors()}
assert not any(n.startswith('Polish_Crossing_') for n in actors)
sites={s['gate_id']:s for s in json.loads((B/'selected-crossing-sites-r3.json').read_text())['crossings']}
decks={d['id']:d for d in json.loads((B/'bridge-placement-r2.json').read_text())['bridges']}
wood=u.load_asset('/Game/Forest_village/Meshes/Wood_modules/SM_bridge_module');assert wood
rows=[]
def hide(label,reason):
 actor=actors[label];actor.set_actor_hidden_in_game(True);actor.set_is_temporarily_hidden_in_editor(True);actor.set_actor_enable_collision(False)
 actor.set_actor_label('Polish_Inactive_'+label)
 rows.append(dict(actor=label,action='inactive, preserved in map',reason=reason))
def timber(id,center,span,yaw,deck_z,gradient,width):
 count=math.ceil(span/2.4);module_length=span/count;axis=u.Vector(math.cos(math.radians(yaw)),math.sin(math.radians(yaw)),0)
 # Native deck runs along -Y; +90 yaw aligns it to the crossing axis.
 rot=u.Rotator(pitch=0,yaw=yaw+90,roll=0);scale=u.Vector(width/1.7144374,module_length/2.45502976,1)
 local_center=u.Vector(85.116386*scale.x,-118.751488*scale.y,0)
 offset=rot.quaternion().rotate_vector(local_center)
 for i in range(count):
  along=-span/2+(i+.5)*module_length
  pos=u.Vector(center[0]*100-175000,center[1]*100-175000,(deck_z+along*gradient)*100)+axis*(along*100)-offset
  actor=a.spawn_actor_from_class(u.StaticMeshActor,pos);actor.set_actor_label('Polish_Crossing_'+id+'_timber_'+str(i));actor.static_mesh_component.set_static_mesh(wood);actor.set_actor_scale3d(scale);actor.set_actor_rotation(rot,False)
  # Rotate around the actual deck centre with the measured bank-to-bank grade.
  # Roll is the pitch about the module's local transverse X axis.
  tilted=u.Rotator(pitch=0,yaw=yaw+90,roll=math.degrees(math.atan(gradient)))
  tilt_offset=tilted.quaternion().rotate_vector(local_center)
  actor.set_actor_location_and_rotation(pos+offset-tilt_offset,tilted,False,False)
  actor.set_actor_enable_collision(True)
 rows.append(dict(id=id,type='owned timber modules',source=wood.get_path_name(),span_m=span,width_m=width,modules=count,center_xy_m=center,deck_z_m=deck_z,gradient=gradient))

hide('Composition_Bridge_woodland_bridge','No fitted route uses this crossing; bounded east-bank connection failed. Do not leave a decorative bridge suggesting a false route.')
id='human_north_bridge';s=sites[id];d=decks[id]
hide('Composition_Bridge_'+id,'Minor tributary crossing replaced with lighter owned timber modules.')
timber(id,s['center_xy_m'],s['span_m'],s['yaw_deg'],d['deck_z_m'],d['deck_gradient'],3.45)
id='orc_broken_bridge';s=sites[id];d=decks[id];old=actors['Composition_Bridge_'+id]
hide('Composition_Bridge_'+id,'Intact stone span contradicts Broken Bridge; preserve ruined ends and a narrow timber repair for the legal passage.')
for suffix in ['West','East']:
 asset=u.load_asset('/Game/SoulCampaignComposition/Polish/SM_RuinedBridge_'+suffix+'_r1');assert asset
 actor=a.spawn_actor_from_class(u.StaticMeshActor,old.get_actor_location());actor.set_actor_label('Polish_Crossing_broken_'+suffix);actor.static_mesh_component.set_static_mesh(asset);actor.set_actor_transform(old.get_actor_transform(),False,False)
timber(id,s['center_xy_m'],8.2,s['yaw_deg'],d['deck_z_m']+.035,d['deck_gradient'],3.1)
rows.append(dict(id=id,type='ruined stone ends with narrow timber repair',stone_gap_m=690/100*d['scale'][0],legal_connection_preserved=True))
# Persist only the owned candidate map and newly created Soul derivatives.
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(E/'crossing-hierarchy-r1.json').write_text(json.dumps({'map':w.get_path_name(),'rows':rows,'terrain_changed':False,'donors_saved':False,'native_clearance_pending':True},indent=2))
print('POLISH_CROSSINGS',len(rows))
