import unreal as u,math,time,json,traceback
from pathlib import Path
ROOT=Path('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview')
ASSET='/Game/SoulCampaignMountain';lib=u.MaterialEditingLibrary
actors=u.get_editor_subsystem(u.EditorActorSubsystem);levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert world.get_path_name().startswith(ASSET+'/')
land=next(a for a in actors.get_all_level_actors() if isinstance(a,u.Landscape))
for a in actors.get_all_level_actors():
    if a.get_actor_label().startswith('Study_'):actors.destroy_actor(a)
def mat(name):
    m=u.load_asset(ASSET+'/'+name)
    assert m,name
    lib.delete_all_material_expressions(m)
    return m
def node(m,kind,**props):
    n=lib.create_material_expression(m,getattr(u,'MaterialExpression'+kind))
    for k,v in props.items():n.set_editor_property(k,v)
    return n
def link(a,b,pin='',out=''):
    assert lib.connect_material_expressions(a,out,b,pin)
def output(n,m,p):assert lib.connect_material_property(n,'',getattr(u.MaterialProperty,'MP_'+p))
def custom(m,code,inputs,typ=u.CustomMaterialOutputType.CMOT_FLOAT3):
    ins=[]
    for k in inputs:
        i=u.CustomInput();i.set_editor_property('input_name',k);ins.append(i)
    n=node(m,'Custom',code=code,output_type=typ,inputs=ins)
    for k,v in inputs.items():link(v,n,k)
    return n
def tex(m,path,uv,normal=False):
    t=u.load_asset(path);assert t,path
    n=node(m,'TextureSample',texture=t,sampler_type=u.MaterialSamplerType.SAMPLERTYPE_NORMAL if normal else (u.MaterialSamplerType.SAMPLERTYPE_COLOR if t.get_editor_property('srgb') else u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR))
    link(uv,n,'UVs');return n
for old,new in [('M_CampaignGround','M_WaterfrontGround'),('M_CampaignWater','M_WaterfrontWater')]:
    if not u.EditorAssetLibrary.does_asset_exist(ASSET+'/'+new):assert u.EditorAssetLibrary.duplicate_asset(ASSET+'/'+old,ASSET+'/'+new)
