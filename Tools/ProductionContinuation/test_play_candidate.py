"""Review launch boundary: explicit verified binary, fixture and separate save slots."""
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import play_candidate as launch

class ReviewLaunchBoundary(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
  self.root=Path(self.temp.name);self.e=self.root/'Evidence/Current';self.s=self.root/'Stage/Windows'
  self.receipt=self.e/'Local/stage/Diagnostics/receipt.json';self.receipt.parent.mkdir(parents=True)
  self.file('Soul/Binaries/Win64/SoulComposition.exe',b'build')
  self.file('Engine/Plugins/Runtime/HDRIBackdrop/HDRIBackdrop.uplugin',b'{}')
  data=[]
  for name in ['RuntimeProof','HumanRuntimeProof','OrcRuntimeProof']:
   path='Data/CampaignComposition/'+name+'.json';p=self.file('Soul/'+path,b'{}');data.append({'path':path,'sha256':launch.digest(p)})
  asset=self.file('Soul/Content/SoulCampaignComposition/L_Composition_3500_r2.umap',b'cooked')
  self.receipt.write_text(json.dumps({'pass':True,'stage':str(self.s.parent),'binary_sha256':launch.digest(self.s/'Soul/Binaries/Win64/SoulComposition.exe')}))
  (self.e/'staged-isolation-verification.json').write_text(json.dumps({'pass':True,'stage':str(self.s),'additional_data':data}))
  (self.receipt.parent/'prelinked-cooked-files.json').write_text(json.dumps([{'relative':asset.relative_to(self.s).as_posix(),'sha256':launch.digest(asset)}]))
  manifests=self.receipt.parent/'UAT';manifests.mkdir()
  (manifests/'Manifest_UFSFiles_Win64.txt').write_text(asset.relative_to(self.s).as_posix()+'\tstamp\n')
  (manifests/'Manifest_NonUFSFiles_Win64.txt').write_text('Engine/Plugins/Runtime/HDRIBackdrop/HDRIBackdrop.uplugin\tstamp\n')
  self.mock=patch.object(launch,'R',self.root);self.mock.start();self.addCleanup(self.mock.stop)
 def file(self,name,value):
  p=self.s/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(value);return p
 def plan(self,**kw):return launch.prepare(self.receipt,kw.pop('human',False),kw.pop('minutes',20),evidence_root=self.e,**kw)
 def test_separate_slots_and_explicit_proof_flags(self):
  for human,orc,name,flag in [(False,False,'Founder',None),(True,False,'HumanProof','-SoulHumanSettlementProof'),(False,True,'OrcProof','-SoulOrcMatchupProof')]:
   p=self.plan(human=human,orc=orc);self.assertEqual(p['save_slot'],'Soul.Composition3500.'+name)
   self.assertEqual(Path(p['persistent_isolated_user_directory']).name,name)
   self.assertFalse(p['automatic_inputs']);self.assertFalse(p['promotion']);self.assertEqual(p['thermal_cutoff_c'],85)
   if flag:self.assertIn('--ue-arg='+flag,p['command'])
 def test_binary_change_rejected(self):
  self.file('Soul/Binaries/Win64/SoulComposition.exe',b'different')
  with self.assertRaisesRegex(ValueError,'executable'):self.plan()
 def test_missing_orc_fixture_rejected(self):
  p=self.e/'staged-isolation-verification.json';d=json.loads(p.read_text());d['additional_data'].pop();p.write_text(json.dumps(d))
  with self.assertRaisesRegex(ValueError,'fixture'):self.plan(orc=True)
 def test_cooked_candidate_change_rejected(self):
  self.file('Soul/Content/SoulCampaignComposition/L_Composition_3500_r2.umap',b'changed')
  with self.assertRaisesRegex(ValueError,'cooked asset'):self.plan()
 def test_conflicting_fixture_and_unbounded_duration_rejected(self):
  with self.assertRaises(ValueError):self.plan(human=True,orc=True)
  with self.assertRaises(ValueError):self.plan(minutes=60)
 def test_missing_recorded_resource_rejected(self):
  (self.s/'Engine/Plugins/Runtime/HDRIBackdrop/HDRIBackdrop.uplugin').unlink()
  with self.assertRaisesRegex(ValueError,'resources are missing'):self.plan()
 def test_mismatched_stage_rejected(self):
  p=self.e/'staged-isolation-verification.json';d=json.loads(p.read_text());d['stage']=str(self.root/'OldStage');p.write_text(json.dumps(d))
  with self.assertRaisesRegex(ValueError,'isolation'):self.plan()

if __name__=='__main__':unittest.main(verbosity=2)
