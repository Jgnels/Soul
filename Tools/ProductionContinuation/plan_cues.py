"""Small contextual cues at existing anchors; no new settlements or strategic nodes."""
from pathlib import Path
import sys,json,math,numpy as np
R=Path.cwd();E=R/'Evidence/ProductionContinuation-20261008';sys.path.insert(0,str(R/'Tools/MapPolish'));from route_surface import sample,assess,dense
sys.path.insert(0,str(R/'Evidence/MapPolish-20261007/Local/python-libs'));from scipy.spatial import cKDTree
D=json.loads((E/'Local/routes-final.json').read_text());A={a['id']:np.array(a['xy_m']) for a in D['anchors']};roads=np.concatenate([dense(s['points_m'],2) for r in D['routes'] for s in r['segments'] if s['type']=='road']);tree=cKDTree(roads);rows=[]
def add(label,mesh,xy,scale,yaw=0,role='',relief=None,z=None):
 rows.append(dict(label=label,mesh=mesh,xy_m=list(map(float,xy)),scale=scale,yaw=yaw,role=role,footprint_relief_m=relief,ground_z_m=float(sample(xy)) if z is None else z))
def bench(id,width,depth,maxrelief=1.0):
 center=A[id];choices=[]
 for dx in range(-35,36,4):
  for dy in range(-35,36,4):
   q=center+[dx,dy]
   if tree.query(q)[0]<max(width,depth)*.6+4:continue
   points=q+np.array([[-width/2,-depth/2],[width/2,-depth/2],[-width/2,depth/2],[width/2,depth/2],[0,0]])
   h=sample(points);relief=float(np.ptp(h))
   if min(h)>2 and relief<=maxrelief:choices.append((float(np.linalg.norm(q-center))+relief*10,q,relief,float(np.min(h))))
 return min(choices,key=lambda x:x[0]) if choices else None
for ident,mesh,scale,width,depth,role in [
 ('dark_castle_approach','/Game/Soul/CampaignProxies/Population/SM_Ravenhold_GateTower_r1',.55,8,12,'provisional occupied-ruin horizon cue; not a capital proxy'),
 ('orc_war_camp','/Game/Soul/CampaignProxies/Population/SM_Field_Defenses_A_r1',.8,8,8,'field fortification beside existing military approach'),
 ('orc_camp','/Game/Soul/CampaignProxies/Population/SM_Field_Defenses_B_r1',.7,7,7,'camp outwork; preserves wide military lane')]:
 b=bench(ident,width,depth,1.25)
 if b:add('Continuation_'+ident,mesh,b[1],[scale]*3,0,role,b[2],b[3]-.12)
# A short landing from a measured low-grade harbor approach into ocean water.
p=A['viking_harbour'];q=np.array([[p[0]+x,p[1]+y] for x in range(-80,81,2) for y in range(-80,81,2)]);h=sample(q);coast=q[(h>1.0)&(h<1.65)];water=q[h<-.7];candidates=[]
for shore in coast:
 if assess([p,shore],1)['max_grade_deg']>22:continue
 end=water[np.argmin(np.linalg.norm(water-shore,axis=1))];dist=np.linalg.norm(end-shore)
 if 6<dist<24:candidates.append((np.linalg.norm(shore-p)+dist*.3,shore,end))
assert candidates
_,shore,end=min(candidates,key=lambda v:v[0]);axis=(end-shore)/np.linalg.norm(end-shore);normal=np.array([-axis[1],axis[0]]);length=np.linalg.norm(end-shore)+3;yaw=math.degrees(math.atan2(axis[1],axis[0]));deckz=max(1.6,float(sample(shore))+.1)
for i in range(math.ceil(length/5)):
 center=shore+axis*(i*5+2.5);add('Continuation_VikingJettyDeck_'+str(i),'/Game/Forest_village/Meshes/Wood_modules/SM_floor_wood_02',center,[1,.95,1],yaw,'harbor landing; native timber modules',None,deckz)
for i in range(math.ceil(length/5)+1):
 for sign in [-1,1]:
  xy=shore+axis*(i*5)+normal*sign*1.2;bottom=float(sample(xy))-.3;top=deckz+.65;add('Continuation_VikingPile_'+str(i)+'_'+str(sign),'/Game/Forest_village/Meshes/Wood_modules/SM_beam_circular',xy,[1,1,(top-bottom)/3.766879],0,'timber pile seated in actual bed',None,bottom)
# Sparse boulder context at the existing mesa transition and woodland bank; never in a travel lane.
for region,center,count in [('Orc',np.array([2480,1450]),8),('Nature',np.array([450,2790]),7)]:
 rng=np.random.default_rng(81008 if region=='Orc' else 81009)
 for i in range(count):
  for attempt in range(40):
   xy=center+rng.uniform([-75,-55],[75,55]);height=float(sample(xy))
   if height>3 and tree.query(xy)[0]>9:break
  else:continue
  sc=float(rng.uniform(2.3,4.5));add('Continuation_'+region+'BankRock_'+str(i),'/Game/Forest_village/Meshes/Rocks/SM_rock_03',xy,[sc,sc*.85,sc*.8],float(rng.uniform(0,360)),'owned weathered rock context',None,height-.25*sc)
(E/'regional-cues-plan.json').write_text(json.dumps({'placements':rows,'viking_spur':{'points_m':[p.tolist(),shore.tolist()],'width_m':2.4,'grade':assess([p,shore],.25)},'heightfield_changed':False,'anchors_unchanged':True,'status':'bounded visual cues, not completed faction settlements'},indent=2));print('CUES',len(rows),'JETTY',shore.tolist(),end.tolist(),length)
