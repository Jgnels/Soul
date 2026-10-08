import os
from pathlib import Path
import tempfile
import unittest
from staged_copy_timestamps import refresh_stage_copies

class StageCopyPreservation(unittest.TestCase):
 def test_freshens_copy_without_mutating_linked_source(self):
  with tempfile.TemporaryDirectory() as name:
   root=Path(name);stage=root/'stage';stage.mkdir();source=root/'owned-cooked.uasset';source.write_bytes(b'cooked-owned');os.utime(source,(1000,1000));linked=stage/'linked.uasset';os.link(source,linked)
   copy=stage/'descriptor.uplugin';copy.write_bytes(b'licensed-descriptor');os.utime(copy,(1000,1000))
   receipt=refresh_stage_copies(stage)
   self.assertEqual(source.stat().st_mtime,1000);self.assertEqual(linked.stat().st_mtime,1000)
   self.assertGreater(copy.stat().st_mtime,1000);self.assertEqual(copy.read_bytes(),b'licensed-descriptor');self.assertEqual(source.read_bytes(),b'cooked-owned')
   self.assertEqual(receipt['linked_files_skipped'],1);self.assertEqual(receipt['copied_files_refreshed'],1)

if __name__=='__main__':unittest.main(verbosity=2)
