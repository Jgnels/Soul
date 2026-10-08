"""Deck-sized native simple collision closes centimetre gaps between owned planks.

Visual triangles remain unchanged. Complex traces can fall through actual plank
gaps; campaign height / physical travel uses the regular simple query surface.
This is a standard UE collision representation, not a second movement authority.
"""
import unreal as u,json
from pathlib import Path
R=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');E=R/'Evidence/MapPolish-20261007'
mesh=u.load_asset('/Game/SoulCampaignComposition/Polish/SM_TimberDeck_Collision_r1');assert mesh
body=mesh.get_editor_property('body_setup');agg=u.KAggregateGeom();box=u.KBoxElem()
props={'center':u.Vector(85.116386,-118.751488,-2.3),'x':170.5,'y':245.503,'z':3.344,'name':u.Name('OwnedDeckFootprint')}
for key,value in props.items():box.set_editor_property(key,value)
agg.set_editor_property('box_elems',[box]);body.set_editor_property('agg_geom',agg);body.set_editor_property('collision_trace_flag',u.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX)
assert u.EditorAssetLibrary.save_loaded_asset(mesh,False)
a=u.get_editor_subsystem(u.EditorActorSubsystem);count=0
for actor in a.get_all_level_actors():
 if isinstance(actor,u.StaticMeshActor) and actor.static_mesh_component.static_mesh==mesh:
  actor.static_mesh_component.set_static_mesh(None);actor.static_mesh_component.set_static_mesh(mesh);count+=1
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(E/'timber-collision-hull.json').write_text(json.dumps({'mesh':mesh.get_path_name(),'method':'Native simple deck hull from measured owned planks; detailed complex render collision also retained','center_cm':[85.116386,-118.751488,-2.3],'dimensions_cm':[170.5,245.503,3.344],'top_cm':-.628,'source_plank_top_median_cm':-.6280427,'rebound_actors':count,'source_render_mesh_changed':False,'donor_modified':False},indent=2))
print('TIMBER_COLLISION_HULL',count)
