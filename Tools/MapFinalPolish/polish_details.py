"""Small capital forecourt and bridge wing-wall details; no terrain/collision edits."""
import unreal as u,json,math
from pathlib import Path
R=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');E=R/'Evidence/MapFinalPolish-20261007';P=R/'Evidence/MapPolish-20261007';a=u.get_editor_subsystem(u.EditorActorSubsystem);w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert w.get_path_name().startswith('/Game/SoulCampaignComposition/L_Composition_3500_r2')
ignore=[v for v in a.get_all_level_actors() if not isinstance(v,u.Landscape)];rows=[]
def ground(x,y):
 h=u.SystemLibrary.line_trace_single(w,u.Vector(x+.1,y+.1,100000),u.Vector(x+.1,y+.1,-100000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE).to_dict();assert h.get('blocking_hit');return h['impact_point'].z

def place(label,mesh,center,dims,pitch=0,yaw=0):
 b=mesh.get_bounding_box();size=b.max-b.min;scale=u.Vector(*(dims[i]/getattr(size,'xyz'[i]) for i in range(3)));rot=u.Rotator(pitch=pitch,yaw=yaw,roll=0);offset=rot.quaternion().rotate_vector((b.min+b.max)*.5*scale);actor=a.spawn_actor_from_class(u.StaticMeshActor,u.Vector(*center)-offset);actor.set_actor_label('FinalPolish_'+label);actor.static_mesh_component.set_static_mesh(mesh);actor.set_actor_scale3d(scale);actor.set_actor_rotation(rot,False);actor.set_actor_enable_collision(False);ignore.append(actor);rows.append(dict(label=actor.get_actor_label(),mesh=mesh.get_path_name(),center_cm=center,dimensions_cm=dims,pitch=pitch,yaw=yaw));return actor
cobble=u.load_asset('/Game/SoulCampaignComposition/FinalPolish/SM_HumanCobble_r1');wall=u.load_asset('/Game/DwarvenCitadel/Meshes/SM_ST_WallM_02')
# Follow the already-reviewed open street into the represented keep entrance.
points=[[727.45098,1660.78431],[734,1654],[739.5,1647]];index=0
for p,q in zip(points[:-1],points[1:]):
 dx=q[0]-p[0];dy=q[1]-p[1];distance=math.hypot(dx,dy);count=math.ceil(distance/3.5);yaw=math.degrees(math.atan2(dy,dx))
 for i in range(count):
  aa=[(p[k]+(q[k]-p[k])*i/count)*100-175000 for k in [0,1]];bb=[(p[k]+(q[k]-p[k])*(i+1)/count)*100-175000 for k in [0,1]];za=ground(*aa);zb=ground(*bb);center=[(aa[0]+bb[0])/2,(aa[1]+bb[1])/2,(za+zb)/2+1];pitch=math.degrees(math.atan2(zb-za,distance*100/count));place('HumanForecourt_'+str(index),cobble,center,[distance*100/count+4,280,9],pitch,yaw);index+=1
# Abutment wing walls sit outside travel width and taper naturally into banks.
B=R/'Evidence/ProductionWorldComposition-20261007';sites=json.loads((B/'selected-crossing-sites-r3.json').read_text())['crossings'];decks={v['id']:v for v in json.loads((B/'bridge-placement-r2.json').read_text())['bridges']}
for s in sites:
 if s['gate_id'] not in ['human_west_bridge','southern_crossing']:continue
 t=math.radians(s['yaw_deg']);ax=[math.cos(t),math.sin(t)];side=[-ax[1],ax[0]];deck=decks[s['gate_id']]
 for end in [-1,1]:
  for sign in [-1,1]:
   along=end*(s['span_m']/2+1.25);across=sign*2.8;px=(s['center_xy_m'][0]+ax[0]*along+side[0]*across)*100-175000;py=(s['center_xy_m'][1]+ax[1]*along+side[1]*across)*100-175000;gz=ground(px,py);top=(deck['deck_z_m']+along*deck['deck_gradient'])*100+18;bottom=min(gz-35,top-65)
   place('Wingwall_'+s['gate_id']+'_'+str(end)+'_'+str(sign),wall,[px,py,(top+bottom)/2],[280,65,top-bottom],0,s['yaw_deg'])
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(E/'arrival-crossing-details-r1.json').write_text(json.dumps(dict(actors=rows,terrain_changed=False,travel_collision_unchanged=True,human_correspondence='Owned CastleTown cobblestone on existing keep forecourt; represented keep direction retained. Original outer gate/curtain/island are omitted from the existing miniature; exact island correspondence is not claimed.'),indent=2));print('DETAILS',len(rows))
