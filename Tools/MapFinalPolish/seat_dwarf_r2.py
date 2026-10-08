"""Replace rejected round floor modules with reviewed owned rectangular paving."""
import unreal as u,json,math
from pathlib import Path
R=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');E=R/'Evidence/MapFinalPolish-20261007'
a=u.get_editor_subsystem(u.EditorActorSubsystem);w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert w.get_path_name().startswith('/Game/SoulCampaignComposition/L_Composition_3500_r2')
previous=json.loads((E/'dwarf-foundation-r1.json').read_text());top=previous['top_cm'];hold=next(x for x in a.get_all_level_actors() if x.get_actor_label()=='Composition_Scale_dwarf_hold');c,e=hold.get_actor_bounds(False);x,y=c.x,c.y
removed=[]
for actor in a.get_all_level_actors():
 if actor.get_actor_label().startswith(('FinalPolish_Dwarf_HallDeck_','FinalPolish_Dwarf_GateDeck_','FinalPolish_Dwarf_Ramp_')):
  removed.append(actor.get_actor_label());a.destroy_actor(actor)
ignore=[v for v in a.get_all_level_actors() if not isinstance(v,u.Landscape)]
def ground(x,y):
 h=u.SystemLibrary.line_trace_single(w,u.Vector(x+.1,y+.1,100000),u.Vector(x+.1,y+.1,-100000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE);d=h.to_dict();assert d.get('blocking_hit');return d['impact_point'].z
mesh=u.load_asset('/Game/DwarvenCitadel/Meshes/SM_ST_FloorTile_04_T01');wall=u.load_asset('/Game/DwarvenCitadel/Meshes/SM_ST_WallM_02');rows=[]
def place(label,m,center,dims,pitch=0,yaw=0):
 b=m.get_bounding_box();size=b.max-b.min;scale=u.Vector(*(dims[i]/getattr(size,'xyz'[i]) for i in range(3)));rot=u.Rotator(pitch=pitch,yaw=yaw,roll=0);offset=rot.quaternion().rotate_vector((b.min+b.max)*.5*scale)
 ac=a.spawn_actor_from_class(u.StaticMeshActor,u.Vector(*center)-offset);ac.static_mesh_component.set_static_mesh(m);ac.set_actor_label('FinalPolish_Dwarf_'+label);ac.set_actor_scale3d(scale);ac.set_actor_rotation(rot,False);ac.set_actor_enable_collision(False);rows.append(dict(label=ac.get_actor_label(),center_cm=center,dimensions_cm=dims,pitch=pitch,yaw=yaw));return ac
# Two connected irregular-footprint decks, pavement on supported retaining walls.
for row in range(19):
 py=y-1810+row*200
 half=1500 if py<y-1300 else 1100
 for col in range(-int(half/200),int(half/200)+1):
  px=x+col*200
  place('Paving_'+str(row)+'_'+str(col),mesh,[px,py,top-12],[202,202,24])
start=u.Vector(x,y-4200,ground(x,y-4200)+8);end=u.Vector(x,y-1730,top)
length=(end-start).length();pitch=math.degrees(math.atan2(end.z-start.z,2470));count=math.ceil(length/200)
for i in range(count):
 center=start+(end-start)*((i+.5)/count)
 for side in [-1,1]:place('PavedRamp_'+str(i)+'_'+str(side),mesh,[center.x+side*98,center.y,center.z-12],[length/count+3,198,24],pitch,90)
 # Side masonry follows the ramp profile, embedded into actual ground.
 for side in [-1,1]:
  px=center.x+side*200;bottom=min(ground(px,center.y-100),ground(px,center.y+100))-30;height=center.z-bottom-14
  if height>35:place('RampFooting_'+str(i)+'_'+str(side),wall,[px,center.y,(center.z-14+bottom)/2],[2470/count+3,65,height],0,90)
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(E/'dwarf-foundation-r2.json').write_text(json.dumps(dict(terrain_changed=False,hold_lift_cm=325,hold_xy_scale_unchanged=True,rejected_modules=removed,reason='Round floor module produced a stepping-stone visual; replaced with reviewed native square paving.',owned_paving=mesh.get_path_name(),owned_retaining_wall=wall.get_path_name(),ramp_grade_deg=pitch,top_cm=top,actors=rows,review_pending=True),indent=2))
print('DWARF_PAVING',len(rows),'ramp_grade',pitch)
