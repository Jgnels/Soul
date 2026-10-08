"""Seat the existing Dwarf miniature on owned masonry, never edit Landscape."""
import unreal as u,json,math
from pathlib import Path
R=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');E=R/'Evidence/MapFinalPolish-20261007'
a=u.get_editor_subsystem(u.EditorActorSubsystem);w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert w.get_path_name().startswith('/Game/SoulCampaignComposition/L_Composition_3500_r2')
assert not any(x.get_actor_label().startswith('FinalPolish_Dwarf_') for x in a.get_all_level_actors())
hold=next(x for x in a.get_all_level_actors() if x.get_actor_label()=='Composition_Scale_dwarf_hold');before=hold.get_actor_location();c,e=hold.get_actor_bounds(False)
ignore=[x for x in a.get_all_level_actors() if not isinstance(x,u.Landscape)]
def ground(x,y):
 h=u.SystemLibrary.line_trace_single(w,u.Vector(x+.1,y+.1,100000),u.Vector(x+.1,y+.1,-100000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE);d=h.to_dict() if h else {};assert d.get('blocking_hit');return d['impact_point'].z
rows=[]
def boxmesh(name,path,center,dimensions,yaw=0):
 m=u.load_asset(path);assert m;b=m.get_bounding_box();size=b.max-b.min;scale=u.Vector(*(dimensions[i]/getattr(size,'xyz'[i]) for i in range(3)));rot=u.Rotator(pitch=0,yaw=yaw,roll=0)
 offset=rot.quaternion().rotate_vector((b.min+b.max)*.5*scale)
 actor=a.spawn_actor_from_class(u.StaticMeshActor,u.Vector(*center)-offset);actor.set_actor_label('FinalPolish_Dwarf_'+name);actor.static_mesh_component.set_static_mesh(m);actor.set_actor_scale3d(scale);actor.set_actor_rotation(rot,False);actor.set_actor_enable_collision(False)
 rows.append(dict(actor=actor.get_actor_label(),mesh=m.get_path_name(),center_cm=center,dimensions_cm=dimensions,yaw=yaw));return actor
# A rigid 3.25 m presentation lift clears the measured 39.38 m high-ground corner.
# No XY/scale change. Architectural support carries the lift rather than terrain.
lift=325.;hold.set_actor_location(before+u.Vector(0,0,lift),False,False);top=c.z-e.z+lift+8
x=c.x;y=c.y;floor='/Game/DwarvenCitadel/Meshes/SM_ExteriorFloor_A_01';wall='/Game/DwarvenCitadel/Meshes/SM_ST_WallM_02'
# Fit the hall's footprint and gate shoulders; avoid a giant square platform.
for ix in range(3):
 for iy in range(6):boxmesh('HallDeck_'+str(ix)+'_'+str(iy),floor,[x+(ix-1)*770,y-1300+(iy+.5)*520,top-40],[776,526,80])
for ix in range(4):boxmesh('GateDeck_'+str(ix),floor,[x+(ix-1.5)*770,y-1580,top-40],[776,570,80])
# Retaining panels follow individual footings. Bury lower ends by 0.35 m.
for side in [-1,1]:
 for iy in range(6):
  px=x+side*1160;py=y-1300+(iy+.5)*520;bottom=min(ground(px,py-260),ground(px,py),ground(px,py+260))-35
  if top-bottom>60:boxmesh('Side_'+str(side)+'_'+str(iy),wall,[px,py,(top+bottom)/2],[526,100,max(60,top-bottom)],90)
for ix in range(3):
 px=x+(ix-1)*770;py=y+1830;bottom=min(ground(px-385,py),ground(px,py),ground(px+385,py))-35
 if top-bottom>60:boxmesh('Rear_'+str(ix),wall,[px,py,(top+bottom)/2],[776,100,max(60,top-bottom)],0)
for side in [-1,1]:
 px=x+side*880;py=y-1860;bottom=min(ground(px-630,py),ground(px+630,py))-35
 boxmesh('GateShoulder_'+str(side),wall,[px,py,(top+bottom)/2],[1260,110,max(60,top-bottom)],0)
# An owned floor assembly provides a compact engineered approach to the native gate.
start=u.Vector(x,y-4200,ground(x,y-4200)+8);end=u.Vector(x,y-1730,top)
length=(end-start).length();pitch=math.degrees(math.atan2(end.z-start.z,2470));rot=u.Rotator(pitch=pitch,yaw=90,roll=0)
count=5
for i in range(count):
 center=start+(end-start)*((i+.5)/count);actor=boxmesh('Ramp_'+str(i),floor,[center.x,center.y,center.z-30],[length/count+6,390,60],90);actor.set_actor_rotation(rot,False)
# Keep native hold transform and all source packages in the receipt.
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(E/'dwarf-foundation-r1.json').write_text(json.dumps(dict(map=w.get_path_name(),hold_before_cm=[before.x,before.y,before.z],hold_after_cm=[before.x,before.y,before.z+lift],xy_unchanged=True,scale_unchanged=True,terrain_changed=False,top_cm=top,ramp_grade_deg=abs(pitch),actors=rows,review_pending=True),indent=2))
print('DWARF_FOUNDATION',len(rows),'lift_cm',lift,'top_cm',top,'ramp_grade',pitch)
