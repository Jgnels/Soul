"""Guardrail tests: external actors must participate; forbidden edges must survive."""
from pathlib import Path
import tempfile
import unittest

from audit_packages import audit, references, package_file
from prepare_migration import build_plan, METHOD


class AuditTests(unittest.TestCase):
    def test_recursive_companions_and_contamination_chain(self):
        with tempfile.TemporaryDirectory() as folder:
            content = Path(folder)
            files = {
                "Environment/Pilot.umap": b"/Script/Engine\x00/Game/Environment/Nested\x00",
                "Environment/Nested.umap": b"/Script/Engine\x00",
                "__ExternalActors__/Environment/Pilot/A/Actor.uasset": b"/Game/Environment/Mesh\x00",
                "__ExternalObjects__/Environment/Pilot/B/Folder.uasset": b"/Script/Engine\x00",
                "__ExternalActors__/Environment/Nested/C/BadActor.uasset": b"/Game/Characters/NPC\x00",
                "Environment/Mesh.uasset": b"/Game/Environment/Material\x00",
                "Environment/Material.uasset": b"/Game/Environment/Missing\x00",
            }
            for name, data in files.items():
                path = content / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(data)
            result = audit(content, "/Game/Environment/Pilot")
            self.assertFalse(result["migration_permitted"])
            self.assertEqual(result["package_count"], 7)
            self.assertEqual(result["bytes"], sum(map(len, files.values())))
            self.assertEqual(result["forbidden"][0]["chain"], [
                "/Game/Environment/Pilot", "/Game/Environment/Nested",
                "/Game/__ExternalActors__/Environment/Nested/C/BadActor", "/Game/Characters/NPC"])
            self.assertEqual(result["missing"][0]["package"], "/Game/Environment/Missing")
            self.assertEqual(result["external"], ["/Script/Engine"])

    def test_ansi_and_utf16_are_not_concatenated(self):
        data = b"/Game/Environment/One\x00tail /Script/Titan\x00" + "/Game/Environment/Two\x00".encode("utf-16-le")
        self.assertEqual(references(data), ["/Game/Environment/One", "/Game/Environment/Two", "/Script/Titan"])

    def test_path_escape_is_rejected(self):
        with self.assertRaises(ValueError):
            package_file(Path("Content"), "/Game/../../Other")

    def test_closure_budget_fails_closed(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / "Test.uasset").write_bytes(b"/Game/Other\x00/Script/Engine\x00")
            with self.assertRaises(RuntimeError):
                audit(root, "/Game/Test", limit=1)

    def test_binary_preflight_cannot_authorize_migration(self):
        with self.assertRaisesRegex(ValueError, "native UE AssetRegistry"):
            build_plan({"method": "binary strings", "migration_permitted": True}, Path("donor"), "anything", Path("target"))

    def test_native_report_cannot_silently_drop_contamination(self):
        seed = "/Game/Environment/Sulfur/Level_Instances/LI_Sulfur_BanditRestOutpost"
        report = {"method": METHOD, "donors": [{"seed": seed, "blocked": ["/Game/Characters/NPC"], "missing": []}]}
        with self.assertRaisesRegex(ValueError, "donor remains stopped"):
            build_plan(report, Path("donor"), seed, Path("target"))

    def test_clean_plan_validates_nested_companions_and_hashes(self):
        seed = "/Game/Environment/Clifftop/Level_Instance/IL_Clifftop_Mine_TownGrid"
        with tempfile.TemporaryDirectory() as folder:
            donor, target = Path(folder) / "donor", Path(folder) / "target"
            target.mkdir()
            root = donor / "Content"
            source = root / (seed[6:] + ".umap")
            source.parent.mkdir(parents=True)
            source.write_bytes(b"map")
            row = dict(package=seed, file=str(source), bytes=3, classes=["/Script/Engine.World"],
                       hard_query_found=True, all_query_found=True, hard=[], soft=[], companions=[])
            report = dict(method=METHOD, donor_content=str(root), donors=[dict(seed=seed,
                blocked=[], missing=[], external=["/Script/Engine"], packages=[row])])
            plan = build_plan(report, donor, seed, target)
            self.assertEqual(plan["bytes"], 3)
            self.assertEqual(len(plan["files"][0]["sha256"]), 64)
            self.assertEqual(list(target.iterdir()), [])  # planning never copies
            companion = root / "__ExternalActors__" / seed[6:] / "A/Actor.uasset"
            companion.parent.mkdir(parents=True)
            companion.write_bytes(b"actor")
            with self.assertRaisesRegex(ValueError, "Companion tree"):
                build_plan(report, donor, seed, target)


    def test_exact_foliage_support_requires_native_passive_classes(self):
        seed = "/Game/Environment/Clifftop/Level_Instance/IL_Clifftop_Mine_TownGrid"
        mpc = "/Game/Blueprint/FoliageInteraction/MPC_Player"
        rt = "/Game/Blueprint/FoliageInteraction/RT_Player"
        with tempfile.TemporaryDirectory() as folder:
            donor, target = Path(folder) / "donor", Path(folder) / "target"
            target.mkdir()
            root = donor / "Content"
            specs = [
                (seed, ".umap", ["/Script/Engine.World"], [mpc, rt]),
                (mpc, ".uasset", ["/Script/Engine.MaterialParameterCollection"], []),
                (rt, ".uasset", ["/Script/Engine.CanvasRenderTarget2D"], []),
            ]
            rows = []
            for package, suffix, classes, hard in specs:
                source = root / (package[6:] + suffix)
                source.parent.mkdir(parents=True, exist_ok=True)
                source.write_bytes(package.encode("utf-8"))
                rows.append(dict(package=package, file=str(source), bytes=source.stat().st_size,
                                 classes=classes, hard_query_found=True, all_query_found=True,
                                 hard=hard, soft=[], companions=[]))
            report = dict(method=METHOD, donor_content=str(root), donors=[dict(
                seed=seed, blocked=[], missing=[], external=["/Script/Engine"], packages=rows)])
            plan = build_plan(report, donor, seed, target)
            self.assertEqual(plan["package_count"], 3)
            rows[1]["classes"] = ["/Script/Engine.Blueprint"]
            with self.assertRaisesRegex(ValueError, "Passive support class mismatch"):
                build_plan(report, donor, seed, target)


    def test_external_actor_query_gap_requires_class_package_in_closure(self):
        seed = "/Game/Environment/Clifftop/Level_Instance/IL_Clifftop_Mine_TownGrid"
        actor = "/Game/__ExternalActors__/Environment/Clifftop/Level_Instance/IL_Clifftop_Mine_TownGrid/A/Actor"
        actor_class = "/Game/Environment/Clifftop/Actors/BP_Wall"
        with tempfile.TemporaryDirectory() as folder:
            donor, target = Path(folder) / "donor", Path(folder) / "target"
            target.mkdir()
            root = donor / "Content"
            specs = [
                (seed, ".umap", ["/Script/Engine.World"], True, True, [], [actor]),
                (actor, ".uasset", [actor_class + ".BP_Wall_C"], False, False, [], []),
                (actor_class, ".uasset", ["/Script/Engine.Blueprint"], True, True, [], []),
            ]
            rows = []
            for package, suffix, classes, hard_ok, all_ok, hard, comps in specs:
                source = root / (package[6:] + suffix)
                source.parent.mkdir(parents=True, exist_ok=True)
                source.write_bytes(package.encode("utf-8"))
                rows.append(dict(package=package, file=str(source), bytes=source.stat().st_size,
                                 classes=classes, hard_query_found=hard_ok, all_query_found=all_ok,
                                 hard=hard, soft=[], companions=comps,
                                 class_dependencies=[actor_class] if package == actor else [],
                                 serialized_read_complete=True, serialized_dependencies=[]))
            report = dict(method=METHOD, donor_content=str(root), donors=[dict(
                seed=seed, blocked=[], missing=[], external=["/Script/Engine"], packages=rows)])
            plan = build_plan(report, donor, seed, target)
            self.assertEqual(plan["package_count"], 3)
            report["donors"][0]["packages"] = rows[:2]
            with self.assertRaisesRegex(ValueError, "Class package lacks complete native Blueprint queries"):
                build_plan(report, donor, seed, target)

    def test_unreviewed_plugin_dependency_stops_plan(self):
        seed = "/Game/Environment/Sulfur/Level_Instances/LI_Sulfur_BanditRestOutpost"
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            report = dict(method=METHOD, donor_content=str(root / "Content"), donors=[dict(
                seed=seed, blocked=[], missing=[], external=["/Script/Titan"], packages=[])])
            with self.assertRaisesRegex(ValueError, "External dependencies"):
                build_plan(report, root, seed, root.parent / "target")


if __name__ == "__main__":
    unittest.main()
