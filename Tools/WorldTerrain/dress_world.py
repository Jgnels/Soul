"""Execute in the live Soul editor. Save only /Game/SoulCampaignWorld assets.

Materials are created first by Nwiro create_material. Imported macro masks are
optional: the analytic fallback is recorded explicitly in the material receipt.
"""
import json
import math
from pathlib import Path
import unreal as u

ROOT = '/Game/SoulCampaignWorld'
DATA = Path('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview/WorldTerrain')
editor = u.get_editor_subsystem(u.EditorActorSubsystem)
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert world.get_path_name().startswith(ROOT+'/L_SoulWorld'), world.get_path_name()
lib = u.MaterialEditingLibrary

def node(m, kind, **props):
    n = lib.create_material_expression(m, getattr(u, 'MaterialExpression'+kind))
    for key, value in props.items(): n.set_editor_property(key, value)
    return n

def link(a, b, pin='', out=''):
    assert lib.connect_material_expressions(a, out, b, pin), (a, b, pin)

def custom(m, code, inputs, dim=3):
    names = []
    for key in inputs:
        item = u.CustomInput(); item.set_editor_property('input_name', key); names.append(item)
    n = node(m, 'Custom', code=code, inputs=names,
             output_type=getattr(u.CustomMaterialOutputType, 'CMOT_FLOAT'+str(dim)))
    for key, value in inputs.items(): link(value, n, key)
    return n

def output(n, m, name):
    assert lib.connect_material_property(n, '', getattr(u.MaterialProperty, 'MP_'+name))

