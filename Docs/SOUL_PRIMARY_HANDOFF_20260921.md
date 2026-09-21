# Soul Primary Recovery Handoff — 2026-09-21

## Authority / machine
- Project: `D:\RefinedBadger\Games\Soul`
- Machine: `DESKTOP-Q1S3RPU` (upstairs)
- Branch: `astra/soul-primary-continuation-20260920`
- HEAD: `8463ecb` — validation: record hero recruitment acceptance
- Do not use downstairs or kill unrelated UE processes.

## Verified completed work
- Commit `98e6456`: battlefield grid persistence/verification.
- Seven battlefield maps reload from disk with one Soul layout, one Soul grid, and 217/217 baked cells.
- Commit `3f26526`: physical-venue hero recruitment gating.
- Hero recruitment validation: SoulEditor production compile succeeded; automation 30 passed / 0 failed.
- Hero tests include CandidatesCanDepart and PhysicalVenueGatesHiring.
- Siege damage proof verification currently passes.
## Current failure diagnosis
- `Evidence/visual_acceptance_batch_manifest.json` currently fails at `verify_city_overlays.py`.
- Both overlay maps reload nearly empty (~8.5 KB) with zero Soul presentation/bootstrap/town/layout/building/wall/objective actors.
- Root cause identified: `build_city_overlays.py` added the donor streaming level before spawning Soul actors.
- Adding the donor changed the editor current level; Soul actors were therefore spawned/saved into the donor maps.
- Binary inspection confirms Soul_* actor names inside:
  - `Content/Medieval_Megapack/Levels/PL_Fortress_Day.umap`
  - `Content/JustBStudios/Water_City/Levels/LV_WaterVillage.umap`
- The overlay maps themselves contain only their map names and no expected Soul actors.

## Recovery safeguards already completed
- Contaminated donor and overlay map files copied to:
  `D:\Recovery\SoulDonorContamination-20260921`
- Added `Tools/repair_contaminated_donor_maps.py` to remove only Soul-owned actors from the two donor maps, save, reload, and verify zero remain.
- Patched `Tools/build_city_overlays.py` so Soul actors are spawned before the donor is streamed, then the overlay world is saved explicitly with `save_map`.
- Python syntax checks pass for repair/build/verify scripts.
## Current blocker
- Upstairs UE is occupied by unrelated `RBMagicFabBridge`.
- Observed process: UnrealEditor.exe PID 14060, launched with `RBMagicFabBridge.uproject -d3d11`.
- A Soul repair launch was attempted, found the shared Build.bat lock, and was stopped.
- Soul shell and UnrealEditor-Cmd child were explicitly terminated; RBMagic was left untouched.
- No Soul UE process should be running now.

## Exact next actions when upstairs UE is free
1. Run `Tools/repair_contaminated_donor_maps.py` in Soul UE.
2. Verify repaired donor binaries no longer contain Soul_* labels/classes.
3. Rebuild Human/Viking city overlays with patched `build_city_overlays.py`.
4. Reload both overlays from disk and run `verify_city_overlays.py`.
5. Rerun `run_visual_acceptance_batch.py`; require siege + city overlay verification pass.
6. Run optional Hivemind navigation/collision audit if the primary batch passes.
7. Inspect Git status/diff and only then commit verified lane-owned changes; do not merge.

## Important unknown
- Do not restore donor maps from a similarly named Vault package by guesswork. One cached 'Viking' manifest inspected was a character pack, not the Water City donor.
- Current repair strategy is actor-surgical and reversible from the backup above.

## Additional hardening completed while UE was occupied
- `Tools/build_battlefield_overlays.py` had the same persistence hazard as the city overlay builder: it saved the current level after adding a streamed donor.
- It now regenerates Soul-owned battlefield overlays deterministically, saves the explicit overlay world with `save_map(world, dst)`, and verifies donor presence through both loaded and streaming level package paths.
- `Tools/verify_city_overlays.py` now checks direct streaming-level package paths as well as loaded level paths.
- Python compile validation passes for donor repair, city overlay build/verify, battlefield overlay proof, unified visual acceptance, and Hivemind nav audit.

## Current live-lane status update
- RB Magic still owns upstairs UE.
- Current RB Magic build is actively producing `RBMagicBuildProbe2` (AutomationTool/UBT), so Soul must continue to wait rather than contend for UE.

## Generated content / Git footprint
- The seven old copied battlefield prototype maps are untracked and extremely large: approximately 469 MB to 797 MB each.
- `LV_Soul_HumanCapital_Prototype.umap` is approximately 232 MB; the Viking prototype is approximately 14 MB.
- These copied-donor prototypes must not be blindly staged. The lightweight streamed overlays are the intended replacement architecture.
- Keep the large prototypes locally until overlay acceptance completes; do not delete them as part of recovery.
- The vendor donor path `Content/Medieval_Megapack/` is already locally excluded from Git. Continue to stage only explicit Soul-owned paths.
