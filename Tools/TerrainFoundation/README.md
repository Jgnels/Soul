# Bounded terrain-foundation study

These are **study and qualification tools**, not a production-world launcher. They do not change canonical gameplay, save, construction or battle rules. The current candidate is **not accepted as Soul's production foundation**.

The 2026-10-07 run compared the verified native Mountain05 domain, retained 1.5 km Soul terrain, and available evidence for the owned FreshCan coastal sample. FreshCan source was unavailable. The physical scale recommendation is **3.5 km square as a working hypothesis**; 4 km did not resolve the source/layout mismatches. See `Evidence/TerrainFoundation-20261007/visual-review.html` and `source-scale-decision.md`.

## Read-only analysis

- `analyze_fit.py`: coarse 36-node/51-edge fitting and 3.5/4 km comparisons. This does not establish final grades or crossing geography.
- `fit_routes.py`: dense sampling using UE Chaos triangle interpolation, grade/bank constraints and bounded rounding. `--water-z 2.144607843 --output <new-name>.json` tests the original donor water level in candidate coordinates. The default lower-water r5 result must not be confused with original-water connectivity.
- `analyze_drainage.py`: unchanged-source water components, spill elevations, named-site checks and shoreline sensitivity. Its priority-flood array is **not exported as terrain**.
- `analyze_crossings.py`: short bank-to-bank opportunities. No bridge asset, traversability or gameplay edge is created.
- `analyze_source_framing.py`: twenty analytical 6.12 km source windows/orientations, all at 3.5 km physical scale and original water. No additional heightfield or Unreal map is exported.

Use the bundled scientific Python runtime with NumPy/Pillow; system Python lacks those dependencies. Generated large records and licensed-derived images are local evidence, intentionally outside the milestone commit.

## Qualification

- `test_foundation.py`: byte preservation, full-domain export sample identity, topology, native collision comparison, road grade receipt and screenshot presence. Passing these tests does **not** accept the art.
- `validate_saved_candidate.py`: run inside a fresh, guarded UE editor after loading `/Game/SoulCampaignFoundation/L_Mountain05_3500_r1`. Checks actual saved anchors, roads, collision, foliage and scale objects. It is not campaign gameplay or RBSave qualification.
- `build_review.py`: local HTML and contact sheets using actual screenshots; no generated player-facing content.
- `summarize_run.py`: compact preservation and capped-editor resource observations.
- `capture_native_readonly.py`: source-only transient camera study. Never save the donor. This run's two resulting captures are incomplete because texture builds reached the memory guard; see `native-weight-recovery-status.json`.

All Unreal use must retain one heavy process at a time and the 85 C cutoff. Use the existing `Tools/WorldTerrain/editor_session.py` guard. Do not infer a performance pass from a capped editor capture or process exit.

## Local candidate and rejected iterations

The one candidate uses every sample of the surviving **2041² full-domain native export**, rotated 90° and uniformly compressed in XYZ. It does not copy all 8161² native vertices. It has 64 Landscape components and a provisional automatic branch of the owned native material. Original authored paint weights were not recovered.

The candidate, original height export, local placement/import scripts, rejected road meshes and large receipts remain on the worker. They are not a complete one-command production build recipe. Do not blindly rerun a creation script over existing candidate packages; use an explicit reviewed revision. Earlier light/material and generic-anchor attempts are retained as rejected study history, not recommended defaults.

Production remains the retained `/Game/SoulCampaignMountain/L_evil_waterfront` reference. No command-line flag, project setting or gameplay adapter activates the candidate. No new source C++ was authored or built in this run. Inherited R10/expansion drafts remain uncompiled and untouched.
