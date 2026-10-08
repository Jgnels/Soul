# Qualified source admission boundary

This is a narrow source-consolidation handoff, not another terrain audit or an implemented refactor. Current worker builds and actual cooked runtime pass. HEAD-only reproduction is not claimed because the previous composition integration and this continuation overlap preserved inactive drafts.

## Retain from the current qualified worker

| Existing file / area | Composition behavior to preserve |
|---|---|
| `Source/Soul/Private/SoulCampaignTerrain.cpp`, `FBake` composition branch around line 96 | Explicit `-SoulComposition`; reject simultaneous World/Expansion flags; exact 36-region/51-pair admission; candidate map, R16, route arc/ferry ranges and miniature transforms |
| Same file, adapter methods around lines 1149–1212 | Current units/scale, native height lookup, arc-length road travel, miniature placement, ferry-range visibility semantics. Do not rescale geometry while consolidating code |
| Same file, `Build` composition branch around line 1295 | Stream the existing candidate, suppress editor/reference actors, verify actual Landscape collision, retain baked roads/foliage instead of duplicating runtime dressing |
| `SoulCampaignWorldActor.cpp` | Skip generated road meshes for the baked composition; present the party on the qualified route and hide its walking representation during ferry ranges |
| `SoulCampaignCamera.cpp` / header | Qualified composition focus/frustum/zoom behavior, preserving the retained profile. The unrelated 100 km World sea-apron branch is not needed |
| `SoulFounderPlaytestStateSubsystem.cpp` / header | Existing 36-region fixture selection plus this continuation's isolated RBSave slots. Keep current two-faction admission and both existing domains |
| Existing GameMode, CampaignActor, HUD, RegionActor, controller and qualification files | Only already-qualified composition presentation/input/state hooks; keep the existing campaign/battle authority |
| `SoulCompositionTraversalQualification.cpp` | Current opt-in observer only: actual clicks, normal AP/day progression, no teleports or owner overrides |
| `SoulComposition.Target.cs` and candidate branch of `Soul.Build.cs` | Same gameplay modules, exact four candidate data files, exact HDRIBackdrop descriptor. No default-map promotion |

## Keep out of the admission

- `SoulCampaignExpansion.cpp` / `.h`, tile loading and Expansion scenario/config references.
- `SoulWorldTerrain` bake/settlement-surface/material/dressing branches and old World capture path.
- `SoulCampaignWorldCapture.cpp` unless separately reduced to a truly needed retained/composition function. It is currently an opt-in old review tool, not production gameplay authority.
- R10 `MapsToCook`, World height/settlement payloads and all machine-local `.uproject`/INI/mount/security differences.
- Unrelated `Tools/WorldTerrain` drafts and old terrain-generation work.

The live mixed file contains a direct `SoulCampaignExpansion::Height` call and expansion-aware bounds even though composition initialization rejects the expansion flags. That inactive coupling must be removed in a reviewed admission, not concealed by committing the whole file. The current package proves both experiment flags are zero and stages neither rejected payload family.

## Smallest safe next sequence

Preserve this worker/recovery archive. Reconstruct the already-qualified composition branches against the tracked retained implementation, retaining the same existing presentation API and data. Do not design a new subsystem or re-author terrain. Admit the source in a separate reviewable commit; then run the changed-boundary editor/game build, native default/composition tests and one focused input/save/transport check. Reuse the authored-city proof unless source consolidation touches visit/battle/state behavior. Default promotion remains a separate decision, with the thermal gate still failed.

`source-delta-working.patch` is only this continuation's delta against its inherited baseline; `source-delta-replay.json` proves all 13 file patches replay exactly. The previous production push owns its earlier delta. `final-local-recovery.json` preserves the exact complete current Source tree locally, including the drafts excluded above. None of these receipts authorizes a blanket source/config add.

## Packaging-only admission completed in closeout

`SoulComposition.Target.cs` and the exact candidate dependency branch of `Soul.Build.cs` are now admitted separately. The staged rule starts from the tracked retained file, so the three inherited R10 dependency entries and their skip guard are not promoted. The live working rule remains byte-identical. Candidate dependencies are exactly equal to the fresh passing target; default dependencies are unchanged from tracked HEAD. This does not admit the gameplay adapter or claim a fresh clean-checkout build. `packaging-source-admission.json` records the indexed blob and equivalence check.
