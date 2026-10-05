# Soul Titan pilot tools

Scope: `codex/soul-titan-pilot-20261005` in the isolated Soul worktree. Titan remains read-only. Clifftop is qualified; Sulfur remains stopped.

The final result and limits are in [HANDOFF.md](../../Evidence/TitanPilot-20261005/HANDOFF.md). Exact commands, including build, audit, transfer, cook and local packaged runtime, are in [COMMANDS.md](../../Evidence/TitanPilot-20261005/COMMANDS.md).

- `audit_packages.py` is a read-only binary string census. Historical strings alone do not establish live dependencies.
- `SoulTitanAudit` scans native AssetRegistry hard/soft edges, concrete classes, complete external actor/object companion trees and external-actor serialized imports/soft references without loading or saving donor UObjects.
- `prepare_migration.py` validates recursive closure, forbidden/plugin edges, files, companions, hashes, the 1 GiB cap and 15 GiB reserve. The sole incomplete-query exception requires a concrete environment Blueprint actor class whose own native queries are complete, plus complete serialized instance references. Unknown classes still fail. See [MIGRATION_DESIGN.md](../../Evidence/TitanPilot-20261005/MIGRATION_DESIGN.md).
- `transfer_clifftop.py` copies only exact Clifftop packages with exclusive file creation and hash checks. Existing files require a matching prior receipt and are never overwritten.
- `Run-TitanPilot.ps1` guards the worktree/branch, storage and Studio Unreal mutex. Stages: BuildEditor, Audit, CreatePilot, Runtime, BuildGame, CookWindows, PackageWindows, PackagedRuntime. It never terminates another worker's process.
- `SoulTitanPilotBuild` creates the Soul-owned world/navigation and refuses an existing map. Donor packages are not resaved.
- `review_unreal_log.py` rejects unresolved loads. The exact known editor HLOD-template warning is recorded separately only if its module subsequently loads; other missing references remain failures.
- `qualify_evidence.py` checks destination hashes, eight native stage receipts, map check and required editor-hosted/packaged runtime markers.
- `Play-TitanPilot.bat` opens interactive traversal: WASD/mouse/Space and existing Soul F5/F9 RBSave handlers. Automatic exit requires the separate proof flag.

Run all regressions: `python -m unittest discover -s Tools/TitanPilot -p 'test_*.py' -v` (27 passing).

The opt-in `Config/Custom/TitanPilot/DefaultGame.ini` cook profile excludes unused Landmass editor brush materials while retaining missing-dependency validation. It does not change the normal campaign cook. Local packaging is not a release/deployment.

Preserve receipts/map backups for rollback. Revert checkpoint commits in dependency order if retiring the pilot; do not reset/clean unrelated work or edit the donor.