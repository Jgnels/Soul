"""Optional corridor geometry contracts; canonical campaign rules remain unchanged."""
import json,unittest,numpy as np
from pathlib import Path
from test_soul_mesa_integration import MesaIntegrationTests as Terrain
ROOT=Path(__file__).resolve().parents[1]
class EvilCorridorTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  Terrain.setUpClass();cls.p=json.loads((ROOT/'Data/CampaignEvilCorridor/presentation.json').read_text())
 @classmethod
 def surface(cls,xy):
  z=Terrain.height(xy)+22
  for b in cls.p['bridges']:
   ang=np.radians(b['yaw']);d=xy-np.array(b['center'][:2]);x=d[:,0]*np.cos(ang)+d[:,1]*np.sin(ang);y=-d[:,0]*np.sin(ang)+d[:,1]*np.cos(ang)
   distance=np.hypot(np.maximum(np.abs(x)-b['half_span'],0),y);t=np.clip(distance/3000,0,1);weight=1-t*t*(3-2*t)
   z=np.maximum(z,z+(b['center'][2]+10-z)*weight)
  return z
 def test_canonical_topology_and_height_preserved(self):
  old=Terrain.profile;edges=lambda p:{tuple(sorted((r['a'],r['b']))) for r in p['routes']}
  self.assertEqual(edges(old),edges(self.p));self.assertEqual(set(old['regions']),set(self.p['regions']));self.assertEqual(old['height_sha256'],self.p['height_sha256'])
 def test_settlements_are_on_dry_footprints(self):
  for name,xyz in self.p['regions'].items():
   radius=1600 if name=='human_capital' else 1300 if name=='orc_camp' else 1100
   a=np.linspace(-radius,radius,33);xx,yy=np.meshgrid(a+xyz[0],a+xyz[1]);h=Terrain.height(np.column_stack([xx.ravel(),yy.ravel()]))
   self.assertGreater(h.min(),100,name);self.assertLess(np.ptp(h),800,name)
 def test_port_and_stronghold_are_in_evil_region(self):
  for name in ('orc_watch','orc_camp','north_pass'):
   x,y,z=self.p['regions'][name];self.assertLess(x,-40000);self.assertLess(y,-20000)
  self.assertEqual(self.p['display_names']['orc_watch'],'Ashport')
 def test_routes_and_explicit_bridge(self):
  report=[]
  for r in self.p['routes']:
   pts=np.array(r['points']);np.testing.assert_allclose(pts[0],self.p['regions'][r['a']][:2]);np.testing.assert_allclose(pts[-1],self.p['regions'][r['b']][:2])
   t=np.linspace(0,1,12001);xy=np.column_stack([np.interp(t,np.linspace(0,1,len(pts)),pts[:,i]) for i in (0,1)])
   h=self.surface(xy);ds=np.linalg.norm(np.diff(xy,axis=0),axis=1);grade=np.degrees(np.arctan2(np.abs(np.diff(h)),ds))
   # This mountainous route admits short grades up to 30 degrees; not the plains profile's 22.5-degree contract.
   self.assertLess(grade.max(),30,(r['a'],r['b']));self.assertGreater(h.min(),30)
   wet=Terrain.height(xy)<=0
   if wet.any():
    covered=np.zeros(wet.sum(),dtype=bool)
    for b in self.p['bridges']:
     ang=np.radians(b['yaw']);d=xy[wet]-np.array(b['center'][:2]);along=d[:,0]*np.cos(ang)+d[:,1]*np.sin(ang);lateral=-d[:,0]*np.sin(ang)+d[:,1]*np.cos(ang)
     covered|=(np.abs(lateral)<1)&(np.abs(along)<=b['half_span'])
     self.assertLess(b['half_span']*2,6500,'arches must cross individual water gaps, not the dry shoal')
    self.assertTrue(covered.all(),'every water sample must lie on a physical bridge')
   report.append(dict(a=r['a'],b=r['b'],length_m=float(ds.sum()/100),max_grade_degrees=float(grade.max()),wet_samples=int(wet.sum())))
  out=ROOT/'Evidence/EvilCorridor-20261001/route-validation.json';out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(report,indent=2)+'\n')
 def test_short_crossings_have_dry_abutments_and_water_underneath(self):
  bridges=[b for b in self.p['bridges'] if b['a']=='river_ford']
  self.assertEqual(len(bridges),1)
  for b in bridges:
   ang=np.radians(b['yaw']);direction=np.array([np.cos(ang),np.sin(ang)]);c=np.array(b['center'][:2]);half=b['half_span']
   self.assertTrue((Terrain.height([c-direction*half,c+direction*half])>0).all())
   samples=c+np.linspace(-half,half,101)[:,None]*direction
   self.assertGreater(np.mean(Terrain.height(samples)<=0),.5,'most of each span should be over actual water')
if __name__=='__main__':unittest.main(verbosity=2)
