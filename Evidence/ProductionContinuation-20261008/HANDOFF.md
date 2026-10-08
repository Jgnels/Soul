# Soul production continuation — qualified local milestone

Continuation start: **2026-10-08 14:10:11 UTC**. Approximately five-hour remaining budget. Exact completion timestamp, elapsed duration and final Git identity are recorded in `final-closeout.json` after the scoped closeout commit. No Dwarf or terrain research was repeated.

Worktree: `D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929`
Branch: `codex/soul-bannerlord-campaign-map-20260929`
Starting HEAD: `8ea786c180f2f8e431831b92b7d8867e2237c818`
Implementation/evidence milestone: `c0e2e63b6f4e277b26dd19bb83e319918284033d` — `Polish composition routes and qualify isolated cooked campaign candidate`
Ending HEAD: recorded in `final-head.txt` after the final closeout commit.
Remote unchanged: `4842eeb403f6b9be6effdf9fe18fc5c24b499887`. **No push, merge, reset, clean, stash, destructive deletion or donor save.**

## Scope and decisions

Candidate remains `/Game/SoulCampaignComposition/L_Composition_3500_r2`, **3.5 × 3.5 km**, opt-in through the existing campaign adapter. The retained `/Game/SoulCampaignMountain/L_evil_waterfront` remains the qualified reference; no default-map promotion occurred. All 36 canonical IDs/physical anchors, 51 endpoint pairs, frozen macro heightfield and established gameplay authority remain unchanged.

**Dwarf research/miniature iteration this continuation: 0 minutes.** Crownspine road curves are road work, not renewed Dwarf research. No terrain research, new city proof, construction framework, combat system or generative player-facing assets were introduced.

- **WORLD FOUNDATION: YES**, as the future working geographic/runtime foundation. This is not final visual quality or authorization to replace the retained map.
- **DWARF PRESENTATION: PROVISIONAL**, unchanged.
- **RUNTIME QUALIFICATION: PASS** for the new cooked input/save/Human-authored boundary and representative transport regression below. This is not full six-faction game qualification.
- **PERFORMANCE QUALIFICATION: FAIL**, single corrected 60-second uncapped attempt stopped at 85 C before completion.
- **SIX-FACTION GAMEPLAY: NOT IMPLEMENTED / NOT ACTIVATED.** Configuration groundwork is separate from the qualified two-faction fixture.

## Visual changes and limits

Fourteen bounded road curves were retained: eight general corrections and six Crownspine corrections. Crownspine changes cover Shrine/Pass, High Quarry/Snow Basin, Forge/Hold, Snow Basin/Hold and Pass/North Pass approaches. Existing shared trunks and canonical edges are preserved. No terrain flattening was used.

The road mask and runtime path data consume the same accepted routes. `presentation.json` contains 112,513 sampled XYZ points. Both existing ferry passages remain distinct; seven legal pairs use them. Major stone bridges, minor timber bridge, unbridged River Ford and repaired Broken Bridge retain their prior functions. No bridge/crossing art change is falsely claimed for this continuation.

| Region | Retained improvement | Honest limit |
|---|---|---|
| Human | Owned broadleaf/pine variation in existing woodlots, local road curves | Farmland/context sparse; regular road junctions and ground tiling remain. Capital size/arrival preserved |
| Crownspine | Six local grade-qualified curves | Several switchbacks remain angular. Hold exterior unchanged |
| Viking | Cooler owned pine variants; about 26.4 m wood jetty with bed-seated supports and qualified local shore spur | Harbor context only; assigned Viking city/town not integrated |
| Orc | Two owned camp outworks, local curves, sparse rocks, roughly 10.3 m feather of the existing Mesa material transition | Large donor transition still visible; no new city claim |
| Nature | Mixed owned pine/broadleaf meshes, two deterministic reduced tree derivatives, sparse bank rocks | Exact 18,524 forest XY placements preserved, so scatter repetition and hard bank masks remain |
| Dark | Bounded road curves | Full gate trial rejected for lack of a suitable natural bench. Two thin-wall trials rejected after rendering and hidden in game. Fortress horizon/causeway identity unfinished |

