"""Owned copy of native water shader; feather only the existing ford's bank edges."""
import unreal as u,json
from pathlib import Path
R=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');E=R/'Evidence/MapFinalPolish-20261007';A='/Game/SoulCampaignComposition/FinalPolish';lib=u.MaterialEditingLibrary
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert w.get_path_name().startswith('/Game/SoulCampaignComposition/L_Composition_3500_r2')
source=u.load_asset('/Game/SoulCampaignComposition/MI_Composition_River_r1');parent=source.get_editor_property('parent');m=u.EditorAssetLibrary.duplicate_asset(parent.get_path_name().split('.')[0],A+'/M_FordBankWater_r2');assert m
mi=u.EditorAssetLibrary.duplicate_asset(source.get_path_name().split('.')[0],A+'/MI_FordBankWater_r2');assert mi;lib.set_material_instance_parent(mi,m)
old=lib.get_material_property_input_node(m,u.MaterialProperty.MP_OPACITY);output=str(lib.get_material_property_input_node_output_name(m,u.MaterialProperty.MP_OPACITY));assert old

def node(name,**kw):
 n=lib.create_material_expression(m,getattr(u,'MaterialExpression'+name))
 for k,v in kw.items():n.set_editor_property(k,v)
 return n

def link(a,b,pin,out=''):assert lib.connect_material_expressions(a,out,b,pin),(a.get_name(),b.get_name(),pin,out)
uv=node('TextureCoordinate');ucoord=node('ComponentMask',r=True,g=False,b=False,a=False);link(uv,ucoord,'')
right=node('Subtract',const_a=1.75);link(ucoord,right,'B');nearest=node('Min');link(ucoord,nearest,'A');link(right,nearest,'B');edge=node('Multiply',const_b=4.0);link(nearest,edge,'A');clamped=node('Clamp');link(edge,clamped,'')
# 7 m river width has UV U in [0,1.75]. The final 1 m of each bank fades.
wp=node('WorldPosition');xy=node('ComponentMask',r=True,g=True,b=False,a=False);link(wp,xy,'')
site=next(x for x in json.loads((R/'Evidence/ProductionWorldComposition-20261007/selected-crossing-sites-r3.json').read_text())['crossings'] if x['gate_id']=='river_ford');x,y=site['center_xy_m'];center=node('Constant2Vector',r=x*100-175000,g=y*100-175000);dist=node('Distance');link(xy,dist,'A');link(center,dist,'B');falloff=node('Subtract',const_a=3800.0);link(dist,falloff,'B');scale=node('Divide',const_b=2000.0);link(falloff,scale,'A');weight=node('Clamp');link(scale,weight,'')
blend=node('LinearInterpolate',const_a=1.0);link(clamped,blend,'B');link(weight,blend,'Alpha');opacity=node('Multiply');link(old,opacity,'A',output);link(blend,opacity,'B');assert lib.connect_material_property(opacity,'',u.MaterialProperty.MP_OPACITY)
lib.recompile_material(m);lib.update_material_instance(mi)
for asset in [m,mi]:assert u.EditorAssetLibrary.save_loaded_asset(asset)
actor=next(x for x in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors() if x.get_actor_label()=='Composition_Water_heart_river');actor.static_mesh_component.set_material(0,mi);assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(E/'ford-water-edge-r2.json').write_text(json.dumps(dict(source_parent=parent.get_path_name(),source_instance=source.get_path_name(),owned_material=m.get_path_name(),owned_instance=mi.get_path_name(),center_xy_m=[x,y],full_effect_radius_m=18,zero_effect_radius_m=38,bank_feather_m=1,river_geometry_changed=False,terrain_changed=False,water_height_changed=False,travel_collision_changed=False,review_pending=True),indent=2));print('FORD_BANK_FADE_BOUND')
