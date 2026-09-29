"""Run in the admitted live Unreal editor. Saves only TerrainV2-owned assets."""
import unreal as u
from pathlib import Path
ROOT='/Game/Soul/Campaign/TerrainV2'
src=Path(u.Paths.project_dir())/'SourceArt/SoulCampaignTerrain'
lib=u.MaterialEditingLibrary
assets=u.AssetToolsHelpers.get_asset_tools()
for name in ['FounderMacro','FounderBiome']:
 task=u.AssetImportTask(); task.filename=str(src/(name+'.tga'));task.destination_path=ROOT;task.destination_name='T_'+name
 task.automated=True;task.replace_existing=True;task.save=True
 assets.import_asset_tasks([task])
 tex=u.load_asset(ROOT+'/T_'+name);tex.set_editor_property('srgb',False)
 tex.set_editor_property('address_x',u.TextureAddress.TA_CLAMP);tex.set_editor_property('address_y',u.TextureAddress.TA_CLAMP)
 u.EditorAssetLibrary.save_loaded_asset(tex)

def mat(name):
 m=u.load_asset(ROOT+'/'+name)
 if m:lib.delete_all_material_expressions(m)
 else:m=assets.create_asset(name,ROOT,u.Material,u.MaterialFactoryNew())
 return m
def node(m,kind,**props):
 n=lib.create_material_expression(m,getattr(u,'MaterialExpression'+kind))
 for k,v in props.items():n.set_editor_property(k,v)
 return n
def link(a,b,pin='',out=''):
 assert lib.connect_material_expressions(a,out,b,pin), (a.get_name(),out,b.get_name(),pin)
def output(n,m,p):
 assert lib.connect_material_property(n,'',getattr(u.MaterialProperty,'MP_'+p)), p
def inp(k):
 i=u.CustomInput();i.set_editor_property('input_name',k);return i
def custom(m,code,inputs,typ=u.CustomMaterialOutputType.CMOT_FLOAT3):
 n=node(m,'Custom',code=code,output_type=typ,inputs=[inp(k) for k in inputs])
 for k,v in inputs.items():link(v,n,k)
 return n
