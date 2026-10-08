# Soul production push — final handoff

## Decision

- **WORLD FOUNDATION: YES**, as the future campaign presentation foundation. Keep it opt-in; no automatic promotion.
- **DWARF PRESENTATION: PROVISIONAL**. The tested replacement was rejected.
- **RUNTIME QUALIFICATION: PASS**, within the explicit founder/Human qualification fixtures and editor-game execution described below.
- **PERFORMANCE QUALIFICATION: FAIL**: observation-window mismatch in the launcher. The valid 30-second sample met frame-time targets and stayed below 85 C; it is not a sustained-performance pass.

The macro geography is worth keeping and now works through Soul's existing campaign architecture. This is not final world art, a complete six-faction campaign, a packaged release, or a Bannerlord/Warhammer quality claim.

## Branch and recovery

Worktree: `D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929`.
Branch: `codex/soul-bannerlord-campaign-map-20260929`.
Starting HEAD: `c610966001d9aafd739f9d49550261e1f5d4f43f`.
Remote remains `4842eeb403f6b9be6effdf9fe18fc5c24b499887`.
Final local milestone SHA is recorded in `final-head.txt` after commit and appended below. No push/merge/reset/clean/stash.

The compiled runtime changes overlap inherited uncommitted R10/expansion source. They remain applied locally, with their exact baseline-relative changes preserved in `source-delta-working.patch` and hashes in `source-delta-working.json`. The scoped milestone commits tools, presentation data and evidence; it does not silently stage inherited source drafts, configuration or licensed payloads. The qualified state is this preserved worker, not a clean checkout of HEAD alone.

## Geography and Dwarf time cap

Candidate: `/Game/SoulCampaignComposition/L_Composition_3500_r2`, 3.5 x 3.5 km. 36 canonical physical anchors and 51 legal pairs unchanged. Frozen heightfield unchanged. Retained 1.5 km map remains the qualified reference/default.

Dwarf work closed at **26.4933927 minutes**, below 60. Inspected the actual owned DwarvenCitadel exterior; derived 153 native gate pieces into a 67,510-triangle trial. Both local presentations remained freestanding and were rejected. No retest of the closed constriction, no manufactured canyon, no terrain edits. Five trial actors are inactive, invisible and collision-disabled. Original representation restored unchanged at approximately `[100197.475953,-116064.598530,4051.680763]` cm, scale 0.5. It remains an exposed proof representation with poor pass logic. Do not call it finished. See `dwarf-result.json` and local rejected captures.

## Accepted visual changes

Two Human shared trunks reduce brief parallel split/rejoin strands: Crossroads–Old Quarry and Crossroads–River Ford now share appropriate portions of the existing Southern Crossing–Broken Bridge corridor. No legal edge removed. Distinct destinations remain separate. Three small grade-qualified fillets were retained on North Pass–Orc Camp and Dwarf High Quarry–Snow Basin. Broader rounding was rejected where it broke grades; no terrain deformation.

Broken Bridge received eight asymmetric fragments from the existing owned masonry derivative, outside the timber travel lane, collision disabled. Reviewed wide and centered close captures. Improvement is modest; clean balustrades, inherited off-lane collision and water edges remain provisional. Stone crossings, minor timber crossing, unbridged Ford and both ferry passages retain their functions. Human forecourt/capital scale and approaches were preserved.

No new population initiative, city framework, regional terrain/material overhaul or foliage experiment. Human roads improved locally; Crownspine still has angular bends; Viking coast/harbor travel works but authored settlement presentation remains sparse; Orc Mesa transition remains obvious; Nature forest/path art remains provisional; Dark horizon/causeway identity remains unfinished.

## Route acceptance

Final closeout: **51/51 PASS**, 52,965 native stations, zero misses. Maximum 22.07781099 degrees (Nature Grassland Edge–River Woodland). Crossroads–River Ford 21.76053862 degrees. Maximum sampled heightfield/collision delta 0.00257593 m. Native output is byte-identical before/after the collision-free bridge fragments.

See `native-route-closeout-summary.json`, `route-inventory.json`, `route-atlas.html`. This is sampled-grade/collision acceptance, not proof of continuous curvature quality. Sub-20 cm numerical intervals are retained in raw diagnostics and excluded from grade acceptance; independent analytical 0.25 m sampling also passes all 51.

## Existing runtime adapter

Explicit `-SoulComposition` loads the saved candidate through existing Terrain/WorldActor/State/Region/Camera systems. It reads 36 anchors, 51 dense route paths and a lossless R16 serialization of the frozen owned heightfield. Baked roads/water/scenery are reused. Tagged review anchors/static Human miniature are removed only from the transient runtime instance; the existing stateful Human miniature uses its measured 0.24 placement.

All six rendered runs verify no SoulWorldTerrain/SoulCampaignExpansion flags. Canonical graph, action costs, battle/save/construction authority and schemas are unchanged. The composition camera uses actual terrain bounds and a 2.2 km maximum viewing distance; the prior unsupported overview edge view was rejected.

**Scope boundary:** the current founder save validator accepts the player faction, enemy faction and neutral owners. Qualification retains established founder/Human-proof ownership and leaves additional regions neutral. Six-faction strategic play is NOT qualified. Existing 9-region saves are not silently migrated into 36-region fixtures. All runs use isolated UserDir slots. Existing camera load behavior recenters on the saved company; exact prior pan/zoom is not a saved field.

## Runtime results

