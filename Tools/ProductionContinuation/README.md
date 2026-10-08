# Production continuation tools

These tools continue the qualified 2026-10-08 worker. They do not establish a new campaign authority or promote the candidate.

## Reproducible checks

- `test_continuation.py`: changed presentation, package profile, save namespace and inactive faction-groundwork contracts.
- `run_native.py --run <fresh-name> [--composition]`: current SoulEditor binary, two CampaignWorld automation tests; one UE process at a time.
- `qualify_runtime.py input|load|development --run <fresh-name>`: only the regressions required by save-slot isolation. Load requires `--source` pointing to the new input run. Development uses the existing normal-input two-domain controls proof; it does not visit or rebuild a city.
- `qualify_runtime.py profile --run <fresh-name>`: one authorized corrected 60-second native-1080p measurement, 20-second warmup, uncapped, existing 85 C guard. Do not repeat it merely to improve a result.
- `verify_package_receipt.py`: inspect the actual `SoulComposition.target` NonUFS dependencies and reject experimental payloads.
- `cook_candidate.py --scope runtime --output <fresh-evidence-directory> --minutes 60 [--os-temp-cook]`: full explicit 216-root runtime cook, NullRHI, no config/default-map edits. The default `candidate` scope only cooks the map/dependency closure and must not be called a full-game cook. `--os-temp-cook` allocates a fresh OS-temp payload directory, recorded in the receipt, while preserving all prior outputs. Stops at 85 C, 8 GiB disk headroom, 4 GiB host commit headroom, 20 GiB process commit or time cap. A successful cook still requires actual staging and a game-executable launch; it is not packaged-runtime acceptance.
- `stage_candidate.py --cook <cook-evidence-directory> --run <fresh-name> [--loose-hardlink]`: requires a passing full-runtime cook and verified target receipt. The optional local loose stage prelinks identical same-volume cooked files and verifies their hashes after UAT; no distributable archive or promotion is implied. It does not modify donor packages.
- `qualify_runtime.py input|load|battle|authored --run <fresh-name> --stage-receipt <verified-stage-receipt>`: launch the real staged `SoulComposition.exe` with the existing guard. This mode rejects another performance attempt.
- `faction_readiness.py`: derived validation of the existing six-faction sandbox data; never enables it or changes ownership.
- `snapshot_delta.py`: preserve this continuation's exact changes relative to the inherited worker baseline.
- `make_gallery.py`: real-image paired review, including rejected iterations explicitly marked as such.

## One-shot authoring history

`checkpoint.py` is the completed start checkpoint and must not be rerun. `smooth_roads.py` and `smooth_crownspine.py` produce bounded curves against the frozen native heightfield. `prepare_roads.py`, `bind_roads.py` and `export_runtime.py` publish the same accepted routes to surface masks and the existing presentation adapter.

`foliage.py`, `place_cues.py` and `place_dark_cues.py` are authoring history, not startup code. Do not rerun them blindly. Foliage preserves 18,524 XY placements; donor meshes are never saved. The two Dark remnants created by `place_dark_cues.py` were rejected after rendered review and left hidden/collision-disabled. Their placement file is not accepted art authority.

Final candidate: `/Game/SoulCampaignComposition/L_Composition_3500_r2`. The retained reference, frozen heightfield, authored cities and donor packages remain protected. All licensed payloads and large captures/cooks remain local-only.

## Manual cooked playtest

`play_candidate.py --dry-run` validates the exact staged executable, four presentation payloads, and 94 cooked candidate files without opening a game or creating a save directory. Run `play_candidate.py` for an ordinary interactive review, or add `--human-proof` for the already-qualified Human construction/visit/battle fixture. `--minutes` is bounded to 1–30 (default 20). The existing guard remains active at 85 C; review is capped at 20 FPS and must never be called a performance test. No automated input or autobattle flag is supplied.

Each mode keeps its own persistent local user directory beneath `Saved/CompositionPlaytest/` and the existing isolated RBSave slot. It does not load, overwrite or migrate the retained save. The tool depends on the recorded local temporary stage, so `--dry-run` must pass after any machine cleanup. This run validated both plans without repeating a gameplay run.

## Current receipts and recovery

- `collect_packaged_results.py` consolidates existing cooked input/fresh-load/Human authored proof receipts. It does not rerun them.
- `faction_admission.py` derives all 102 directed context records from existing canonical sources and approved role assignments. It does not enable six-faction gameplay.
- `stage_footprint.py` measures the actual local package and records redacted configuration findings. It does not prune or alter content.
- `plot_telemetry.py` produces an SVG from the one existing failed thermal attempt, without third-party plotting dependencies or another benchmark.
- `preserve_final.py` created the one additive local recovery archive. Do not rerun over it. Its 241 member hashes are verified in `final-local-recovery.json`; it includes the exact current Source tree with preserved inactive inherited drafts and therefore is not clean-source admission.

All current authored/candidate art remains local-only. These tools operate on the qualified worker and its recorded stage, not an arbitrary clean checkout. The source delta patch and previous milestone patch must not be mistaken for an automatically approved R10/Expansion merge.
