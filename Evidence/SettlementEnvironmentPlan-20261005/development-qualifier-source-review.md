# Development controls and visit source review

Read-only review of `SoulSettlementDevelopmentQualification.cpp` and `SoulSettlementVisitGameMode.h/.cpp`, traced through the real controller, HUD, campaign actor and save callbacks. No UE process, build, source or donor mutation was performed.

## Findings sent to the implementation owner

1. **P2 — dead HUD click dispatch can pass the duplicate-action assertions.** At qualifier lines 187–191 and 231–234, the successful build/hire uses keyboard input first. The subsequent rendered HUD click is judged solely by unchanged canonical snapshots/revision. If the click never reaches `NotifyHitBoxClick`, the assertions still pass. A rendered hitbox establishes that the button exists, not that its action ran. Require an observed action response transition to the existing duplicate rejection message, or exercise a successful HUD action with a canonical state change. Do not synthesize the response in the qualifier.

2. **P2 — the completed-state image can race F9.** At qualifier line 204, `Capture("completed")` and `StartLoad()` run in the same tick. Screenshot requests are deferred; a fast restore can change the state/panel before the image is rendered. File existence at the final step cannot detect this wrong-state image. Hold completed state until the screenshot request has processed and the nonempty fresh file exists, then dispatch F9 in a separate step. The installed `UnrealClient.h` exposes `FScreenshotRequest::IsScreenshotRequested` and `OnScreenshotRequestProcessed`.

Both are evidence defects; the two-domain persistence logic itself is substantially stronger than checking an exit code or stale success flag. The findings above describe qualifier SHA256 `8fbed3f9b36c8b3f01ee5c4e77849ed1313f7dc23723c6ae6ee8871c15cd1d51`. Later owner changes require focused re-review.

## Pending revision 2 review

The owner prepared `Local/qualifier-revision2/SoulSettlementDevelopmentQualification.cpp` outside live Source during the parent's build. Reviewed candidate SHA256 `d14176be3ef1e30d05cbcf04fa1af41c140bcf0da31016abf4f43bf10e712e47` fixes both findings: it requires actual success-to-rejection message transitions around the duplicate clicks, and waits for the fresh completed PNG on a later tick before F9.

It also corrects an original sequencing failure confirmed against `SoulFounderPlaytestCampaignActor.cpp:195`: `EndDay` closes the town. The original case 11 pressed T after that closure, reopening the town, then case 12 expected it closed. Revision 2 accounts for EndDay and reopens the mid/completed panels before their captures. Its case 15 label still incorrectly attributes closure to T although EndDay did it; the owner was asked to correct that assertion label. No additional substantive blocker was found in the candidate. **Candidate fixes are source-reviewed, not yet established as applied, built or runtime-passed.**

## Checks that withstand source review

- Each F5 compares actual checkpoint bytes before/after, waits for persistence completion and requires success; an ignored F5 cannot pass because its bytes remain equal.
- Each F9 records the next campaign-load and settlement-development revisions, requires both exact increments and ready state, and compares complete campaign plus settlement snapshots. The live state is deliberately changed before each restore. The first restore recovers partial construction; the second recovers completion, hired hero, gold and day together.
- The controls qualifier requires a fresh isolated absolute UserDir, absent slot/prefix artifacts, explicit retained-terrain proof flags and an actual 1920×1080 viewport. Its result explicitly declares authored environment, miniature and battle environment unqualified. Its 20 FPS cap is functional-only evidence.
- `HUD.h` includes `HUDHitBox.h`, so the qualifier's use of `FHUDHitBox::GetName` is not an incomplete-type issue. No definite header compile blocker was identified in this bounded review.

## Visit mode boundary

No additional definite authority or input-sequencing defect was found. Visit authorization uses the actual GameInstance settlement subsystem, the current owned capital and the bound owned map. BeginPlay verifies the loaded map. Actions recheck authority and persistence locks, revision changes refresh presentation, and Return remains available when the visit fails.

`SOUL_SETTLEMENT_VISIT_READY` currently establishes a matching map and activated camera. `RefreshPresentation` accepts zero matching presentation controllers/groups, so this marker cannot establish building-state parity or intact authored-environment composition. The eventual environment qualification must inspect the actual reviewed actor-group membership and visible/collision state. This is a scope limit, not a demand for a new subsystem.

Reviewed visit source SHA256: cpp `e53ebe4d69337cd920876397f512412247a6007fa5ba164959b2bdf1b83450be`; header `76758d8329390a844228a280901a6f6ed153c242d4d0b674751451af047d3ee5`.
