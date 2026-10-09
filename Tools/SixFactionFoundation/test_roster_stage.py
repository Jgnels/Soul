from pathlib import Path
import tempfile,json,unittest,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'ProductionContinuation'))
from roster_stage import append_viking_cook,append_faction_cook,PACKAGES,PREFIX,NATURE_PACKAGES,FACTION_ROOTS
from stage_manifest import verify_manifest_presence
class AdditiveCookTests(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.root=Path(self.tmp.name);self.cook=self.root/'cook';self.stage=self.root/'stage';self.logs=self.root/'Diagnostics';(self.logs/'UAT').mkdir(parents=True)
  self.manifest=self.logs/'UAT/Manifest_UFSFiles_Win64.txt';self.manifest.write_text('')
  (self.logs/'UAT/Manifest_NonUFSFiles_Win64.txt').write_text('')
  self.receipt=self.logs/'receipt.json'
  for p in PACKAGES:
   for ext in ['.uasset','.uexp']:
    f=self.cook/(PREFIX/p).with_suffix(ext);f.parent.mkdir(parents=True,exist_ok=True);f.write_bytes((p+ext).encode())
 def tearDown(self):self.tmp.cleanup()
 def test_exact_addition_and_after_run_tamper_detection(self):
  rows=append_viking_cook(self.cook,self.stage,self.manifest);self.assertEqual(len(rows),12);self.assertEqual(self.manifest.read_text(),'')
  self.receipt.write_text(json.dumps({'supplemental_cooked_files':rows}))
  self.assertEqual(verify_manifest_presence(self.receipt,self.stage),12)
  (self.stage/rows[0]['relative']).write_bytes(b'changed')
  with self.assertRaises(ValueError):verify_manifest_presence(self.receipt,self.stage)
 def test_never_overwrites_base_packages(self):
  p=self.stage/(PREFIX/PACKAGES[0]).with_suffix('.uasset');p.parent.mkdir(parents=True);p.write_bytes(b'base')
  with self.assertRaises(AssertionError):append_viking_cook(self.cook,self.stage,self.manifest)
  self.assertEqual(p.read_bytes(),b'base')
 def test_missing_cooked_dependency_rejected(self):
  (self.cook/(PREFIX/PACKAGES[0]).with_suffix('.uasset')).unlink()
  with self.assertRaises(AssertionError):append_viking_cook(self.cook,self.stage,self.manifest)
 def test_nature_addition_is_bounded_and_keeps_base_registry(self):
  for name in NATURE_PACKAGES:
   f=self.cook/Path(name).with_suffix('.uasset');f.parent.mkdir(parents=True,exist_ok=True);f.write_bytes(name.encode())
  registry=self.stage/'Soul/AssetRegistry.bin';registry.parent.mkdir(parents=True);registry.write_bytes(b'unchanged base registry')
  rows=append_faction_cook(self.cook,self.stage,self.manifest)
  self.assertEqual(len(rows),12+len(NATURE_PACKAGES));self.assertEqual(registry.read_bytes(),b'unchanged base registry')
  self.assertEqual(len(FACTION_ROOTS),15)
  self.assertTrue(all('/Animals_Warrior_Pack/' in x or '/Viking_Ulf/' in x for x in FACTION_ROOTS))
  self.assertEqual(len({x['relative'] for x in rows}),len(rows))
 def test_nature_never_overwrites_existing_base_package(self):
  for name in NATURE_PACKAGES:
   f=self.cook/Path(name).with_suffix('.uasset');f.parent.mkdir(parents=True,exist_ok=True);f.write_bytes(name.encode())
  existing=self.stage/Path(NATURE_PACKAGES[0]).with_suffix('.uasset');existing.parent.mkdir(parents=True);existing.write_bytes(b'protected')
  with self.assertRaises(AssertionError):append_faction_cook(self.cook,self.stage,self.manifest)
  self.assertEqual(existing.read_bytes(),b'protected')
if __name__=='__main__':unittest.main()