37 small collision-disabled native cue actors were retained. No new strategic location, population system or gameplay collision was added. The lowland grid-like texture repetition is visible in both prior and current captures; it is inherited art debt, not claimed as repaired. No new checkerboard fallback was identified in the reviewed campaign captures. Three existing CastleTown algae material instances report fallback during cook and remain known defects. Cooked Human visits emit 34 error lines (unresolved modeling function and invalid/uncooked shader maps) plus warnings; campaign-only runs emit four missing-WebBrowser-material warnings. `runtime-diagnostics.md` separates these from the zero-error cook summary. Functional PASS is not warning-free distribution acceptance.

`regional-polish-review.md` and `visual-review.html` distinguish accepted changes from rejected Dark trials. The latter remain `Continuation_Dark_Remnant_*`, persistently hidden in game and collision-disabled; editor-session visibility must still be hidden when reopening for review.

## Route acceptance

**51/51 analytical and 51/51 native PASS**. Native final: **52,944 stations, zero misses**, maximum **22.07781099°**, River Ford **21.76053862°**. The maximum native/sample height discrepancy is approximately **0.00258 m**. Raw collision evidence and per-route results: `native-route-final-summary.json` and `Local/native-route-hits-continuation2.json`.

The older `route-atlas.html` / `route-inventory.json` are explicitly labeled the initial analytical audit, not the final route receipt. Surface quality remains distinct from geometric acceptance. `final-route-review.html` is the final interactive analytical index: current versus inherited geometry, crossing hierarchy, native grades and actual cooked travel coverage. It independently matches all 52,944 recorded native XY stations to the current polylines exactly. Fourteen curve locations affect 15 routes through shared trunks. The native summary `source_sha256` identifies raw collision hits; the runtime presentation separately identifies the final route JSON, including the local Viking spur.

## Save-compatibility decision

Use existing RBSave domains and unchanged schemas, with isolated slots:

- Retained/default: `Soul.VerticalCampaign`.
- Composition fixture: `Soul.Composition3500.Founder`.
- Human authored proof: `Soul.Composition3500.HumanProof`.

No silent 9-to-36 migration. Existing region-count/ID/state validation rejects the wrong shape without changing live state. Old qualification checkpoints remain in their original UserDirs. No player checkpoint was automatically copied or converted.

Fresh default/composition native tests, normal F5/F9, independent fresh-process restoration and existing construction controls pass. The first actual cooked boundary separately passes the same input/save flow and Human authored-state/battle proof. `save-compatibility.md`, `save-regression-results.json`, `packaged-runtime-results.json`.

## Cooked runtime and builds

Fresh builds: **SoulEditor PASS, Soul PASS, SoulComposition PASS**. The initial editor build failed on a test-only UE5.8 key conversion, fixed before the passing build. A target-wide HDRI plugin-enable attempt was rejected by the installed shared build environment; the retained fix is the exact candidate-only NonUFS plugin descriptor dependency. Metadata rebuilt successfully and the gameplay executable hash stayed unchanged.

Native CampaignWorld: **2/2 default + 2/2 composition PASS**. Source/tool contracts: **13 + 18 + 1 PASS**. Later read-only/manual-tool preflight verifies 27 Python helpers parse and both manual profiles validate staged hashes without starting a game. Do not interpret these receipts as a clean checkout build of HEAD alone.

Full-runtime cook: **4,518 packages, zero errors, nine unique warnings**. Initial D: attempt stopped at storage guard and is preserved. Successful full cook is local-only at `C:/Users/Jeff/AppData/Local/Temp/SoulCompositionCookPayload_dikr_ozr/Cooked`. Exact receipts in `cook-results.json`.

Corrected loose stage: `C:\Users\Jeff\AppData\Local\Temp\SoulCompositionStage_rffb6atp\Stage\Windows`. **12,658 files, 17.56 GiB apparent size**; 10,754 cooked files share hardlinks with this run's cook/first stage. These totals are not independent disk allocations. No distributable archive was produced.

First actual staged launch failed on the missing HDRIBackdrop descriptor; the corrected target/stage then passed:

