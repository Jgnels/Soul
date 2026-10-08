"""Admission boundaries that must hold independently of a successful binary launch."""
import hashlib,json,subprocess,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[2]
E=R/'Evidence/CleanFoundationAdmission-20261008'
class AdmissionContracts(unittest.TestCase):
 def test_excluded_drafts_are_preserved_but_cannot_compile(self):
  for row in json.loads((E/'excluded-drafts.json').read_text()):
   self.assertFalse((R/row['path']).exists())
   self.assertEqual(hashlib.sha256((R/row['preserved']).read_bytes()).hexdigest(),row['sha256'])
   self.assertEqual((R/row['preserved']).read_bytes(),(E/'Local/Before'/row['path']).read_bytes())
 def test_no_experimental_implementation_or_dependency(self):
  cpp='\n'.join(p.read_text(encoding='utf-8-sig') for p in (R/'Source').rglob('*') if p.suffix in {'.cpp','.h','.cs'})
  for forbidden in ['SoulCampaignExpansion::','SoulCampaignTerrain::World()','Data/CampaignWorldTerrain/','Data/CampaignWorldLocal/','Data/CampaignExpansion/']:
   self.assertFalse(forbidden in cpp,forbidden)
  self.assertFalse((R/'Source/Soul/Private/SoulCampaignWorldCapture.cpp').exists())
  # Tracked retained Crownstead already borrows one broadleaf mesh by this
  # historical asset path. Preserve that default behavior; no World loader is admitted.
  refs=[line.strip() for line in cpp.splitlines() if '/Game/SoulCampaignWorld/' in line]
  baseline=subprocess.check_output(['git','show','10aa400d75c6a4bf5db2d7f403122a6e3e17b323:Source/Soul/Private/SoulCampaignTerrain.cpp'],cwd=R,text=True)
  self.assertEqual(refs,[line.strip() for line in baseline.splitlines() if '/Game/SoulCampaignWorld/' in line])
  self.assertEqual(len(refs),1)
 def test_every_compiled_source_is_tracked_or_explicit_admission(self):
  tracked=set(subprocess.check_output(['git','ls-files','Source'],cwd=R,text=True).splitlines())
  actual={p.relative_to(R).as_posix() for p in (R/'Source').rglob('*') if p.suffix in {'.cpp','.h','.cs'}}
  self.assertEqual(actual-tracked,{'Source/Soul/Private/SoulCompositionTraversalQualification.cpp'}-tracked)
 def test_reference_and_composition_data_are_unchanged(self):
  checkpoint=json.loads((E/'checkpoint.json').read_text())
  # The expensive donor preservation inventory is rechecked at closeout; this
  # targeted boundary guards actual gameplay inputs during source admission.
  rows=[v for v in checkpoint['protected'] if Path(v['path']).resolve()==(R/'Data/CampaignComposition/presentation.json').resolve()]
  self.assertEqual(len(rows),1)
  self.assertEqual(hashlib.sha256((R/'Data/CampaignComposition/presentation.json').read_bytes()).hexdigest(),rows[0]['sha256'])
  for p in ['Data/CampaignComposition/RuntimeProof.json','Data/CampaignComposition/HumanRuntimeProof.json']:
   self.assertEqual((R/p).read_bytes().replace(b'\r\n',b'\n'),subprocess.check_output(['git','show','10aa400d75c6a4bf5db2d7f403122a6e3e17b323:'+p],cwd=R).replace(b'\r\n',b'\n'))
  for p in ['Data/soul_world_overmap_v1_20260922.json','Data/soul_campaign_start_states_v1_20260922.json']:
   self.assertEqual((R/p).read_bytes().replace(b'\r\n',b'\n'),subprocess.check_output(['git','show','HEAD:'+p],cwd=R).replace(b'\r\n',b'\n'))
 def test_candidate_remains_opt_in_and_default_rule_unchanged(self):
  rule=(R/'Source/Soul/Soul.Build.cs').read_bytes().replace(b'\r\n',b'\n')
  self.assertEqual(rule,subprocess.check_output(['git','show','10aa400d75c6a4bf5db2d7f403122a6e3e17b323:Source/Soul/Soul.Build.cs'],cwd=R).replace(b'\r\n',b'\n'))
  profile=json.loads((R/'Data/CampaignComposition/PackageProfile.json').read_text())
  self.assertEqual(profile['required_launch_args'],['-SoulComposition'])
  self.assertTrue(profile['default_map_unchanged'])
if __name__=='__main__':unittest.main(verbosity=2)