m=mat('M_WaterfrontGround')
pos=node(m,'WorldPosition');normal=node(m,'VertexNormalWS')
uv=custom(m,'return P.xy/3700;',{'P':pos},u.CustomMaterialOutputType.CMOT_FLOAT2)
grass=tex(m,'/Game/LandscapePackOne/Textures/Landscape/Grass/01/T_Grass_01',uv)
rock=tex(m,'/Game/LandscapePackOne/Textures/Landscape/Rock/01/T_Rock_01',uv)
color=custom(m,'float s=1-saturate(N.z); float r=smoothstep(.025,.18,s); float variation=.91+.10*sin(P.x/9700+sin(P.y/5700))+.06*sin(P.y/3900+P.x/7100); float3 g=lerp(float3(.105,.145,.065),G*.57,.16); float3 rock=lerp(float3(.22,.205,.18),R*.55,.45); float3 soil=lerp(g,rock,r)*variation; float shore=1-smoothstep(30,180,P.z); return lerp(soil,float3(.20,.18,.125),shore*.65);',{'P':pos,'N':normal,'G':grass,'R':rock})
mapuv=custom(m,'return (P.xy+75000)/150000;',{'P':pos},u.CustomMaterialOutputType.CMOT_FLOAT2)
biome=custom(m,'float2 q=(P.xy+75000)/150000; float dist=10; bool inside=false;{float2 a=float2(-0.08,0.075),b=float2(0.075,0.09); float2 d=b-a; float t=saturate(dot(q-a,d)/dot(d,d)); dist=min(dist,length(q-a-t*d)); if((a.y>q.y)!=(b.y>q.y)){if(q.x<(b.x-a.x)*(q.y-a.y)/(b.y-a.y)+a.x)inside=!inside;}}{float2 a=float2(0.075,0.09),b=float2(0.16,0.13); float2 d=b-a; float t=saturate(dot(q-a,d)/dot(d,d)); dist=min(dist,length(q-a-t*d)); if((a.y>q.y)!=(b.y>q.y)){if(q.x<(b.x-a.x)*(q.y-a.y)/(b.y-a.y)+a.x)inside=!inside;}}{float2 a=float2(0.16,0.13),b=float2(0.2,0.2); float2 d=b-a; float t=saturate(dot(q-a,d)/dot(d,d)); dist=min(dist,length(q-a-t*d)); if((a.y>q.y)!=(b.y>q.y)){if(q.x<(b.x-a.x)*(q.y-a.y)/(b.y-a.y)+a.x)inside=!inside;}}{float2 a=float2(0.2,0.2),b=float2(0.25,0.285); float2 d=b-a; float t=saturate(dot(q-a,d)/dot(d,d)); dist=min(dist,length(q-a-t*d)); if((a.y>q.y)!=(b.y>q.y)){if(q.x<(b.x-a.x)*(q.y-a.y)/(b.y-a.y)+a.x)inside=!inside;}}{float2 a=float2(0.25,0.285),b=float2(0.18,0.325); float2 d=b-a; float t=saturate(dot(q-a,d)/dot(d,d)); dist=min(dist,length(q-a-t*d)); if((a.y>q.y)!=(b.y>q.y)){if(q.x<(b.x-a.x)*(q.y-a.y)/(b.y-a.y)+a.x)inside=!inside;}}{float2 a=float2(0.18,0.325),b=float2(0.08,0.355); float2 d=b-a; float t=saturate(dot(q-a,d)/dot(d,d)); dist=min(dist,length(q-a-t*d)); if((a.y>q.y)!=(b.y>q.y)){if(q.x<(b.x-a.x)*(q.y-a.y)/(b.y-a.y)+a.x)inside=!inside;}}{float2 a=float2(0.08,0.355),b=float2(-0.08,0.34); float2 d=b-a; float t=saturate(dot(q-a,d)/dot(d,d)); dist=min(dist,length(q-a-t*d)); if((a.y>q.y)!=(b.y>q.y)){if(q.x<(b.x-a.x)*(q.y-a.y)/(b.y-a.y)+a.x)inside=!inside;}}{float2 a=float2(-0.08,0.34),b=float2(-0.08,0.075); float2 d=b-a; float t=saturate(dot(q-a,d)/dot(d,d)); dist=min(dist,length(q-a-t*d)); if((a.y>q.y)!=(b.y>q.y)){if(q.x<(b.x-a.x)*(q.y-a.y)/(b.y-a.y)+a.x)inside=!inside;}}float w=smoothstep(0,85.0/1500.0,dist)*(inside?1:0)*smoothstep(0,400,P.z); return float3(w,(1-smoothstep(1100,1700,P.z))*smoothstep(.45,.9,w),0);',{'P':pos})
biome=custom(m,'float2 q=(P.xy+75000)/150000; float dist=10; bool inside=false;{float2 a=float2(0.115,0.265),b=float2(0.245,0.265); float2 d=b-a; float t=saturate(dot(q-a,d)/dot(d,d)); dist=min(dist,length(q-a-t*d)); if((a.y>q.y)!=(b.y>q.y)){if(q.x<(b.x-a.x)*(q.y-a.y)/(b.y-a.y)+a.x)inside=!inside;}}{float2 a=float2(0.245,0.265),b=float2(0.28,0.295); float2 d=b-a; float t=saturate(dot(q-a,d)/dot(d,d)); dist=min(dist,length(q-a-t*d)); if((a.y>q.y)!=(b.y>q.y)){if(q.x<(b.x-a.x)*(q.y-a.y)/(b.y-a.y)+a.x)inside=!inside;}}{float2 a=float2(0.28,0.295),b=float2(0.26,0.326); float2 d=b-a; float t=saturate(dot(q-a,d)/dot(d,d)); dist=min(dist,length(q-a-t*d)); if((a.y>q.y)!=(b.y>q.y)){if(q.x<(b.x-a.x)*(q.y-a.y)/(b.y-a.y)+a.x)inside=!inside;}}{float2 a=float2(0.26,0.326),b=float2(0.19,0.346); float2 d=b-a; float t=saturate(dot(q-a,d)/dot(d,d)); dist=min(dist,length(q-a-t*d)); if((a.y>q.y)!=(b.y>q.y)){if(q.x<(b.x-a.x)*(q.y-a.y)/(b.y-a.y)+a.x)inside=!inside;}}{float2 a=float2(0.19,0.346),b=float2(0.125,0.335); float2 d=b-a; float t=saturate(dot(q-a,d)/dot(d,d)); dist=min(dist,length(q-a-t*d)); if((a.y>q.y)!=(b.y>q.y)){if(q.x<(b.x-a.x)*(q.y-a.y)/(b.y-a.y)+a.x)inside=!inside;}}{float2 a=float2(0.125,0.335),b=float2(0.115,0.265); float2 d=b-a; float t=saturate(dot(q-a,d)/dot(d,d)); dist=min(dist,length(q-a-t*d)); if((a.y>q.y)!=(b.y>q.y)){if(q.x<(b.x-a.x)*(q.y-a.y)/(b.y-a.y)+a.x)inside=!inside;}}float w=smoothstep(0,25.0/1500.0,dist)*(inside?1:0); return float3(max(B.r,w),B.g*(1-w),0);',{'P':pos,'B':biome})
mesa=tex(m,'/Game/LandscapePackTwo/Textures/Landscape/Rock/02/T_CanyonRock_02',uv)
color=custom(m,'float w=saturate(B.r); float3 ash=lerp(float3(.085,.065,.055),M*float3(.30,.22,.18),.65); float ledge=.88+.12*sin(P.z/95); return lerp(C,ash*ledge,w);',{'C':color,'B':biome,'M':mesa,'P':pos})
glow=custom(m,'float2 q=P.xy/850; float2 cell=floor(q); float f1=10,f2=10; for(int y=-1;y<=1;y++){for(int x=-1;x<=1;x++){float2 c=cell+float2(x,y); float2 h=frac(sin(float2(dot(c,float2(127.1,311.7)),dot(c,float2(269.5,183.3))))*43758.5453); float d=length(c+h-q); if(d<f1){f2=f1;f1=d;}else{f2=min(f2,d);}}} float crack=1-smoothstep(.018,.06,f2-f1); return float3(1.2,.075,.003)*crack*B.g;',{'B':biome,'P':pos})
output(glow,m,'EMISSIVE_COLOR')
output(color,m,'BASE_COLOR');output(node(m,'Constant',r=.88),m,'ROUGHNESS');output(node(m,'Constant',r=.1),m,'SPECULAR')
lib.recompile_material(m);u.EditorAssetLibrary.save_loaded_asset(m)
land.call_method('EditorSetLandscapeMaterial',args=(m,))
water=mat('M_WaterfrontWater');p=node(water,'WorldPosition');t=node(water,'Time')
c=custom(water,'return float3(.018,.065,.078);',{'P':p})
output(c,water,'BASE_COLOR');output(node(water,'Constant',r=.28),water,'ROUGHNESS');output(node(water,'Constant',r=.45),water,'SPECULAR')
lib.recompile_material(water);u.EditorAssetLibrary.save_loaded_asset(water)
def spawn(cls,label,loc=(0,0,0)):
    a=actors.spawn_actor_from_class(cls,u.Vector(*loc));a.set_actor_label('Study_'+label);return a
