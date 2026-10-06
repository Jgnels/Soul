"""Read-only entry-corridor survey using the existing battle's exact checks.

The earlier deployment survey did not test reserve entries. This survey uses
WorldStatic object traces, 65 cm sphere sweeps, 150 cm steps and the existing
110 cm step/0.7 normal limits from IsDirectGroundRouteClear. No collision or
combat rules are changed. Candidate origins still require a real battle.
"""
def inspect():
    import datetime
    import json
    import math
    from pathlib import Path
    import unreal
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    assert world.get_name() == 'L_DwarfHold_Authored'
    objects = [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1]  # Engine WorldStatic
    def vec(v): return [v.x, v.y, v.z]
    def corridor(origin, side, y):
        direction = 1 if side == 0 else -1
        start = unreal.Vector(origin[0]-direction*2000, origin[1]+y, origin[2]+100)
        previous = None
        points = []
        for step in range(4):
            point = start + unreal.Vector(direction*350*step/3, 0, 0)
            hit = unreal.SystemLibrary.line_trace_single_for_objects(world,
                point+unreal.Vector(0,0,500), point-unreal.Vector(0,0,700),
                objects, False, [], unreal.DrawDebugTrace.NONE)
            data = hit.to_dict() if hit else {}
            if not data.get('blocking_hit'):
                return dict(clear=False, reason='no WorldStatic ground', step=step, point=vec(point))
            at = data['impact_point'] + unreal.Vector(0,0,110)
            points.append(vec(data['impact_point']))
            if data['impact_normal'].z < .7:
                return dict(clear=False, reason='ground normal', step=step, normal=vec(data['impact_normal']), points=points)
            if previous:
                if abs(at.z-previous.z)>110:
                    return dict(clear=False, reason='step exceeds 110 cm', step=step, points=points)
                obstacle = unreal.SystemLibrary.sphere_trace_single_for_objects(world,
                    previous, at, 65, objects, False, [], unreal.DrawDebugTrace.NONE)
                obstruction = obstacle.to_dict() if obstacle else {}
                if obstruction.get('blocking_hit'):
                    component = obstruction.get('hit_component')
                    return dict(clear=False, reason='sphere obstruction', step=step, points=points,
                        component=component.get_path_name() if component else None)
            previous = at
        return dict(clear=True, points=points)
    def deployment(origin):
        points = []
        for side in (0,1):
            direction = 1 if side == 0 else -1
            for rearward,lateral,spacing in ((0,250,135),(620,250,175),(150,-650,350)):
                for i in range(5):
                    row,col=divmod(i,4)
                    members=min(4,5-row*4)
                    x=origin[0]-direction*1500-direction*rearward-direction*row*spacing
                    y=origin[1]+direction*lateral+direction*(col-(members-1)*.5)*spacing
                    hit=unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x,y,origin[2]+8100),
                        unreal.Vector(x,y,origin[2]-11900), unreal.TraceTypeQuery.ECC_VISIBILITY,
                        False, [], unreal.DrawDebugTrace.NONE)
                    data=hit.to_dict() if hit else {}
                    if not data.get('blocking_hit'): return dict(misses=True)
                    points.append((data['impact_point'].z,math.degrees(math.acos(max(-1,min(1,data['impact_normal'].z))))))
        return dict(misses=False, max_slope=max(s for z,s in points),
            height_range=max(z for z,s in points)-min(z for z,s in points))
    rows=[]
    for z in globals().get('ENTRY_Z_VALUES', [0]):
        for x in range(-16000,-9999,500):
            for y in range(-2000,3001,500):
                origin=[x,y,z]
                entries=[dict(side=side,y_offset=offset,**corridor(origin,side,offset))
                    for side in (0,1) for offset in (-1400,0,1400)]
                counts=[sum(e['clear'] for e in entries if e['side']==side) for side in (0,1)]
                rows.append(dict(origin=origin, clear_entries=counts, entries=entries, deployment=deployment(origin)))
    rows.sort(key=lambda r:(r['deployment']['misses'], -min(r['clear_entries']),
        r['deployment'].get('max_slope',999),r['deployment'].get('height_range',99999)))
    root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
    suffix=globals().get('ENTRY_SURVEY_SUFFIX', '')
    assert all(c.isalnum() or c=='-' for c in suffix)
    out=root/'Evidence/SettlementEnvironmentPlan-20261005/Continuation-20261006'/('dwarven-reserve-entry-survey'+suffix+'.json')
    assert not out.exists()
    out.write_text(json.dumps(dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        source='existing ChooseReinforcementAnchor and IsDirectGroundRouteClear geometry; no enemy-distance scoring in this static survey',
        world=world.get_path_name(), object_query=str(objects[0]), candidates=rows),indent=2)+'\n')
    for row in rows[:8]: print({k:v for k,v in row.items() if k!='entries'})
    print('ORIGINAL_XY',[r for r in rows if r['origin'][:2]==[-13500,0]])
    print('SOUL_RESERVE_ENTRY_SURVEY',str(out))

inspect()
