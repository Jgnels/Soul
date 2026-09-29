"""Independent contracts for the baked presentation, not campaign rule authority."""
import hashlib
import json
from pathlib import Path
import unittest
import numpy as np

ROOT=Path(__file__).resolve().parents[1]
class TerrainBakeTests(unittest.TestCase):
 def setUp(self):
  self.p=json.loads((ROOT/'Data/CampaignTerrainV2/presentation.json').read_text())
  self.raw=(ROOT/'Data/CampaignTerrainV2/FounderHeight.r16').read_bytes()
 def test_geometry_payload_and_import_range(self):
  self.assertEqual(len(self.raw),1009*1009*2)
  self.assertEqual(hashlib.sha256(self.raw).hexdigest(),self.p['height_sha256'])
  a=np.frombuffer(self.raw,dtype='<u2');self.assertGreater(a.min(),0);self.assertLess(a.max(),65535)
 def test_routes_cover_exact_canonical_founder_adjacency(self):
  world=json.loads((ROOT/'Data/soul_world_overmap_v1_20260922.json').read_text())
  ids=set(self.p['regions']);self.assertEqual(len(ids),9)
  expected={tuple(sorted((e['a'],e['b']))) for e in world['edges'] if e['a'] in ids and e['b'] in ids}
  actual={tuple(sorted((e['a'],e['b']))) for e in self.p['routes']}
  self.assertEqual(actual,expected)
  for r in self.p['routes']:
   np.testing.assert_allclose(r['points'][0],self.p['regions'][r['a']][:2],atol=.001)
   np.testing.assert_allclose(r['points'][-1],self.p['regions'][r['b']][:2],atol=.001)
 def test_frontier_scale_and_retained_margin(self):
  self.assertEqual(self.p['scale'],20)
  for p in self.p['regions'].values():self.assertLess(max(abs(p[0]),abs(p[1])),80000)
  self.assertEqual(self.p['half_extent']*self.p['scale']*2,560000)
 def test_founder_banks_have_no_vertical_heightfield_steps(self):
  h=(np.frombuffer(self.raw,dtype='<u2').astype(float).reshape(1009,1009)-32768)/8
  h=h[330:680,330:680]
  step=max(np.abs(np.diff(h,axis=0)).max(),np.abs(np.diff(h,axis=1)).max())
  self.assertLessEqual(step,17.125)
 def test_water_has_downstream_fall_and_connected_tributaries(self):
  main=np.array(self.p['waterlines'][0])
  self.assertTrue(np.all(np.diff(main[:,2])<=0))
  for line in self.p['waterlines'][1:]:
   self.assertTrue(np.all(np.diff(np.array(line)[:,2])<=0))
   outlet=np.array(line[-1]);expected=np.interp(outlet[1],main[:,1],main[:,0])
   self.assertLess(abs(outlet[0]-expected),1.)
   self.assertLess(abs(outlet[2]-np.interp(outlet[1],main[:,1],main[:,2])),1.)
 def test_bridge_decks_clear_the_river_on_existing_routes(self):
  river=np.array(self.p['waterlines'][0]);edges={frozenset((r['a'],r['b'])) for r in self.p['routes']}
  self.assertEqual(sum(b['ford'] for b in self.p['bridges']),1)
  for b in self.p['bridges']:
   self.assertIn(frozenset((b['a'],b['b'])),edges)
   x,y,z=b['center']
   self.assertLess(abs(x-np.interp(y,river[:,1],river[:,0])),5.)
   self.assertGreater(z-np.interp(y,river[:,1],river[:,2]),50.)
   self.assertGreater(b['half_span'],2000)
if __name__=='__main__':unittest.main()
