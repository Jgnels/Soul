"""Profile isolation and changed-presentation contracts; no UE process."""
import unittest,json,re,hashlib,sys
from pathlib import Path
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionContinuation-20261008'
class ContinuationContracts(unittest.TestCase):
 def test_no_canonical_change(self):
  g=json.loads((R/'Data/soul_world_overmap_v1_20260922.json').read_text());p=json.loads((R/'Data/CampaignComposition/presentation.json').read_text())
  self.assertEqual(set(p['regions']),{n['id'] for n in g['nodes']});self.assertEqual({frozenset((v['a'],v['b'])) for v in p['routes']},{frozenset((v['a'],v['b'])) for v in g['edges']})
  old=json.loads((E/'Local/Before/Data/CampaignComposition/presentation.json').read_text());self.assertEqual(p['regions'],old['regions']);self.assertEqual(p['miniatures'],old['miniatures']);self.assertEqual(p['height_png_sha256'],old['height_png_sha256'])
 def test_two_ferry_passages_preserved(self):
  p=json.loads((R/'Data/CampaignComposition/presentation.json').read_text());self.assertEqual(sum(bool(x['ferries']) for x in p['routes']),7);self.assertEqual(len({f['id'] for x in p['routes'] for f in x['ferries']}),2)
 def test_packaging_target_does_not_promote(self):
  p=json.loads((R/'Data/CampaignComposition/PackageProfile.json').read_text());self.assertEqual(p['target'],'SoulComposition');self.assertEqual(p['required_launch_args'],['-SoulComposition']);self.assertTrue(p['default_map_unchanged']);self.assertFalse(any(x.startswith('/Game/SoulCampaignWorld/') for x in p['cook_roots']))
  self.assertEqual(len(p['cook_roots']),len(set(p['cook_roots'])))
  for path in p['exact_additional_runtime_files']:self.assertTrue((R/path).is_file(),path)
  b=(R/'Source/Soul/Soul.Build.cs').read_text();self.assertIn('Target.Name == "SoulComposition"',b);self.assertNotIn('Data/CampaignWorldTerrain/',b);self.assertNotIn('Data/CampaignWorldLocal/',b)
  self.assertIn('GameDefaultMap=/Engine/Maps/Entry',(R/'Config/DefaultEngine.ini').read_text())
 def test_save_namespaces_and_schemas(self):
  s=(R/'Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp').read_text()
  for name in ['Soul.VerticalCampaign','Soul.Composition3500.Founder','Soul.Composition3500.HumanProof']:self.assertIn(name,s)
  self.assertIn('SaveSelectedDomainsAsync(GetCampaignSaveSlotName(),CampaignSaveDomains,Done)',s);self.assertIn('LoadSelectedDomainsAsync(GetCampaignSaveSlotName(),CampaignSaveDomains,Done)',s)
  self.assertIn('In.SchemaVersion!=1',s);self.assertIn('(*Owners)->Values.Num()==World.Regions.Num()',s)
 def test_six_faction_groundwork_is_inactive(self):
  d=json.loads((E/'six-faction-readiness.json').read_text());self.assertEqual(len(d['factions']),6);self.assertEqual(d['regions'],36);self.assertEqual(d['legal_pairs'],51);self.assertFalse(any(f['runtime_enabled'] for f in d['factions']))
  for n in ['RuntimeProof.json','HumanRuntimeProof.json']:self.assertEqual((R/'Data/CampaignComposition'/n).read_bytes(),(E/'Local/Before/Data/CampaignComposition'/n).read_bytes())
 def test_regional_placement_bounds(self):
  d=json.loads((E/'regional-cues-plan.json').read_text());self.assertFalse(d['heightfield_changed']);self.assertTrue(d['anchors_unchanged']);self.assertLessEqual(d['viking_spur']['grade']['max_grade_deg'],22.1)
  f=json.loads((E/'foliage-r1.json').read_text());self.assertEqual(f['count_before'],f['count_after']);self.assertEqual(f['count_after'],18524)
if __name__=='__main__':unittest.main(verbosity=2)