| Cooked run | Result | Observed scope | Peak GPU |
|---|---|---|---:|
| `packaged-input-r2` | PASS | Normal keyboard/mouse/controller selection, camera, recruitment, movement, exact F5/F9 | 63 C |
| `packaged-load-r1` | PASS | Separate process/UserDir restores copied candidate checkpoint exactly | 65 C |
| `packaged-human-r1` | PASS | Native tavern/miniature start and upgrade, construction/service, visit/return, real authored battle, reinforcements, natural victory and exact post-battle two-domain F5/F9 | 84 C |
| `packaged-traversal-r1` | PASS | 40 legal moves, 31 distinct edges, 28 regions, six ferry legs; no teleports or ownership overrides | 71 C |

The cooked transport journey covered 31 of 51 unique legal edges and 28 of 36 regions, across all six macro-regions. Six ferry legs hid the walking party and restored it after passage; the screenshot camera follows the destination rather than depicting a boat. This remains provisional ferry presentation. Unvisited edges are not runtime failures, but exhaustive all-edge runtime travel is not claimed. Ford, bridge and mountain arrival captures were reviewed alongside the Human, Viking, Nature and Dark destinations. Nature Treehold, Dark Fortress and several other sites remain labeled locations without completed settlement art.

Human battle returned at Crossroads with **33 allied survivors / 0 hostile**, shared settlement state intact. Starting/completed city and miniature, actual battle and return screenshots were directly reviewed. Twelve Human captures are retained. No third city proof or Dwarf requalification was performed. Prior defeat/retry evidence remains inherited and was not rerun because those systems were unchanged.

Actual cooked log: `regions=36 routes=51 world_experiment=0 expansion_experiment=0`, exact candidate map, **81/81 startup collision hits** and **0.2065 cm** maximum startup discrepancy.

## Performance — truthful failed result

One corrected attempt only: native **1920×1080**, **100% scale**, D3D11 diagnostic path, uncapped, requested **20-second warmup + 60-second sample**. The unchanged guard stopped it at **85 C** before completion. No complete mean/FPS/P95/P99 or below-30/40/60 counts exist; all remain null. Peak device-wide VRAM **6,696 MiB**, process working set **4,214.61 MiB**, private commit **5,779.55 MiB**. VRAM includes other applications.

No second uncapped attempt. The later 20-FPS capped functional cook tests are not performance passes. `performance-result.json`, `performance-thermal.svg`, `Local/runtime-profile-60s-r1/runtime/`.

## Six-faction configuration groundwork

Derived current canonical sandbox: six factions, 36 IDs, 51 edges, 12 initially owned regions, 24 neutral; all declared immediate frontiers agree with adjacency. Nothing was activated or rebalanced.

Existing battle identity gate still permits only Humans/`human_knight` versus Dwarves/`dwarf_warrior`. Generic six-faction ownership alone would otherwise conceal side-based roster substitution. Existing SoulCore faction/world/strategy rules should be extended; no replacement authority is needed.

The admission worksheet covers **102 directed connections**, all with recipe metadata, but only **20 existing explicit founder approach records**. The other 82 remain unset rather than fabricated; the current runtime can still select its admitted fallback. Human/Dwarf authored wrappers are cooked. Other approved seats are candidates, not six completed city/battle integrations. Starting army balance and diplomacy remain pending. See `six-faction-groundwork.md`, `six-faction-readiness.json`, `six-faction-admission.md` / `.json`.

## Preservation and commit boundaries

Protected baseline: **375 files**, **369 unchanged**, six intentional changes (candidate map plus five source/test files); **zero unexpected changes**. All 36 inherited dirty tracked files separately checked with zero unexpected changes. The final check completed after the last cooked journey: all 241 recovery files still match their verified archive (`recovery-final-verification.json`). Source configuration, rejected R10 drafts, donor projects, retained reference, frozen heightfield and authored cities remain preserved.

- Candidate before SHA256: `b89f4557e58095c2253467c6990d9af88e16e5b849213215b56567ed2f9c4f0c`.
- Candidate after SHA256: `a80d7df96d0cf3ca3268a09ad1806806a75b022a023c2e4c5082b9e65668a39a`.
- Frozen 3.5 km height PNG: `bd84f1986c9fd71b51725647d8ae89644a0681c5d65f2d2aebcf6ab31383ba6b`.
- Retained map: `d18c21c2aeadf50036ef3a605737bac971e2e91e2a43aaa988d92f5987a8094f`.
- Retained height: `908f9de444112694afe6a242db86a45c64adef8bcaa392c464e304a1f1c3990b`.
- Retained presentation: `6d93437e19aaf9efb3683aabace2431ca3b05234dcef935b3c10e0d362e95eaf`.

