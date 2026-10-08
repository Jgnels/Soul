"""Local crossing cues and refined capital paving from reviewed owned geometry."""
import unreal as u,json,math
from pathlib import Path
R=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');E=R/'Evidence/MapFinalPolish-20261007';P=R/'Evidence/MapPolish-20261007';a=u.get_editor_subsystem(u.EditorActorSubsystem);w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert w.get_path_name().startswith('/Game/SoulCampaignComposition/L_Composition_3500_r2')
ignore=[v for v in a.get_all_level_actors() if not isinstance(v,u.Landscape)];rows=[]
def ground(x,y):
 h=u.SystemLibrary.line_trace_single(w,u.Vector(x+.1,y+.1,100000),u.Vector(x+.1,y+.1,-100000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE).to_dict();assert h.get('blocking_hit');return h['impact_point'].z

def place(label,mesh,center,dims,pitch=0,yaw=0,roll=0):
 b=mesh.get_bounding_box();size=b.max-b.min;scale=u.Vector(*(dims[i]/getattr(size,'xyz'[i]) for i in range(3)));rot=u.Rotator(pitch=pitch,yaw=yaw,roll=roll);offset=rot.quaternion().rotate_vector((b.min+b.max)*.5*scale);actor=a.spawn_actor_from_class(u.StaticMeshActor,u.Vector(*center)-offset);actor.set_actor_label('FinalPolish_'+label);actor.static_mesh_component.set_static_mesh(mesh);actor.set_actor_scale3d(scale);actor.set_actor_rotation(rot,False);actor.set_actor_enable_collision(False);ignore.append(actor);rows.append(dict(label=actor.get_actor_label(),mesh=mesh.get_path_name(),center_cm=center,dimensions_cm=dims,pitch=pitch,yaw=yaw,roll=roll));return actor
for actor in a.get_all_level_actors():
 if actor.get_actor_label().startswith('FinalPolish_HumanForecourt_'):a.destroy_actor(actor)
ignore=[v for v in a.get_all_level_actors() if not isinstance(v,u.Landscape)]
cobble=u.load_asset('/Game/SoulCampaignComposition/FinalPolish/SM_HumanCobble_r1');points=json.loads((E/'human-keep-arrival-r2.json').read_text())['points_m'];index=0
for p,q in zip(points[:-1],points[1:]):
 dx=q[0]-p[0];dy=q[1]-p[1];distance=math.hypot(dx,dy);count=math.ceil(distance/3);yaw=math.degrees(math.atan2(dy,dx))
 for i in range(count):
  aa=[(p[k]+(q[k]-p[k])*i/count)*100-175000 for k in [0,1]];bb=[(p[k]+(q[k]-p[k])*(i+1)/count)*100-175000 for k in [0,1]];za=ground(*aa);zb=ground(*bb);center=[(aa[0]+bb[0])/2,(aa[1]+bb[1])/2,(za+zb)/2-1];pitch=math.degrees(math.atan2(zb-za,distance*100/count));place('HumanForecourt_'+str(index),cobble,center,[distance*100/count+3,240 if index==0 else 280,8],pitch,yaw);index+=1
rubble=u.load_asset('/Game/SoulCampaignComposition/FinalPolish/SM_MasonryRubble_r1');post=u.load_asset('/Game/Forest_village/Meshes/Wood_modules/SM_beam_circular');rock=u.load_asset('/Game/Forest_village/Meshes/Rocks/SM_rock_03')
B=R/'Evidence/ProductionWorldComposition-20261007';sites=json.loads((B/'selected-crossing-sites-r3.json').read_text())['crossings'];decks={v['id']:v for v in json.loads((B/'bridge-placement-r2.json').read_text())['bridges']}
# Masonry rubble collars break up the clean ruined ends, clear of timber passage.
s=next(v for v in sites if v['gate_id']=='orc_broken_bridge');t=math.radians(s['yaw_deg']);axis=[math.cos(t),math.sin(t)];side=[-axis[1],axis[0]];deck=decks[s['gate_id']]['deck_z_m']*100
for end in [-1,1]:
 for sign in [-1,1]:
  for n in range(3):
   along=end*(2.55+n*.38);across=sign*(1.92+n*.21);px=(s['center_xy_m'][0]+axis[0]*along+side[0]*across)*100-175000;py=(s['center_xy_m'][1]+axis[1]*along+side[1]*across)*100-175000
   place('BrokenRubble_'+str(end)+'_'+str(sign)+'_'+str(n),rubble,[px,py,deck+8-n*18],[95+n*12,85,32+n*6],pitch=end*(n*9),yaw=s['yaw_deg']+n*29)
# Actual timber piers are below the unchanged deck; never in the travel lane.
b=post.get_bounding_box();dims=[b.max.x-b.min.x,b.max.y-b.min.y,b.max.z-b.min.z];major=max(range(3),key=lambda i:dims[i])
def timber(label,xy,top,bottom,width=22):
 dd=[width,width,width];dd[major]=top-bottom;rot={0:dict(pitch=90),1:dict(roll=90),2:{}}[major];place(label,post,[xy[0],xy[1],(top+bottom)/2],dd,**rot)
s=next(v for v in sites if v['gate_id']=='human_north_bridge');t=math.radians(s['yaw_deg']);axis=[math.cos(t),math.sin(t)];side=[-axis[1],axis[0]];d=decks[s['gate_id']]
for along in [-4,0,4]:
 for sign in [-1,1]:
  xy=[(s['center_xy_m'][k]+axis[k]*along+side[k]*sign*1.55)*100-175000 for k in [0,1]];top=(d['deck_z_m']+along*d['deck_gradient'])*100-8;bottom=min(ground(*xy)-25,top-45);timber('TimberPier_'+str(along)+'_'+str(sign),xy,top,bottom)
# Clear mooring posts at all four existing waterward landing ends.
for jetty in json.loads((P/'ferry-jetty-placement.json').read_text())['jetties']:
 t=math.radians(jetty['yaw_deg']);axis=[math.cos(t),math.sin(t)];side=[-axis[1],axis[0]]
 for sign in [-1,1]:
  xy=[(jetty['center_xy_m'][k]+axis[k]*(jetty['span_m']/2-.4)+side[k]*sign*1.5)*100-175000 for k in [0,1]];top=jetty['deck_z_m']*100+80;bottom=ground(*xy)-25;timber('Mooring_'+jetty['id']+'_'+str(sign),xy,top,bottom,20)
# A few partly buried riverbank rocks frame the existing unbridged shallow bed.
s=next(v for v in sites if v['gate_id']=='river_ford');t=math.radians(s['yaw_deg']);axis=[math.cos(t),math.sin(t)];side=[-axis[1],axis[0]]
for end in [-1,1]:
 for sign in [-1,1]:
  for n in range(3):
   xy=[(s['center_xy_m'][k]+axis[k]*end*(8+n*.75)+side[k]*sign*(2.4+n*.45))*100-175000 for k in [0,1]];gz=ground(*xy);place('FordBank_'+str(end)+'_'+str(sign)+'_'+str(n),rock,[xy[0],xy[1],gz-4],[65+17*n,50+10*n,28],yaw=n*43+sign*26)
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(E/'crossing-finish-r2.json').write_text(json.dumps(dict(actors=rows,terrain_changed=False,travel_collision_unchanged=True,ford_unbridged=True,both_ferries_retained=True,donors_saved=False),indent=2));print('LOCAL_FINISH',len(rows))
