"""Live editor: restrained lighting/water and measured scale objects only."""
import unreal as u,json,math
from pathlib import Path
ROOT=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929')
out=ROOT/'Evidence/ProductionWorldComposition-20261007'
a=u.get_editor_subsystem(u.EditorActorSubsystem);w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
lands=[x for x in a.get_all_level_actors() if isinstance(x,u.Landscape)]
assert len(lands)==1 and lands[0].get_actor_label()=='Soul_Composition_3500_r1'
assert not any(x.get_actor_label()=='Composition_Sun' for x in a.get_all_level_actors())
sun=a.spawn_actor_from_class(u.DirectionalLight,u.Vector(0,0,100000));sun.set_actor_label('Composition_Sun');sun.set_actor_rotation(u.Rotator(pitch=-38,yaw=-35,roll=0),False)
lc=sun.get_component_by_class(u.DirectionalLightComponent);lc.set_mobility(u.ComponentMobility.MOVABLE);lc.set_intensity(3)
sky=a.spawn_actor_from_class(u.SkyLight,u.Vector(0,0,10000));sky.set_actor_label('Composition_Sky');sc=sky.get_component_by_class(u.SkyLightComponent);sc.set_mobility(u.ComponentMobility.MOVABLE);sc.set_intensity(.8)
atm=a.spawn_actor_from_class(u.SkyAtmosphere,u.Vector(0,0,0));atm.set_actor_label('Composition_Atmosphere')
water=a.spawn_actor_from_class(u.StaticMeshActor,u.Vector(0,0,0));water.set_actor_label('Composition_Ocean_NativeDatum');water.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/Plane'));water.static_mesh_component.set_material(0,u.load_asset('/Game/SoulCampaignComposition/MI_Composition_Water_r1'));water.set_actor_scale3d(u.Vector(3500,3500,1));water.set_actor_enable_collision(False);water.static_mesh_component.set_cast_shadow(False)
cam=a.spawn_actor_from_class(u.CameraActor,u.Vector(0,0,0));cam.set_actor_label('Composition_ReviewCamera');cc=cam.get_component_by_class(u.CameraComponent);cc.set_field_of_view(50);pp=cc.get_editor_property('post_process_settings')
for k,v in [('override_auto_exposure_min_brightness',True),('override_auto_exposure_max_brightness',True),('auto_exposure_min_brightness',1),('auto_exposure_max_brightness',1),('override_auto_exposure_bias',True),('auto_exposure_bias',1.15),('override_motion_blur_amount',True),('motion_blur_amount',0)]:pp.set_editor_property(k,v)
cc.set_editor_property('post_process_settings',pp)
for cmd in ['t.MaxFPS 12','r.Streaming.PoolSize 1600','grass.Enable 0','ShowFlag.Fog 0','r.MotionBlurQuality 0']:u.SystemLibrary.execute_console_command(w,cmd)
rot=u.Rotator(pitch=-65,yaw=-90,roll=0);cam.set_actor_location_and_rotation(u.Vector(0,0,5000)-rot.get_forward_vector()*780000,rot,False,False);u.EditorLevelLibrary.pilot_level_actor(cam)
sc.recapture_sky()
assert u.EditorLoadingAndSavingUtils.save_map(w,'/Game/SoulCampaignComposition/L_Composition_3500_r1')
print('FOUNDATION_LIGHTING_SAVED')
