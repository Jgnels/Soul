"""Bounded candidate-only palette fix after the first rendered review."""
import unreal as u,json
from pathlib import Path
E=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929/Evidence/ProductionWorldComposition-20261007');A='/Game/SoulCampaignComposition';lib=u.MaterialEditingLibrary
m=u.load_asset(A+'/M_Composition_Review_r3') or u.EditorAssetLibrary.duplicate_asset(A+'/M_Composition_Review_r2',A+'/M_Composition_Review_r3');assert m
def node(kind,**props):
 n=lib.create_material_expression(m,getattr(u,'MaterialExpression'+kind))
 for k,v in props.items():n.set_editor_property(k,v)
 return n
def link(a,ao,b,bi):assert lib.connect_material_expressions(a,ao,b,bi)
canyon=u.find_object(m,'MaterialExpressionTextureSample_3');assert 'CanyonRock' in canyon.get_editor_property('texture').get_name()
desat=node('Desaturation');fraction=node('Constant',r=.78);link(canyon,'RGB',desat,'');link(fraction,'',desat,'Fraction')
link(desat,'',u.find_object(m,'MaterialExpressionLinearInterpolate_1'),'B')
mask=u.find_object(m,'MaterialExpressionTextureSample_2');road_weight=node('Multiply',const_b=.62);link(mask,'R',road_weight,'A')
for i in [14,15]:link(road_weight,'',u.find_object(m,'MaterialExpressionLinearInterpolate_'+str(i)),'Alpha')
# The first full-rock southern blend visibly repeated. Preserve the native
# surface variation and apply only the existing southern instance tint.
base=u.find_object(m,'MaterialExpressionBreakMaterialAttributes_2')
link(base,'BaseColor',u.find_object(m,'MaterialExpressionMultiply_0'),'A')
link(base,'Normal',u.find_object(m,'MaterialExpressionLinearInterpolate_7'),'B')
# Finer physical texture repeat reduces the obvious eight-metre pattern seen
# on the first badlands/ash render. Existing native terrain branches untouched.
for i in range(50):
 d=u.find_object(m,'MaterialExpressionDivide_'+str(i))
 if d and abs(d.get_editor_property('const_b')-800)<.01:d.set_editor_property('const_b',300.)
lib.recompile_material(m)
mi=u.EditorAssetLibrary.duplicate_asset(A+'/MI_Composition_Review_r3',A+'/MI_Composition_Review_r4');assert mi;lib.set_material_instance_parent(mi,m);lib.update_material_instance(mi)
for obj in [m,mi]:assert u.EditorAssetLibrary.save_loaded_asset(obj,False)
a=u.get_editor_subsystem(u.EditorActorSubsystem);land=next(x for x in a.get_all_level_actors() if isinstance(x,u.Landscape));land.call_method('EditorSetLandscapeMaterial',args=(mi,))
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(E/'surface-review-r3.json').write_text(json.dumps({'material':mi.get_path_name(),'changes':['owned canyon albedo desaturation 0.78','road blend 0.62','new-source texture repeat 3 m'],'donors_modified':False,'status':'provisional review material; not final biome art'},indent=2));print('REVIEW_SURFACE_R3')