Full donor/reference hashes: `preservation-final.json`; inherited dirty hashes: `inherited-preservation-final.json`. Additive local recovery archive: `Local/final-candidate-recovery.zip`, **241 verified members**, SHA256 `d71aab5845b52ca35c976ec667c0f1f8cab8ca6930b57115ec9b9f8381002fb1`. It contains the exact current Source tree, including preserved inactive drafts, plus local candidate assets/data. It is not a source promotion or licensed redistribution.

Scoped commits contain owned data, tools, compact evidence, and the explicit `SoulComposition` target with its isolated packaging branch. The staged `Soul.Build.cs` was constructed from tracked retained rules plus the exact already-built candidate branch; excluded inherited R10 additions remain only in the unchanged working file. `packaging-source-admission.json` proves the candidate dependency set is identical to the passing build, while the tracked default dependency set stays unchanged. No new build is claimed for this index-only consolidation. Licensed assets, binaries, screenshots/cooks and machine configuration stay local-only. The 13-file continuation source/tool patch exactly replays against the inherited worker (`source-delta-replay.json`). The exact keep/exclude boundary is recorded in `source-promotion-boundary.md`. Qualified composition source still overlaps prior inactive drafts; **HEAD alone is not claimed to reproduce this binary**. No unrelated draft was silently admitted to make the status clean.

## Classification and next action

**PROVEN / QUALIFIED:** frozen geometry/topology, analytical/native grades, new cooked stage, current input/save/state/visit/Human battle/return, profile isolation, donor/reference preservation.

**PROVISIONAL ART:** road surfaces/junctions/switchbacks, inherited ground tiling, shores/water masks, Dwarf exterior, sparse regional population, Viking landing and Orc/Nature context.

**KNOWN DEFECTS:** thermally failed uncapped performance; three cooked Human algae fallbacks/invalid shader maps, unresolved modeling function and missing WebBrowser material dependencies; unfinished Dark fortress/causeway identity; visible Mesa transition; rejected hidden Dark fragments; qualified source not yet consolidated independently of inherited drafts.

**NOT YET IMPLEMENTED:** six active gameplay factions/rosters/strategic AI, 82 detailed directed approaches, broader geographically matched battle environments, the other four authored capital integrations, distribution archive and default-map promotion.

Next meaningful production step: finish reconciling the qualified composition gameplay adapter into a clean, reviewed source change that excludes inherited R10/Expansion drafts, then admit one additional exact faction/roster matchup through the existing bridge before enabling six-faction AI. Keep full-world art and thermal/performance work as explicit gates; do not reopen Dwarf research or terrain foundations to avoid those integration decisions.

Manual review: `Tools/ProductionContinuation/play_candidate.py --dry-run`, then normal invocation or `--human-proof`. It uses the existing 85 C guard, a documented 20-FPS review cap, isolated persistent saves and no automated gameplay. The local temporary stage must still exist.

Primary visual evidence: `visual-review.html` (paired current captures and actual cooked states), `Local/captures-final-r2/`, `Local/packaged-input-r2/`, `Local/packaged-load-r1/`, `Local/packaged-human-r1/`, `Local/packaged-traversal-r1/`. Local gallery links and inline JavaScript syntax pass (`final-review-integrity.json`). Browser automation failed to initialize twice; the review page was queued in Codex, so interactive browser validation is not claimed. Actual Unreal pixels were reviewed directly. This is not a claim of Bannerlord/Warhammer-tier final art.

## Exact changed / inherited state

`milestone-file-boundary.json` records the committed paths, this continuation's 13 source/tool deltas, and the exact intentional local asset/source boundary. `Local/closeout-status.txt` preserves full status without forcing it clean. `inherited-preservation-final.json` identifies inherited tracked state and its hashes; machine configuration and rejected experiments remain outside the milestone commits. Only the two explicitly admitted packaging files cross the Source commit boundary; mixed gameplay source remains local. `final-head.txt` and `final-closeout.json` are post-commit receipts, intentionally local-only to avoid a self-referential commit loop.