def texture(m,path,uv,normal=False):
 n=node(m,'TextureSample',texture=u.load_asset(path),sampler_type=u.MaterialSamplerType.SAMPLERTYPE_NORMAL if normal else u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
 if not normal and n.get_editor_property('texture').get_editor_property('srgb'): n.set_editor_property('sampler_type',u.MaterialSamplerType.SAMPLERTYPE_COLOR)
 link(uv,n,'UVs');return n

m=mat('M_FounderLandscape');m.set_editor_property('used_with_static_lighting',True);wp=node(m,'WorldPosition')
uv=custom(m,'return P.xy / 560000.0 + 0.5;',{'P':wp},u.CustomMaterialOutputType.CMOT_FLOAT2)
detailuv=custom(m,'return P.xy / 600.0;',{'P':wp},u.CustomMaterialOutputType.CMOT_FLOAT2)
macro=texture(m,ROOT+'/T_FounderMacro',uv)
biome=texture(m,ROOT+'/T_FounderBiome',uv)
detail=texture(m,'/Game/Forest_village/Textures/T_ground_02/T_ground_02_D',detailuv)
normal=texture(m,'/Game/Forest_village/Textures/T_ground_02/T_ground_02_N',detailuv,True)
rock=texture(m,'/Game/Forest_village/Textures/T_rocks/T_small_modules_D',detailuv)
rocknormal=texture(m,'/Game/Forest_village/Textures/T_rocks/T_small_modules_N',detailuv,True)
color=custom(m,'return lerp(Macro*(0.48+Detail*1.1),Rock*float3(.66,.67,.61),Biome.r*.85);',{'Macro':macro,'Detail':detail,'Rock':rock,'Biome':biome})
normal=custom(m,'return normalize(lerp(Grass,Rock,Biome.r));',{'Grass':normal,'Rock':rocknormal,'Biome':biome})
inputs={'P':wp,'Color':color}
for i in range(9):
 inputs['R'+str(i)]=node(m,'VectorParameter',parameter_name='Region'+str(i),default_value=u.LinearColor(0,0,0,.94))
code='float nearD=1e12; float d[9]; float v[9];\n'
for i in range(9):code+=f'd[{i}]=distance(P.xy,R{i}.xy); v[{i}]=R{i}.a; nearD=min(nearD,d[{i}]);\n'
code+='float veil=0, total=0, worn=0; for(int i=0;i<9;i++){float w=1-smoothstep(0,12400,d[i]-nearD); veil+=w*v[i]; total+=w; if(v[i]<.9) worn=max(worn,1-smoothstep(2500,6000,d[i]));} Color=lerp(Color,float3(.18,.16,.11),worn*.55); return lerp(Color,float3(.10,.14,.18),veil/max(total,.001));'
# RGBA output is essential: vector parameters' default output excludes alpha.
veil=node(m,'Custom',code=code,output_type=u.CustomMaterialOutputType.CMOT_FLOAT3,inputs=[inp(k) for k in inputs])
for k,v in inputs.items():link(v,veil,k,'RGBA' if k.startswith('R') else '')
output(veil,m,'BASE_COLOR');output(normal,m,'NORMAL');output(node(m,'Constant',r=.88),m,'ROUGHNESS')
output(node(m,'Constant',r=.12),m,'SPECULAR')
assert not lib.recompile_material(m);u.EditorAssetLibrary.save_loaded_asset(m)

m=mat('M_FounderRoad');uv=node(m,'TextureCoordinate')
detail=texture(m,'/Game/Forest_village/Textures/T_ground_01/T_ground_01_D',uv)
color=custom(m,'return C*float3(.32,.27,.20);',{'C':detail});output(color,m,'BASE_COLOR')
output(node(m,'Constant',r=.96),m,'ROUGHNESS')
# Feather shoulder coverage to keep the route grounded rather than a hard ribbon.
alpha=custom(m,'return smoothstep(0,.40,U.x)*(1-smoothstep(.60,1,U.x))*.96;',{'U':uv},u.CustomMaterialOutputType.CMOT_FLOAT1)
output(alpha,m,'OPACITY');m.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT);m.set_editor_property('two_sided',True)
m.set_editor_property('translucency_lighting_mode',u.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
lib.recompile_material(m);u.EditorAssetLibrary.save_loaded_asset(m)

m=mat('M_FounderWater');wp=node(m,'WorldPosition');time=node(m,'Time')
uv=custom(m,'return P.xy/2400.0+float2(T*.008,T*.021);',{'P':wp,'T':time},u.CustomMaterialOutputType.CMOT_FLOAT2)
normal=texture(m,'/Game/Kingdom_Capital/Textures/T_water/T_wave_N',uv,True);output(normal,m,'NORMAL')
water=custom(m,'float v=.5+.5*sin(P.x/3200+sin(P.y/4200)); return lerp(float3(.014,.033,.025),float3(.035,.073,.058),v);',{'P':wp})
output(water,m,'BASE_COLOR')
output(node(m,'Constant',r=.18),m,'ROUGHNESS');output(node(m,'Constant',r=.5),m,'SPECULAR')
lib.recompile_material(m);u.EditorAssetLibrary.save_loaded_asset(m)
u.log('SOUL_TERRAIN_V2_MATERIALS_SAVED')

# Preserve the licensed branch textures while adapting coverage to the campaign
# camera. The donor's one-sided, heavily clipped leaf material loses its canopy
# at strategic distance. Only this owned instance is saved.
leaf=u.load_asset(ROOT+'/MI_CampaignPine')
if not leaf:leaf=assets.create_asset('MI_CampaignPine',ROOT,u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew())
lib.set_material_instance_parent(leaf,u.load_asset('/Game/Forest_village/Materials/MI_pine_tree'))
overrides=u.MaterialInstanceBasePropertyOverrides()
overrides.set_editor_property('override_two_sided',True);overrides.set_editor_property('two_sided',True)
overrides.set_editor_property('override_opacity_mask_clip_value',True);overrides.set_editor_property('opacity_mask_clip_value',.20)
leaf.set_editor_property('base_property_overrides',overrides)
lib.set_material_instance_scalar_parameter_value(leaf,'Opacity Mask',.05)
lib.update_material_instance(leaf)
u.EditorAssetLibrary.save_loaded_asset(leaf)

# Bounded campaign overrides: retain the licensed albedo/normal assets, with
# ordinary UV PBR shading suitable for miniature HISM architecture.
for name,base,tint in [
 ('M_CampaignWood','/Game/Forest_village/Textures/T_old_wood/T_old_wood',(.68,.53,.36)),
 ('M_CampaignMasonry','/Game/Kingdom_Capital/Textures/T_Building_Wall/T_building_wall',(.82,.78,.66))]:
 m=mat(name);m.set_editor_property('used_with_instanced_static_meshes',True);uv=node(m,'TextureCoordinate')
 albedo=texture(m,base+'_D',uv);normal=texture(m,base+'_N',uv,True)
 color=custom(m,'return max(C,float3(.07,.06,.05))*float3(%s);'%','.join(map(str,tint)),{'C':albedo})
 output(color,m,'BASE_COLOR');output(normal,m,'NORMAL')
 output(node(m,'Constant',r=.86),m,'ROUGHNESS')
 lib.recompile_material(m);u.EditorAssetLibrary.save_loaded_asset(m)
