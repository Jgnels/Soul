"""Local ford cues and ferry landing geometry from conventional owned meshes."""
import unreal as u,json,math,random
from pathlib import Path
R=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');E=R/'Evidence/MapPolish-20261007'
a=u.get_editor_subsystem(u.EditorActorSubsystem);w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert w.get_path_name().startswith('/Game/SoulCampaignComposition/L_Composition_3500_r2')
assert not any(x.get_actor_label().startswith('Polish_Landing_') for x in a.get_all_level_actors())
wood=u.load_asset('/Game/Forest_village/Meshes/Wood_modules/SM_bridge_module');post=u.load_asset('/Game/Forest_village/Meshes/Wood_modules/SM_beam_circular');rock=u.load_asset('/Game/Forest_village/Meshes/Rocks/SM_rock_03');assert wood and post and rock
ignored=[x for x in a.get_all_level_actors() if not isinstance(x,u.Landscape)]
def ground(x,y):
 hit=u.SystemLibrary.line_trace_single(w,u.Vector(x+.1,y+.1,100000),u.Vector(x+.1,y+.1,-100000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignored,u.DrawDebugTrace.NONE).to_dict()
 assert hit.get('blocking_hit') and isinstance(hit['hit_actor'],u.Landscape),(x,y,str(hit))
 return hit['impact_point'].z
def spawn(label,mesh,pos,scale,rot):
 actor=a.spawn_actor_from_class(u.StaticMeshActor,pos);actor.set_actor_label(label);actor.static_mesh_component.set_static_mesh(mesh);actor.set_actor_scale3d(scale);actor.set_actor_rotation(rot,False);ignored.append(actor);return actor
rows=json.loads((E/'ferry-jetty-placement.json').read_text())['jetties'];placed=[]
for row in rows:
 span=row['span_m'];count=math.ceil(span/2.4);length=span/count;yaw=row['yaw_deg'];rot=u.Rotator(yaw=yaw+90);t=math.radians(yaw);axis=u.Vector(math.cos(t),math.sin(t),0);side=u.Vector(-math.sin(t),math.cos(t),0)
 center=u.Vector(row['center_xy_m'][0]*100-175000,row['center_xy_m'][1]*100-175000,row['deck_z_m']*100)
 scale=u.Vector(row['width_m']/1.7144374,length/2.45502976,1);offset=rot.quaternion().rotate_vector(u.Vector(85.116386*scale.x,-118.751488*scale.y,0))
 for i in range(count):
  pos=center+axis*((-span/2+(i+.5)*length)*100)-offset
  spawn('Polish_Landing_'+row['id']+'_deck_'+str(i),wood,pos,scale,rot)
 b=post.get_bounding_box();dims=[b.max.x-b.min.x,b.max.y-b.min.y,b.max.z-b.min.z];major=max(range(3),key=lambda i:dims[i]);postrot=[u.Rotator(pitch=90),u.Rotator(roll=90),u.Rotator()][major]
 for i in range(count+1):
  for sign in [-1,1]:
   foot=center+axis*((-span/2+i*length)*100)+side*(sign*row['width_m']*42)
   gz=ground(foot.x,foot.y);height=max(40,center.z-gz+30);sc=[18/max(d,1) for d in dims];sc[major]=height/dims[major]
   mid=u.Vector((b.min.x+b.max.x)*.5*sc[0],(b.min.y+b.max.y)*.5*sc[1],(b.min.z+b.max.z)*.5*sc[2]);foot.z=gz+height*.5-15
   spawn('Polish_Landing_'+row['id']+'_post_'+str(i)+'_'+str(sign),post,foot-postrot.quaternion().rotate_vector(mid),u.Vector(*sc),postrot).set_actor_enable_collision(False)
 placed.append(dict(id=row['id'],deck_modules=count,span_m=span,posts=2*(count+1)))
# The ford remains an unbridged, shallow bed. Low owned stones expose its edges
# and underwater bed, not an invented dam or stepping-stone bridge.
random.seed(71007);c=[775.666972773728,2196.255208067845];t=math.pi/4;rb=rock.get_bounding_box();rd=[rb.max.x-rb.min.x,rb.max.y-rb.min.y,rb.max.z-rb.min.z]
for i in range(38):
 along=random.uniform(-7.5,7.5);across=random.choice([-1,1])*random.uniform(1.9,2.8);x=c[0]+math.cos(t)*along-math.sin(t)*across;y=c[1]+math.sin(t)*along+math.cos(t)*across
 wx,wy=x*100-175000,y*100-175000;gz=ground(wx,wy);width=random.uniform(25,55);scale=u.Vector(width/rd[0],width/rd[1],random.uniform(8,15)/rd[2]);rot=u.Rotator(yaw=random.uniform(0,360));center=rot.quaternion().rotate_vector(u.Vector((rb.min.x+rb.max.x)*.5*scale.x,(rb.min.y+rb.max.y)*.5*scale.y,rb.min.z*scale.z))
 spawn('Polish_FordBed_'+str(i),rock,u.Vector(wx,wy,gz-3)-center,scale,rot).set_actor_enable_collision(False)
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(E/'landing-ford-placement.json').write_text(json.dumps({'jetties':placed,'ford_owned_rock_instances':38,'meshes':[wood.get_path_name(),post.get_path_name(),rock.get_path_name()],'terrain_edited':False,'donor_saved':False,'status':'render review required'},indent=2))
print('FERRY_LANDINGS_AND_FORD',placed)
