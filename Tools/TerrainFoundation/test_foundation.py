"""Evidence-backed invariants for the bounded terrain study; not visual acceptance."""
from pathlib import Path
import hashlib,json,unittest,math
import numpy as np
from PIL import Image
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'Evidence/TerrainFoundation-20261007'
def read(name):return json.loads((OUT/name).read_text(encoding='utf-8-sig'))
class FoundationTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.manifest=read('candidate-transform.json');cls.raw=np.load(cls.manifest['source']);cls.fit=read('dense-route-fit-r5.json');cls.world=json.loads((ROOT/'Data/soul_world_overmap_v1_20260922.json').read_text())
 def test_reference_bytes_preserved(self):
  for r in read('reference-baseline.json')['files']:
   with self.subTest(path=r['path']):self.assertEqual(hashlib.sha256((ROOT/r['path']).read_bytes()).hexdigest(),r['sha256'])
 def test_source_bytes_and_every_height_sample_preserved(self):
  self.assertEqual(hashlib.sha256(Path(self.manifest['source']).read_bytes()).hexdigest(),self.manifest['source_sha256'])
  with Image.open(self.manifest['heightmap']) as source_image:image=np.array(source_image)
  self.assertEqual(image.shape,(2041,2041));np.testing.assert_array_equal(image,np.rot90(self.raw,1))
 def test_uniform_scale_and_physical_envelope(self):
  m=self.manifest;self.assertEqual(m['physical_size_m'],[3500,3500]);self.assertEqual(m['component_count_per_side'],8)
  self.assertAlmostEqual(m['scale'][0]*2040,350000,6);self.assertAlmostEqual(m['scale'][2]/300,3500/8160,10)
 def test_exact_canonical_node_and_edge_sets(self):
  f=self.fit;w=self.world;self.assertEqual(len(f['anchors']),36);self.assertEqual(len(f['routes']),51)
  self.assertEqual({a['id'] for a in f['anchors']},{a['id'] for a in w['nodes']})
  edge=lambda rows:{tuple(sorted((r['a'],r['b']))) for r in rows}
  self.assertEqual(edge(f['routes']),edge(w['edges']));self.assertEqual(len(edge(f['routes'])),51)
 def test_barycentric_height_matches_independent_native_collision(self):
  # Compute triangle-plane barycentric heights, independently of route sampler.
  probes=np.array(read('triangulation-probes.json'));xy=(probes[:,:2]+175000)/self.manifest['scale'][0];ij=np.floor(xy).astype(int);uv=xy-ij
  z=np.rot90(self.raw,1).astype(float);zs=[]
  for (x,y),(u,v) in zip(ij,uv):
   if u>=v:h=(1-u)*z[y,x]+(u-v)*z[y,x+1]+v*z[y+1,x+1]
   else:h=(1-v)*z[y,x]+u*z[y+1,x+1]+(v-u)*z[y+1,x]
   zs.append((h-32768)*self.manifest['scale'][2]/128+self.manifest['location'][2])
  self.assertEqual(len(zs),300);self.assertLess(float(np.max(np.abs(np.array(zs)-probes[:,2]))),.2)
 def test_native_road_centerline_grade_and_explicit_shoulder_limit(self):
  roads=read('road-mesh-progress-r2.json')['routes'];self.assertEqual(len(roads),51)
  self.assertLess(max(r['maximum_collision_centerline_grade_deg'] for r in roads),22)
  # Deliberately retain evidence of provisional shoulders instead of hiding it.
  self.assertGreater(max(r['maximum_cross_slope_deg'] for r in roads),22)
  self.assertLess(max(r['maximum_cross_slope_deg'] for r in roads),27)
 def test_water_limitations_remain_explicit(self):
  drainage=read('drainage-study.json');self.assertFalse(drainage['terrain_modified'])
  self.assertEqual(len(drainage['routes_affected_at_original_water_datum']),28)
  native=read('dense-route-native-water-r1.json');self.assertLess(native['success_count'],51)
 def test_rendered_views_exist(self):
  receipt=read('Local/candidate-r3-captures/capture-receipt-r1.json');self.assertEqual(len(receipt['views']),12)
  for view in receipt['views']:
   with self.subTest(view=view['name']):
    with Image.open(view['path']) as im:self.assertEqual(im.size,(1920,1080))
    self.assertGreater(Path(view['path']).stat().st_size,100000)
if __name__=='__main__':unittest.main(verbosity=2)