- `runtime-input-r3`: PASS actual controller input paths: keyboard/mouse, drag, zoom/orbit, simulated gamepad controls, selection, recruitment, legal movement, rejection of illegal/no-AP moves, exact F5/F9 restoration. Rendered maximum camera view reviewed.
- `runtime-load-r1`: PASS separate-process F9; exact snapshot restored at River Ford with 46 troops and correct selection/camera focus.
- `runtime-traversal-r3`: PASS 40 continuous legal moves across 28 regions, six ferry legs, normal clicks/end-day actions, no teleports or ownership/AP overrides. Includes bridge/ford paths, both physical ferry passages and northern/Crownspine/Nature/Dark travel. Ferry walking figures hide over saved water segments and reappear on land. Two physical ferry passages are shared by seven legal pairs. No boat animation; arrival screenshots do not always frame the intermediate crossing.
- `runtime-human-r1`: PASS starting/constructed native tavern, 200 gold/two-day timing, companion-service unlock and hire, exact two-domain F5/F9, matching miniature, two actual city visits, return, real authored battle with reinforcements, natural victory 30/0, correct Crossroads return, post-battle F5/F9. City/combat/return images reviewed. Intact uncooked city loads were slow (~10 minutes per visit); full proof 43.3 minutes. Peak 81 C, ~7.77 GiB VRAM and ~17 GiB private commit; capped 20 FPS, not a performance result.
- `runtime-recovery-r1`: PASS two natural defeats 0/30, correct River Ford return, actual travel to capital, three recruits deducted from finite pool/resources, new encounter, second result and RBSave restoration. Existing Dragon Graveyard battle recipe retained for this recovery proof; no combat changes.

See `runtime-results.json`, `human-runtime-receipt.json`, `traversal-receipt.json` and each `Local/runtime-*/runtime/summary.json`. Engine exit alone was not acceptance.

## Builds and tests

Fresh SoulEditor r3 PASS 196.58 s; final r4 PASS 36.95 s after a test-observer input-frame timing correction. Fresh Soul game r1 PASS 237.56 s; final r2 PASS 93.32 s. `build-results.json` links exact arguments/times; binary hashes in `new-assets-and-binaries.json`.

Native CampaignWorld: 2/2 default and 2/2 composition PASS. Source contracts 18/18 PASS. Composition data/route/preservation tests 5/5 PASS. Input source tests 3/3 PASS. Broader Tools 97/99 PASS; packaging assertions remain unresolved (inherited Expansion/r9 drafts and deliberately unpromoted composition cook scope). Default cooking/packaged runtime is NOT qualified. No inherited binary is presented as a fresh build.

## Single performance measurement

Native 1920x1080, 100% render scale, D3D11, VSync off, uncapped. 20-second warmup, 30.00623 measured seconds,2,477 frames. Mean 12.1139 ms (**82.55 FPS**), P95 12.8529 ms, P99 13.3113 ms. Frames below 30/40/60 FPS: **0/0/0**. Peak GPU **84 C**, VRAM 6,197 MiB, process working set 4,270.68 MiB, private commit 5,647.39 MiB.

The measurement and screenshot completed normally, with no crash or 85 C cutoff. However, the wrapper required 60 seconds observed alive after ready; it observed 56.62 before the benchmark's normal exit. Therefore the overall receipt remains **FAIL**. No repeat run. Future launcher sample length was corrected to 60 seconds while retaining the existing observation minimum and 85 C guard; that revised duration has not been rerun. No sustained or worst-case fully populated-world performance claim. `performance-receipt.json` retains the exact distinction.

## Preservation

Final 371-file manifest: 359 unchanged; only the candidate map and 11 explicitly attributed existing source files differ. No unexpected donor/reference changes and no unexpected inherited tracked changes. `preservation-final.json` records before/after hashes; `inherited-tracked-hashes.json` and ignored `Local/Inherited/` preserve the dirty baseline. New traversal source is separately recorded in the patch.

Candidate before: `c468893303763b1dae1b536884079ceaf40ff2e9e45675869e38e7ed23b4feb4`.
Candidate after: `b89f4557e58095c2253467c6990d9af88e16e5b849213215b56567ed2f9c4f0c`.
Frozen 3.5 km height PNG: `bd84f1986c9fd71b51725647d8ae89644a0681c5d65f2d2aebcf6ab31383ba6b`.
Retained map: `d18c21c2aeadf50036ef3a605737bac971e2e91e2a43aaa988d92f5987a8094f` (use manifest as hash authority).
Retained height: `908f9de444112694afe6a242db86a45c64adef8bcaa392c464e304a1f1c3990b`.
Retained presentation: `6d93437e19aaf9efb3683aabace2431ca3b05234dcef935b3c10e0d362e95eaf`.

No donor saves, licensed payload/config staging, generated player-facing art, source resets or cleanup. Candidate/donor assets, large captures/logs, inherited source/configuration and inactive rejected work intentionally remain local.

## Evidence and next action

`visual-review.html`: paired prior/current map captures, 18 final/detail views, selected runtime input/load/travel, native Human construction/battle/return and defeat return. Full captures and logs remain in `Local/`. Rejected iterations are retained and described in `rejected-iterations.md`.

**Proven:** existing campaign integration, measured terrain/route alignment, input, saves, construction/visits, victory and defeat/retry.
**Provisional art:** Dwarf exterior, road surfaces and sharp turns, bridge styling, water/shore masks, regional materials/foliage and sparse settlement silhouettes.
**Known defects:** inherited city material/load warnings, expensive uncooked Human loads, unresolved default cook assertions, performance launcher observation mismatch in the recorded run.
**Not implemented:** full six-faction campaign ownership/balance, 9-to-36 save migration, packaged candidate promotion, animated ferries, final population/art for all regions.

Next recommended action: review this milestone, then scope a clean source/promotion-and-cook pass that separates the composition adapter from inherited inactive drafts and explicitly decides save compatibility and six-faction campaign configuration. Keep the frozen geography; do not restart terrain or another deep city proof. Dwarf art can remain a separate backlog item.