sea=spawn(u.StaticMeshActor,'Water')
sea.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/Plane'));sea.static_mesh_component.set_material(0,water)
sea.set_actor_scale3d(u.Vector(1500,1500,1));sea.static_mesh_component.set_cast_shadow(False);sea.set_actor_enable_collision(False)
sun=spawn(u.DirectionalLight,'Sun',(0,0,20000));sun.set_actor_rotation(u.Rotator(pitch=-42,yaw=-35,roll=0),False)
sun.light_component.set_intensity(3.2);sun.light_component.set_light_color(u.LinearColor(1,.92,.80))
sky=spawn(u.SkyLight,'Ambient',(0,0,10000));sc=sky.get_component_by_class(u.SkyLightComponent)
sc.set_editor_property('source_type',u.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
sc.set_cubemap(u.load_asset('/Engine/MapTemplates/Sky/DaylightAmbientCubemap'));sc.set_intensity(.7);sc.recapture_sky()
cam=spawn(u.CameraActor,'ReviewCamera');cc=cam.get_component_by_class(u.CameraComponent);cc.set_field_of_view(50)
pp=cc.get_editor_property('post_process_settings')
for k,v in [('override_auto_exposure_min_brightness',True),('override_auto_exposure_max_brightness',True),('auto_exposure_min_brightness',1),('auto_exposure_max_brightness',1),('override_auto_exposure_bias',True),('auto_exposure_bias',.5),('override_motion_blur_amount',True),('motion_blur_amount',0)]:pp.set_editor_property(k,v)
cc.set_editor_property('post_process_settings',pp)
for cmd in ['t.MaxFPS 20','r.Streaming.PoolSize 1600','grass.Enable 0','r.MotionBlurQuality 0']:
    u.SystemLibrary.execute_console_command(world,cmd)
levels.save_current_level()
print('MOUNTAIN_STUDY_DRESSED '+world.get_path_name())
