# Canonical settlement development source review

**Disposition: no outstanding source-review blocker in the reviewed revision.** One P2 saved-binding defect was identified and repaired during review. This is a read-only source assessment; it does not establish a successful build, native automation run, real F5/F9 round trip, or authored-environment acceptance. Exact reviewed file hashes are in `canonical-development-source-review.json`.

## Resolved finding

**P2 — successful campaign restore could retain an incompatible settlement while reporting development ready.** The initial implementation validated the capital's region during setup, but `OnCampaignLoaded` only called `EnsureScenario`. That method intentionally preserves an existing town. A legacy `human_capital` saved against `human_home` could therefore remain ready after F9; construction/hire rejected it, while `AdvanceDay` could still advance its construction. This was a concrete compatibility path: the older capital authoring script used `human_home`.

The revised `SoulFounderPlaytestStateSubsystem.cpp` now shares `ValidateSettlementDevelopmentBinding` between initialization, readiness and successful-load handling (lines 254, 281, 288 and 594). It checks saved region, settlement faction and campaign-region ownership. It preserves incompatible saved progress, reports the proof unready, and blocks day advancement. A later valid restore can recover because the callback does not require prior readiness.

`SoulSettlementDevelopmentTests.cpp:186` now restores wrong-region and wrong-faction snapshots and invokes the actual `OnCampaignLoaded` UFUNCTION through `ProcessEvent`. It checks preserved data, rejected readiness, no day advancement and recovery. The expected two error logs are declared. **Repair verified by source inspection; test execution pending.**

## Reviewed behavior

- Scenario validation rejects invalid identifiers, duplicate definitions, invalid levels/durations/costs and missing/self/repeated/cyclic prerequisites before seeding. Authored development definitions convert into existing SoulCore rules rather than a new mutable authority.
- `EnsureScenario` defaults to preserving existing saved settlement state. Bootstrap uses that shared path. Explicit replacement remains opt-in through its existing flag.
- Construction spends through `FSoulTownRules::BeginConstruction`; the caller does not duplicate the charge. Campaign and settlement construction advance once per accepted day. Battle/persistence locks, location/ownership, prerequisite/resource/max-level checks and the existing tavern hire service remain connected.
- Campaign and settlement persistence remain the existing two RB Save domains. No new save authority or schema change is introduced. The scoped-save/battle test checks construction progress, hired-hero state, survivors and exactly-once battle result application.
- `SoulSettlementBuildingActor.cpp:94` checks construction before integrity, fixing both zero-integrity first construction and an upgrade retaining its former integrity. The fourth native test covers all four actor groups plus hidden/collision state.

## Compile and execution boundaries

No definite standalone-header or UHT blocker was found. The development test explicitly includes `UObject/Package.h`, respecting the `11cd71c` no-PCH fix. The presentation test includes World, SceneComponent and ScopeExit headers; its `CreateWorld`/`InitializationValues` use matches the installed UE5.8 declarations. The inline scenario accessor's forward declaration is compatible with the inspected UE object-pointer implementation. These checks are not a compiler result.

The four reviewed automation tests are `DevelopmentDefinitionsRejectInvalidData`, `ConstructionGatesCostsDaysAndExistingService`, `DevelopmentSurvivesScopedSaveAndBattleBoundary` and `ConstructionPresentationPrecedesIntegrity`, under `Soul.Integration.Settlement`.

Remaining acceptance gates are the parent's native build/automation, real asynchronous save/load, and a completed licensed environment demonstrating the same state in miniature, visit and battle. The transient actor test does not prove nested LevelInstance visibility/collision, native collision preservation, scene dependency closure or visual quality. Those are known integration boundaries, not newly introduced regressions. This review launched no UE process, ran no build and changed no source or donor asset.
