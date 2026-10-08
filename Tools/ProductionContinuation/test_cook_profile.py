import copy,json,unittest
from pathlib import Path
from cook_profile import verify_compatible_cook_profile
R=Path(__file__).resolve().parents[2]
class CookProfileContracts(unittest.TestCase):
 def setUp(self):
  self.before=json.loads((R/'Data/CampaignComposition/PackageProfile.json').read_text());self.after=copy.deepcopy(self.before)
 def test_identity(self):self.assertFalse(verify_compatible_cook_profile(self.before,self.after)['fresh_asset_cook'])
 def test_new_isolated_fixture(self):
  self.after['exact_additional_runtime_files'].append('Data/CampaignComposition/TestFixture.json');self.after['save_slots']['new_proof']='Soul.Composition3500.TestProof'
  self.assertEqual(verify_compatible_cook_profile(self.before,self.after)['added_loose_fixtures'],['Data/CampaignComposition/TestFixture.json'])
 def test_changed_asset_roots_rejected(self):
  self.after['cook_roots'].append('/Game/Uncooked/Map')
  with self.assertRaises(ValueError):verify_compatible_cook_profile(self.before,self.after)
 def test_changed_default_rejected(self):
  self.after['default_map_unchanged']=False
  with self.assertRaises(ValueError):verify_compatible_cook_profile(self.before,self.after)
 def test_existing_slot_migration_rejected(self):
  self.after['save_slots']['default']='Soul.Composition3500.TestProof'
  with self.assertRaises(ValueError):verify_compatible_cook_profile(self.before,self.after)
 def test_experiment_or_asset_payload_rejected(self):
  for path in ['Data/CampaignExpansion/New.json','Data/CampaignComposition/New.uasset','Data/CampaignComposition/../bad.json']:
   d=copy.deepcopy(self.before);d['exact_additional_runtime_files'].append(path)
   with self.assertRaises(ValueError):verify_compatible_cook_profile(self.before,d)
if __name__=='__main__':unittest.main(verbosity=2)
