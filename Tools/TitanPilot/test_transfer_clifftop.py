import hashlib
from pathlib import Path
import tempfile
import unittest
from audit_packages import SEEDS
from transfer_clifftop import transfer


class TransferTests(unittest.TestCase):
    def test_hash_verification_and_no_overwrite(self):
        with tempfile.TemporaryDirectory() as tmp:
            donor, soul = Path(tmp) / "donor", Path(tmp) / "soul"
            source = donor / "Content/Environment/Proof.uasset"
            source.parent.mkdir(parents=True)
            soul.mkdir()
            source.write_bytes(b"asset")
            row = dict(relative_file="Environment/Proof.uasset", bytes=5,
                       sha256=hashlib.sha256(b"asset").hexdigest())
            plan = dict(seed=SEEDS["ClifftopMine"], files=[row], bytes=5)
            receipt = transfer(plan, donor, soul, soul / "receipt.json")
            self.assertEqual(receipt["status"], "VERIFIED")
            self.assertEqual((soul / "Content/Environment/Proof.uasset").read_bytes(), b"asset")
            with self.assertRaisesRegex(ValueError, "collision"):
                transfer(plan, donor, soul, soul / "receipt2.json")

    def test_bad_source_hash_and_sulfur_refused_before_copy(self):
        with tempfile.TemporaryDirectory() as tmp:
            donor, soul = Path(tmp) / "donor", Path(tmp) / "soul"
            source = donor / "Content/Environment/Proof.uasset"
            source.parent.mkdir(parents=True)
            source.write_bytes(b"asset")
            soul.mkdir()
            plan = dict(seed=SEEDS["ClifftopMine"], bytes=5, files=[dict(
                relative_file="Environment/Proof.uasset", bytes=5, sha256="wrong")])
            with self.assertRaises(ValueError): transfer(plan, donor, soul, soul / "receipt.json")
            self.assertEqual(list(soul.iterdir()), [])
            plan["seed"] = SEEDS["SulfurBandit"]
            with self.assertRaises(ValueError): transfer(plan, donor, soul, soul / "receipt.json")

    def test_supplement_reuses_exact_files_without_overwriting(self):
        with tempfile.TemporaryDirectory() as tmp:
            donor, soul = Path(tmp) / "donor", Path(tmp) / "soul"
            soul.mkdir()
            rows = []
            for name in ("Existing", "New"):
                relative = "Environment/" + name + ".uasset"
                src = donor / "Content" / relative
                src.parent.mkdir(parents=True, exist_ok=True)
                src.write_bytes(name.encode())
                rows.append(dict(relative_file=relative, bytes=len(name), sha256=hashlib.sha256(name.encode()).hexdigest()))
            existing = soul / "Content" / rows[0]["relative_file"]
            existing.parent.mkdir(parents=True)
            existing.write_bytes(b"Existing")
            before = existing.stat().st_mtime_ns
            plan = dict(seed=SEEDS["ClifftopMine"], files=rows, bytes=11, verified_existing_files=[rows[0]])
            result = transfer(plan, donor, soul, soul / "receipt.json")
            self.assertEqual(result["new_file_count"], 1)
            self.assertEqual(result["file_count"], 2)
            self.assertEqual(existing.stat().st_mtime_ns, before)
            existing.write_bytes(b"modified")
            with self.assertRaisesRegex(ValueError, "destination changed"):
                transfer(plan, donor, soul, soul / "receipt2.json")


if __name__ == "__main__":
    unittest.main()
