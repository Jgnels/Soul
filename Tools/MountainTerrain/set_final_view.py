import unreal as u
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert world.get_path_name().startswith('/Game/SoulCampaignMountain/L_mountain_campaign.')
a=u.get_editor_subsystem(u.EditorActorSubsystem)
cam=next(x for x in a.get_all_level_actors() if x.get_actor_label()=='Study_ReviewCamera')
rot=u.Rotator(pitch=-38,yaw=-90,roll=0);pos=u.Vector(10000,20000,1000)-rot.get_forward_vector()*90000
cam.set_actor_location_and_rotation(pos,rot,False,False)
u.EditorLevelLibrary.set_level_viewport_camera_info(pos,rot)
u.EditorLevelLibrary.pilot_level_actor(cam)
u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
u.SystemLibrary.execute_console_command(world,'t.MaxFPS 10')
print('Final map saved with human-plains review camera')