def texture(m, path, uv, normal=False):
    asset = u.load_asset(path); assert asset, path
    sampler = u.MaterialSamplerType.SAMPLERTYPE_NORMAL if normal else (
        u.MaterialSamplerType.SAMPLERTYPE_COLOR if asset.get_editor_property('srgb')
        else u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    n = node(m, 'TextureSample', texture=asset, sampler_type=sampler)
    link(uv, n, 'UVs')
    return n

def material(name):
    m = u.load_asset(ROOT+'/'+name); assert m, name
    lib.delete_all_material_expressions(m)
    return m

def save(m):
    errors = lib.recompile_material(m)
    assert not errors, errors
    assert u.EditorAssetLibrary.save_loaded_asset(m)

m = material('M_WorldGround')
m.set_editor_property('used_with_static_lighting', True)
pos = node(m, 'WorldPosition'); normal = node(m, 'VertexNormalWS')
camera = node(m,'CameraPositionWS')
distance = custom(m,'return distance(P,C);',{'P':pos,'C':camera},1)
uv = custom(m, 'return P.xy/450.0;', {'P':pos}, 2)
coarse_uv = custom(m,'return float2(P.x*.81-P.y*.586,P.x*.586+P.y*.81)/1900.0+float2(.31,.67);',{'P':pos},2)
macro_uv = custom(m, 'return (P.xy+508000.0)/1016000.0;', {'P':pos}, 2)
grass = texture(m, '/Game/LandscapePackOne/Textures/Landscape/Grass/01/T_Grass_01', uv)
coarse_grass = texture(m,'/Game/LandscapePackOne/Textures/Landscape/Grass/01/T_Grass_01',coarse_uv)
rock = texture(m, '/Game/LandscapePackOne/Textures/Landscape/Rock/01/T_Rock_01', coarse_uv)
grass_n = texture(m, '/Game/LandscapePackOne/Textures/Landscape/Grass/01/T_Grass_01_NRM', uv, True)
rock_n = texture(m, '/Game/LandscapePackOne/Textures/Landscape/Rock/01/T_Rock_01_NRM', coarse_uv, True)
biome = u.load_asset(ROOT+'/T_WorldBiome')
regions = u.load_asset(ROOT+'/T_WorldRegions')
mask = custom(m, '''
float2 q=float2(P.x/1000+500,475-P.y/1000);
float w=sin(q.x*.035+sin(q.y*.027))*11+sin(q.y*.081)*4;
float ash=smoothstep(580,700,q.y+w)*smoothstep(480,590,q.x-w);
float bad=smoothstep(555,710,q.x+w)*(1-smoothstep(600,715,q.y+w));
float forest=(1-smoothstep(420,565,q.x+w))*smoothstep(530,665,q.y-w);
float snow=smoothstep(42000,65000,P.z+(1-smoothstep(190,335,q.y))*18500);
return float4(bad,ash,forest,snow);''', {'P':pos}, 4)
if biome and regions:
    mask = custom(m, 'return float4(R.r,R.b,R.g,B.g);',
                  {'R':texture(m,ROOT+'/T_WorldRegions',macro_uv),
                   'B':texture(m,ROOT+'/T_WorldBiome',macro_uv)},4)
inputs = {'P':pos, 'N':normal, 'G':grass, 'G2':coarse_grass, 'R':rock, 'B':mask,'D':distance,
          'Detail':texture(m,ROOT+'/T_WorldDetail',macro_uv),
          'Field':texture(m,ROOT+'/T_WorldCultivation',macro_uv)}
macro = u.load_asset(ROOT+'/T_WorldMacro')
if macro:
    inputs['M'] = texture(m, ROOT+'/T_WorldMacro', macro_uv)
    base = '''float micro=1-smoothstep(18000,70000,D);
float3 linearMacro=lerp(M/12.92,pow((M+.055)/1.055,2.4),step(.04045,M));
float breakup=.79+Detail.r*.17+Detail.g*.22+Detail.b*.20;
float3 ground=linearMacro*breakup*(.94+(G*.55+G2*.45-.35)*.30*micro);'''
else:
    base = '''float3 grassland=G*float3(.43,.49,.30);
float3 ground=lerp(grassland,R*float3(.54,.34,.22),B.r);
ground=lerp(ground,R*float3(.20,.19,.18),B.g);
ground=lerp(ground,G*float3(.27,.35,.22),B.b);'''
profile=json.loads((DATA/'presentation.json').read_text())
field_code='''
float3 crops=lerp(float3(.100,.100,.044),float3(.071,.105,.035),saturate(Field.g*2));
crops=lerp(crops,float3(.118,.108,.050),saturate(Field.g*2-1));
float angle=Field.b*3.14159265;
float rowFade=1-smoothstep(16000,45000,D);
float rows=1+.045*sin(dot(P.xy,float2(-sin(angle),cos(angle)))/95.0)*rowFade;
ground=lerp(ground,crops*rows*(.88+Detail.r*.24),Field.r*.65);
'''
color = custom(m, base+'''
float rockWeight=smoothstep(.06,.30,1-saturate(N.z));
float3 rockDetail=lerp(float3(.42,.42,.42),R,1-smoothstep(30000,100000,D));
float3 stone=rockDetail*lerp(lerp(float3(.60,.61,.58),float3(.57,.40,.28),B.r),float3(.32,.28,.25),B.g)*(.81+Detail.g*.30+Detail.r*.12);
ground=lerp(ground,stone,rockWeight*.68);
float strata=.95+.05*sin(P.z/760+sin(P.x/6300)+sin(P.y/4300));
ground*=strata;
ground=lerp(ground,float3(.49,.54,.58)*(0.88+Detail.g*.13+Detail.r*.07),B.a*saturate(N.z*1.4-.2));
float shore=(1-smoothstep(80,1000,P.z))*(1-B.a);
ground=lerp(ground,rockDetail*float3(.30,.27,.21)*(.90+Detail.r*.20),shore*.65);
'''+field_code+'return ground;', inputs)
output(color, m, 'BASE_COLOR')
output(custom(m, 'float3 n=lerp(G,R,smoothstep(.05,.28,1-saturate(N.z))); return normalize(lerp(float3(0,0,1),n,.5*(1-smoothstep(12000,45000,D))));',
              {'G':grass_n,'R':rock_n,'N':normal,'D':distance}), m, 'NORMAL')
output(node(m,'Constant',r=.89),m,'ROUGHNESS')
output(node(m,'Constant',r=.13),m,'SPECULAR')
save(m)
lands = [a for a in editor.get_all_level_actors() if isinstance(a,u.LandscapeProxy)]
assert lands
for land in lands:
    land.call_method('EditorSetLandscapeMaterial',args=(m,))
    land.set_editor_property('use_dynamic_material_instance',True)

water = material('M_WorldWater')
p = node(water,'WorldPosition'); t = node(water,'Time')
uv = custom(water,'return P.xy/1800+float2(T*.009,T*.006);',{'P':p,'T':t},2)
n = texture(water,'/Game/Kingdom_Capital/Textures/T_water/T_wave_N',uv,True)
output(custom(water,'return normalize(float3(N.xy*.55,N.z));',{'N':n}),water,'NORMAL')
output(custom(water,'return lerp(float3(.010,.026,.030),float3(.021,.049,.050),saturate(N.x+N.y+.4));',{'N':n}),water,'BASE_COLOR')
output(node(water,'Constant',r=.28),water,'ROUGHNESS')
output(node(water,'Constant',r=.42),water,'SPECULAR')
save(water)

road = material('M_WorldRoad')
p = node(road,'WorldPosition'); uv = custom(road,'return P.xy/1000;',{'P':p},2)
dirt = texture(road,'/Game/Forest_village/Textures/T_ground_01/T_ground_01_D',uv)
output(custom(road,'return C*float3(.38,.31,.22);',{'C':dirt}),road,'BASE_COLOR')
output(node(road,'Constant',r=.95),road,'ROUGHNESS')
edge = node(road,'TextureCoordinate')
output(custom(road,'return smoothstep(0,.27,U.x)*(1-smoothstep(.73,1,U.x))*.94;',{'U':edge},1),road,'OPACITY')
road.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT)
road.set_editor_property('two_sided',True)
road.set_editor_property('translucency_lighting_mode',u.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
save(road)

masonry=material('M_WorldMasonry')
masonry.set_editor_property('used_with_instanced_static_meshes',True)
uv=node(masonry,'TextureCoordinate')
albedo=texture(masonry,'/Game/Medieval_Megapack/Textures/T_BrickWall_03_basecolor',uv)
brick_normal=texture(masonry,'/Game/Medieval_Megapack/Textures/T_BrickWall_03_normal',uv,True)
tint=node(masonry,'VectorParameter',parameter_name='Tint',default_value=u.LinearColor(1,1,1,1))
output(custom(masonry,'return C*Tint;',{'C':albedo,'Tint':tint}),masonry,'BASE_COLOR')
output(brick_normal,masonry,'NORMAL')
output(node(masonry,'Constant',r=.84),masonry,'ROUGHNESS')
output(node(masonry,'Constant',r=.18),masonry,'SPECULAR')
save(masonry)

# Only the owned ocean surface belongs in the persistent map. Runtime retains
# Soul lighting/weather and generates river surfaces and strategic proxies.
for actor in editor.get_all_level_actors():
    if actor.get_actor_label().startswith('SoulWorld_'): editor.destroy_actor(actor)
sea = editor.spawn_actor_from_class(u.StaticMeshActor,u.Vector(0,0,0))
sea.set_actor_label('SoulWorld_Ocean')
sea.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/Plane'))
sea.static_mesh_component.set_material(0,water)
sea.set_actor_scale3d(u.Vector(100000,100000,1))
sea.static_mesh_component.set_cast_shadow(False)
sea.set_actor_enable_collision(False)
assert levels.save_current_level()
receipt={'map':world.get_path_name(),'landscape_count':len(lands),
         'macro_texture_used':bool(macro),'authored_biome_masks_used':bool(biome and regions),
         'macro_palette_space':'display sRGB stored in linear HDR texture; explicit IEC shader decode; biome channels linear','field_parcels':len(profile.get('fields',[])),
         'surface_breakup_scales_m':[24,105,470], 'field_shader':'one baked mask sample with near-only rows; no per-parcel shader loops',
         'materials':[ROOT+'/'+x for x in ['M_WorldGround','M_WorldWater','M_WorldRoad','M_WorldMasonry']],
         'donor_assets_saved':False}
(DATA/'material-receipt.json').write_text(json.dumps(receipt,indent=2))
print('SOUL_WORLD_MATERIALS_SAVED '+json.dumps(receipt))
