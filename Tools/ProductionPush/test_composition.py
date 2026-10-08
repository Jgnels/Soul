"""Independent profile/topology/preservation contracts for the opt-in candidate."""
from pathlib import Path
import unittest,json,hashlib,math
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionPush-20261008'
class CompositionData(unittest.TestCase):
 @classmethod
 def setUpClass(c):
  c.p=json.loads((R/'Data/CampaignComposition/presentation.json').read_text());c.w=json.loads((R/'Data/soul_world_overmap_v1_20260922.json').read_text())
 def test_exact_canonical_graph(self):
  self.assertEqual(set(self.p['regions']),{n['id'] for n in self.w['nodes']})
  self.assertEqual({tuple(sorted((r['a'],r['b']))) for r in self.p['routes']},{tuple(sorted((r['a'],r['b']))) for r in self.w['edges']})
  self.assertEqual(len(self.p['routes']),51)
 def test_frozen_height_and_route_receipts(self):
  self.assertEqual(hashlib.sha256((R/'Data/CampaignCompositionLocal/Composition_3500_r2.png').read_bytes()).hexdigest(),self.p['height_png_sha256'])
  self.assertEqual(self.p['height_png_sha256'],'bd84f1986c9fd71b51725647d8ae89644a0681c5d65f2d2aebcf6ab31383ba6b')
  self.assertEqual(hashlib.sha256((E/'Local/routes-final.json').read_bytes()).hexdigest(),self.p['source_routes_sha256'])
  q=json.loads((E/'native-route-final-summary.json').read_text());self.assertEqual(q['passes'],51);self.assertEqual(q['misses'],0)
 def test_ferry_ranges_and_route_endpoints(self):
  passages=set();uses=0
  for r in self.p['routes']:
   p=r['points'];length=sum(math.dist(a[:2],b[:2]) for a,b in zip(p,p[1:]));self.assertGreater(length,0)
   for end,k in [(p[0],r['a']),(p[-1],r['b'])]:self.assertLess(math.dist(end[:2],self.p['regions'][k][:2]),.01)
   for f in r['ferries']:
    self.assertGreaterEqual(f['start_cm'],0);self.assertGreater(f['end_cm'],f['start_cm']);self.assertLessEqual(f['end_cm'],length+1);passages.add(f['id'])
   uses+=bool(r['ferries'])
  self.assertEqual(len(passages),2);self.assertEqual(uses,7)
 def test_miniature_uses_measured_native_placement(self):
  h=next(a for a in json.loads((E/'candidate-scale-actors.json').read_text()) if a['label']=='Composition_Scale_human_capital')
  self.assertEqual(self.p['miniatures']['human_capital'],{k:h[k] for k in ('location','rotation','scale')})
 def test_existing_scenario_parameters_preserved(self):
  for new,old in [('RuntimeProof.json','soul_vertical_scenario_20260925.json'),('HumanRuntimeProof.json','SettlementEnvironments/HumanCapitalRuntimeProof.json')]:
   a=json.loads((R/'Data/CampaignComposition'/new).read_text());b=json.loads((R/'Data'/old).read_text())
   for k in b:
    if k not in ['start_state','scenario_id','status']:self.assertEqual(a[k],b[k],k)
   self.assertEqual(set(a['start_state']['region_ids']),set(self.p['regions']))
if __name__=='__main__':unittest.main(verbosity=2)
