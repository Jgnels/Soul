"""Read the owned Human wrapper without retaining actor/world references.

Execute in a temporary live-Python namespace after loading the owned map.
Reports native instance readiness and actual camera-component transforms.
Does not load additional maps, request instance edits or save any package.
"""
def inspect():
    import collections
    import datetime
    import json
    from pathlib import Path
    import unreal

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    package = '/Game/Soul/Maps/Settlements/L_HumanCapital_Authored'
    assert world.get_path_name().split('.')[0] == package
    actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
    def vec(v): return [v.x, v.y, v.z]
    instances, cameras = [], []
    for actor in actors:
        if isinstance(actor, unreal.LevelInstance):
            # Reading the soft world_asset property through Python can load a
            # second source UWorld. Inspect the already loaded instance only.
            loaded_level = actor.get_loaded_level()
            instances.append(dict(path=actor.get_path_name(), label=actor.get_actor_label(),
                loaded_level=loaded_level.get_path_name() if loaded_level else None,
                loaded=actor.is_loaded(), location=vec(actor.get_actor_location()),
                behavior=str(actor.get_editor_property('desired_runtime_behavior'))))
        for camera in actor.get_components_by_class(unreal.CameraComponent):
            cameras.append(dict(actor=actor.get_path_name(), label=actor.get_actor_label(),
                location=vec(camera.get_world_location()), rotation=str(camera.get_world_rotation()),
                fov=camera.get_editor_property('field_of_view')))
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ')
    root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
    out = root/'Evidence/SettlementEnvironmentPlan-20261005/Continuation-20261006'/('human-readiness-'+stamp+'.json')
    data = dict(utc=stamp, map=package, actor_count=len(actors),
        classes=dict(collections.Counter(a.get_class().get_name() for a in actors)),
        instances=instances, cameras=cameras,
        dirty_maps=[p.get_path_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()],
        dirty_content=[p.get_path_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()])
    out.write_text(json.dumps(data, indent=2)+'\n')
    print('SOUL_HUMAN_READINESS', len(actors), 'instances', len(instances),
        'loaded', sum(r['loaded'] for r in instances), str(out))

inspect()
