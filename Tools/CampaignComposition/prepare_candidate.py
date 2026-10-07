from pathlib import Path
import numpy as np,json,hashlib,math,argparse
from PIL import Image,ImageDraw,ImageFilter
ROOT=Path(__file__).resolve().parents[2];E=ROOT/'Evidence/ProductionWorldComposition-20261007';L=E/'Local';D=ROOT/'Data/CampaignCompositionLocal';D.mkdir(exist_ok=True)
parser=argparse.ArgumentParser();parser.add_argument('--routes',default='route-composition-proposal.json');parser.add_argument('--control',default='Composition_Control_r3.png');args=parser.parse_args()
z=np.load(L/'composed-height.npy');raw=np.rint(z*128+32768);assert raw.min()>0 and raw.max()<65535
p=D/'Composition_3500_r2.png';assert not p.exists() or np.array_equal(np.array(Image.open(p)),raw.astype(np.uint16));Image.fromarray(raw.astype(np.uint16)).save(p)
j=json.loads((E/args.routes).read_text());assert j['success_count']==51 and max(r['max_road_grade_deg'] for r in j['routes'])<22.01
world=json.loads((ROOT/'Data/soul_world_overmap_v1_20260922.json').read_text());assert {a['id'] for a in j['anchors']}=={a['id'] for a in world['nodes']};assert {tuple(sorted([r['a'],r['b']])) for r in j['routes']}=={tuple(sorted([r['a'],r['b']])) for r in world['edges']}
# Deterministic road/source selection weights, not generated visual textures.
road=Image.new('L',(2041,2041));draw=ImageDraw.Draw(road);anchors={a['id']:a for a in j['anchors']};allpoints=[]
for r in j['routes']:
 for s in r['segments']:
  if s['type']!='road':continue
  dense=[]
  for a,b in zip(s['points_m'][:-1],s['points_m'][1:]):
   a,b=np.array(a),np.array(b);dense.extend(a+(b-a)*np.linspace(0,1,max(1,math.ceil(np.linalg.norm(b-a))),endpoint=False)[:,None])
  dense.append(np.array(s['points_m'][-1]));run=[]
  for pnt in dense:
   inside=any(abs(pnt[0]-a['xy_m'][0])<a.get('footprint_m',[20,20])[0]/2+5 and abs(pnt[1]-a['xy_m'][1])<a.get('footprint_m',[20,20])[1]/2+5 for a in j['anchors'] if a['kind']=='capital')
   if inside:
    if len(run)>1:draw.line(run,fill=255,width=3)
    run=[]
   else:run.append(tuple(pnt/3500*2040));allpoints.append(pnt.tolist())
  if len(run)>1:draw.line(run,fill=255,width=3)
road=road.filter(ImageFilter.GaussianBlur(.55));mesa=np.load(L/'mesa-mask.npy');yy,xx=np.mgrid[:2041,:2041];x=xx*3500/2040;y=yy*3500/2040
# Mask-only regional selection of owned surfaces; no invented albedo content.
dark=np.clip((y-2500)/350,0,1)*np.clip((x-1250)/500,0,1);snow=np.clip((z-90)/45,0,1)*np.clip((1150-y)/450,0,1)
rgba=np.stack([np.array(road),np.uint8(mesa*255),np.uint8(dark*255),np.uint8(snow*255)],-1);Image.fromarray(rgba).save(D/args.control)
receipt={'map':'/Game/SoulCampaignComposition/L_Composition_3500_r2','heightmap':str(p),'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'dimensions_m':[3500,3500],'resolution':[2041,2041],'components':[8,8],'section_size':255,'subsections':1,'location_cm':[-175000,-175000,0],'scale':[350000/2040,350000/2040,100],'water_z_m':0,'quantization_max_error_m':float(np.abs((raw-32768)/128-z).max()),'status':'isolated opt-in candidate; not approved foundation','control_channels':{'R':'merged road footprint','G':'actual Mesa donor influence','B':'southern owned surface selection','A':'northern high-elevation owned snow selection'}}
(E/'candidate-import-plan.json').write_text(json.dumps(receipt,indent=2));print(json.dumps(receipt))
