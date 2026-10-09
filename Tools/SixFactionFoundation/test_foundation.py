"""Data and isolation contracts; native tests exercise actual C++ state/save behavior."""
from pathlib import Path
import json,unittest,sys,subprocess
R=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(R/'Tools/ProductionContinuation'))
from cook_profile import verify_compatible_cook_profile
from roster_stage import FACTION_ROOTS
class Contracts(unittest.TestCase):
 def test_canonical_overlay_not_rewritten(self):
  cfg=json.loads((R/'Data/CampaignComposition/SixFactionRuntimeProof.json').read_text())
  starts=json.loads((R/'Data/soul_campaign_start_states_v1_20260922.json').read_text())['scenarios'][cfg['canonical_start_scenario']]
  self.assertEqual(set(starts['factions']),{'humans','dwarves','orcs','vikings','nature','dark'})
  self.assertEqual(len(starts['region_owners']),12);self.assertEqual(len(starts['neutral_regions']),24)
  self.assertEqual(cfg['start_state']['owners'],{})
  self.assertNotIn('hostile_garrisons',cfg)
 def test_topology(self):
  geo=json.loads((R/'Data/soul_world_overmap_v1_20260922.json').read_text())
  cfg=json.loads((R/'Data/CampaignComposition/SixFactionRuntimeProof.json').read_text())
  self.assertEqual(len(geo['nodes']),36);self.assertEqual(len(geo['edges']),51)
  self.assertEqual(set(cfg['start_state']['region_ids']),{n['id'] for n in geo['nodes']})
 def test_existing_slots_preserved_and_new_axe_requires_cook(self):
  before=json.loads(subprocess.check_output(['git','show','41f604475e94905ff9afc2cb0e55148e0412618b:Data/CampaignComposition/PackageProfile.json'],cwd=R,text=True))
  after=json.loads((R/'Data/CampaignComposition/PackageProfile.json').read_text())
  with self.assertRaises(ValueError):verify_compatible_cook_profile(before,after)
  self.assertEqual(set(after['cook_roots'])-set(before['cook_roots']),set(FACTION_ROOTS))
  self.assertEqual(set(before['cook_roots'])-set(after['cook_roots']),set())
  for k,v in before['save_slots'].items():self.assertEqual(after['save_slots'][k],v)
  self.assertEqual(len(set(after['save_slots'].values())),len(after['save_slots']))
 def test_candidate_dependency_only(self):
  source=(R/'Source/Soul/Soul.Build.cs').read_text();name='Data/CampaignComposition/SixFactionRuntimeProof.json'
  self.assertEqual(source.count(name),1);self.assertGreater(source.index(name),source.index('Target.Name == "SoulComposition"'))
 def test_viking_fixture_uses_actual_canonical_pass(self):
  d=json.loads((R/'Data/CampaignComposition/VikingRuntimeProof.json').read_text())
  self.assertEqual((d['enemy_faction'],d['enemy_unit_id']),('vikings','viking_axe_warrior'))
  self.assertEqual(d['start_state']['player_start_region'],'mountain_shrine')
  self.assertEqual(d['start_state']['enemy_primary_region'],'viking_snow_pass')
  geo=json.loads((R/'Data/soul_world_overmap_v1_20260922.json').read_text())
  self.assertIn(frozenset({'mountain_shrine','viking_snow_pass'}),{frozenset((x['a'],x['b'])) for x in geo['edges']})
  self.assertEqual(d['hostile_garrisons'],{'viking_snow_pass':30})
 def test_no_ai_activation(self):
  s=(R/'Source/Soul/Private/SoulSixFactionCampaign.cpp').read_text()
  self.assertNotIn('FSoulStrategyAI::Choose',s)
  self.assertIn('FSoulWorldRules::CanMove',s);self.assertIn('FSoulCampaignRules::SpendAction',s)
if __name__=='__main__':unittest.main()
