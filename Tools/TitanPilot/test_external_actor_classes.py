"""Regression coverage for the bounded World Partition Blueprint-class fallback."""
import hashlib
from pathlib import Path
import tempfile
import unittest

from prepare_migration import build_plan, METHOD
from audit_packages import SEEDS


class ExternalActorClassTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.donor = Path(self.temp.name) / "donor"
        self.target = Path(self.temp.name) / "soul"
        self.target.mkdir()
        self.seed = SEEDS["ClifftopMine"]
        self.actor = "/Game/__ExternalActors__/" + self.seed[6:] + "/A/BC/Actor"
        self.bp = "/Game/Environment/Clifftop/BP_Pillar"
        self.mesh = "/Game/Environment/Clifftop/Mesh"
        specs = [
            (self.seed, ".umap", "/Script/Engine.World", [], [self.actor], True),
            (self.actor, ".uasset", self.bp + ".BP_Pillar_C", [], [], False),
            (self.bp, ".uasset", "/Script/Engine.Blueprint", [self.mesh], [], True),
            (self.mesh, ".uasset", "/Script/Engine.StaticMesh", [], [], True),
        ]
        self.rows = []
        for package, ext, cls, hard, companions, complete in specs:
            source = self.donor / "Content" / (package[6:] + ext)
            source.parent.mkdir(parents=True, exist_ok=True)
            source.write_bytes(package.encode())
            self.rows.append(dict(package=package, file=str(source), bytes=source.stat().st_size,
                classes=[cls], hard=hard, soft=[], companions=companions,
                class_dependencies=[self.bp] if package == self.actor else [],
                serialized_read_complete=not complete, serialized_dependencies=[],
                hard_query_found=complete, all_query_found=complete))
        self.report = dict(method=METHOD, donor_content=str(self.donor / "Content"), donors=[dict(
            seed=self.seed, packages=self.rows, blocked=[], missing=[], external=[])])

    def plan(self):
        return build_plan(self.report, self.donor, self.seed, self.target)

    def test_valid_fallback_preserves_actor_class_mesh_and_hash(self):
        plan = self.plan()
        self.assertEqual(plan["package_count"], 4)
        actor = next(f for f in plan["files"] if f["package"] == self.actor)
        self.assertEqual(actor["sha256"], hashlib.sha256(self.actor.encode()).hexdigest())
        self.assertEqual(plan["actor_class_fallbacks"], [dict(package=self.actor, class_package=self.bp)])

    def test_unknown_native_or_non_environment_classes_fail(self):
        for classes in ([], ["/Script/Engine.Actor"], ["/Script/Titan.TitanActor"],
                        ["/Game/Blueprint/Bad.Bad_C"], [self.bp + ".Unknown_C"],
                        [self.bp + ".BP_Pillar_C", "/Script/Engine.Actor"]):
            with self.subTest(classes=classes):
                self.rows[1]["classes"] = classes
                with self.assertRaises(ValueError): self.plan()

    def test_class_package_must_exist_and_have_complete_native_queries(self):
        for field in ("hard_query_found", "all_query_found"):
            with self.subTest(field=field):
                self.rows[2][field] = False
                with self.assertRaises(ValueError): self.plan()
                self.rows[2][field] = True
        self.rows.remove(self.rows[2])
        with self.assertRaises(ValueError): self.plan()

    def test_class_must_be_native_blueprint_asset(self):
        self.rows[2]["classes"] = ["/Script/Engine.StaticMesh"]
        with self.assertRaises(ValueError): self.plan()

    def test_explicit_class_edge_and_companion_membership_required(self):
        self.rows[1]["class_dependencies"] = []
        with self.assertRaises(ValueError): self.plan()
        self.rows[1]["class_dependencies"] = [self.bp]
        self.rows[0]["companions"] = []
        with self.assertRaises(ValueError): self.plan()

    def test_blueprint_plugin_and_gameplay_dependencies_still_fail(self):
        for dependency in ("/Script/Titan", "/UnknownPlugin/Asset", "/Game/Characters/NPC"):
            with self.subTest(dependency=dependency):
                self.rows[2]["hard"] = [dependency]
                with self.assertRaises(ValueError): self.plan()

    def test_external_object_or_regular_package_does_not_get_exception(self):
        for package in (self.actor.replace("__ExternalActors__", "__ExternalObjects__"), self.bp):
            with self.subTest(package=package):
                self.rows[1]["package"] = package
                self.rows[0]["companions"] = [package]
                with self.assertRaises(ValueError): self.plan()

    def test_actor_file_change_or_destination_collision_fails(self):
        target = self.target / "Content" / (self.actor[6:] + ".uasset")
        target.parent.mkdir(parents=True)
        target.write_bytes(b"existing")
        with self.assertRaises(ValueError): self.plan()

    def test_serialized_instance_read_required(self):
        self.rows[1]["serialized_read_complete"] = False
        with self.assertRaisesRegex(ValueError, "serialized instance"): self.plan()

    def test_instance_material_override_recurses_and_checks_forbidden_plugins(self):
        self.rows[2]["hard"] = []
        self.rows[1]["serialized_dependencies"] = [self.mesh]
        self.assertEqual(self.plan()["package_count"], 4)
        for dep in ("/Game/Environment/MissingOverride", "/Script/Titan", "/UnknownPlugin/Asset", "/Game/Characters/NPC"):
            self.rows[1]["serialized_dependencies"] = [self.mesh, dep]
            with self.subTest(dep=dep), self.assertRaises(ValueError): self.plan()

    def test_prior_receipt_requires_unchanged_hashes_and_subset(self):
        plan = self.plan()
        actor = next(r for r in plan["files"] if r["package"] == self.actor)
        target = self.target / "Content" / actor["relative_file"]
        target.parent.mkdir(parents=True)
        target.write_bytes(self.actor.encode())
        receipt = dict(status="VERIFIED", seed=self.seed, files=[actor])
        updated = build_plan(self.report, self.donor, self.seed, self.target, receipt)
        self.assertEqual(updated["verified_existing_files"], [actor])
        target.write_bytes(b"changed")
        with self.assertRaises(ValueError):
            build_plan(self.report, self.donor, self.seed, self.target, receipt)
        target.unlink()
        Path(self.rows[1]["file"]).write_bytes(b"changed")
        with self.assertRaises(ValueError): self.plan()


if __name__ == "__main__":
    unittest.main()
