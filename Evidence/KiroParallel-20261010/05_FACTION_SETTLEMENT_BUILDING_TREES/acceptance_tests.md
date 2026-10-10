# Acceptance Tests — Faction Settlement Development Trees

Lane 05. These tests are written to extend the EXISTING automation patterns so Codex (the Unreal integration lane) can implement them without a new framework.

Reference existing tests to mirror:
- `Source/Soul/Private/Tests/SoulSettlementDevelopmentTests.cpp`
  - `FSoulDevelopmentDefinitionValidationTest` ("Soul.Integration.Settlement.DevelopmentDefinitionsRejectInvalidData")
  - `FSoulDevelopmentCommandTest` ("Soul.Integration.Settlement.ConstructionGatesCostsDaysAndExistingService")
- `Source/SoulCore/Private/Tests/SoulMechanicsTests.cpp` (SoulCore rule-level tests)
- `Source/Soul/Private/Tests/SoulHeartlandTests.cpp` (Heartland JSON loader + clamp)

All tests must stay deterministic and presentation-independent (AGENTS.md).

---

## A. Data-validation acceptance (per faction, SoulCore rules unchanged)

For each of DwarfHold / OrcCamp / VikingHarbour development definitions:

- **AT-V1 Valid tree validates.** Loading the faction `USoulSettlementScenarioData` (seeded from the faction `*Development.json`) returns `ValidateDefinition(Error) == true`.
- **AT-V2 Unique IDs.** Duplicate building id in `Buildings` or `DevelopmentDefinitions` is rejected with a nonempty Error (mirror the existing `duplicate initial id` / `duplicate definition id` cases).
- **AT-V3 Development ids present in initial Buildings.** Every `DevelopmentDefinitions[i].BuildingId` exists in `Buildings` (mirror `missing definition building`).
- **AT-V4 Founder timing clamp.** Every `BuildDays` is in [1,3]. The SoulCore rule only requires >=1; add a faction-loader clamp mirroring `SoulHeartlandContent.cpp` ("buildings must take 1-3 days"). `BuildDays==0` and `BuildDays==4` are both rejected.
- **AT-V5 DAG.** A cycle in prerequisites is rejected (mirror `cyclic prerequisites`). Verify specifically the longest chains:
  - Dwarf: kings_hall → stoneguard_hall → hammer_hall → kings_guard_hall → dragon_eyrie.
  - Orc: warchief_hall → grunt_barracks → shield_pit → berserker_pit → brute_hall → elephant_yard.
  - Viking: great_hall → raider_longhouse → shield_hall → huscarl_hall → wolf_kennels.
- **AT-V6 Nonnegative costs with resource ids.** A negative cost or empty resource key is rejected (mirror `negative cost`).
- **AT-V7 Unique unlock ids per building.** Duplicate unlock in one building's `UnlockIds` is rejected.
- **AT-V8 Single service node.** `FindUniqueServiceDefinition(service.tavern_hero)` returns exactly one building (dwarf.caravan_hall / viking.great_hall-or-tavern-equivalent / orc.warchief_hall-service) OR nullptr if ambiguous — NEVER two. Any faction tree with two `service.tavern_hero` carriers must fail this test.

## B. Construction / timing acceptance (SoulCore + Town rules unchanged)

Mirror `FSoulDevelopmentCommandTest`:

- **AT-C1 Prerequisite gate.** `FSoulSettlementRules::CanBeginConstruction` is false for a building whose prerequisite is Unbuilt/Building/Ruined, true once prerequisites are Intact (Damaged still satisfies).
- **AT-C2 Cost gate.** `FSoulTownRules::BeginConstruction` fails when the economy cannot afford `BuildCost`; succeeds and debits exactly the cost when it can.
- **AT-C3 Day countdown.** After `BeginConstruction`, `ConstructionDaysRemaining == BuildDays` (normal=1 completes next `AdvanceDay`; significant=2 after two; capstone=3 after three). Assert Level increments to 1 and Condition becomes Intact with IntegrityPermille 1000 at completion (per `FSoulSettlementRules::AdvanceDay`).
- **AT-C4 One decision per day.** With the Heartland-style per-day guard, a second `BeginSettlementConstruction` in the same `Economy.Day` is rejected (mirror the `HeartlandConstructionDay==Economy.Day` rule); allowed after `AdvanceDay`.
- **AT-C5 Max level cap.** A building at `Level == MaxLevel` cannot begin construction (e.g. dwarf.great_forge at L2, orc.spoils_market at L2).

## C. Effect acceptance (requires additive effect wiring; see implementation_order)

- **AT-E1 Service unlock (NO new code).** For the service node carrying `service.tavern_hero`, `IsTavernOperational()` is true only when that building is operational (Level>0, Intact/Damaged, integrity>=500) and the settlement is player-owned in the right region. This already works generically via `FindUniqueServiceDefinition` — assert it works for a dwarf/orc/viking settlement id, proving the Human-only assumption is gone.
- **AT-E2 Recruitment gate.** A faction core-fighter pool with `RequiredBuildingId = <faction>.<core_barracks>` cannot recruit until that building is operational; `FSoulTownRules::RecruitFromBuilding` refuses otherwise (mirror `CanRecruitHumanCompany`). Growth scales with integrity (`EffectiveWeeklyGrowth`).
- **AT-E3 Economy effect generalization.** The L2 market/mine income effect fires for the faction economy building. NOTE: today this is hardcoded against `human.market` in `SoulFounderPlaytestStateSubsystem::AdvanceDay`. AT-E3 asserts the GENERALIZED dispatch (driven off `economy.market` unlock id, not a literal `human.market` string) so dwarf.mine / orc.spoils_market / viking.market all benefit. If generalization is not done, AT-E3 is expected-fail and documents the gap.
- **AT-E4 Spell gate (if magic nodes shipped).** A faction magic building unlocks exactly the faction's school spell via the same RequiredBuilding mechanism as `RefreshHeartlandSpellLearning` + HeartlandDevelopment.json `spells`. Expected-fail until the hardcoded human spell map is generalized.

## D. Save acceptance (no schema change for additive trees)

- **AT-S1 Round-trip.** `USoulSettlementStateSubsystem` captures and restores a dwarf/orc/viking settlement's buildings `{id, level, integrity, days, condition}` deterministically (name-sorted), schema version 1 unchanged. Mid-construction `ConstructionDaysRemaining` survives a save/load.
- **AT-S2 No schema bump needed.** Confirm no new persisted per-building field and no new `ESoulBuildingCondition` value were introduced by the trees (they were not). If a future effect needs a new persisted field, AT-S2 becomes a required `SoulSettlementSaveSchema` bump test.
- **AT-S3 Occupation preserves identity.** While a settlement region is occupied by another faction, construction pauses and resumes on recapture (mirror the `bFourFactionAlpha` occupation branch in `AdvanceDay`).

## E. Faction-differentiation acceptance (design-intent regression)

- **AT-D1 Not a rename.** Automated structural check: no faction tree is a 1:1 id/field clone of another with only the prefix changed. Assert at least these differences hold:
  - Orc core-fighter (`orc.grunt_barracks`) BuildDays == 1 while Dwarf core-fighter (`dwarf.stoneguard_hall`) BuildDays == 2.
  - Viking has a mandatory maritime economy node (`viking.shipyard`) gating its elite capstone (`viking.huscarl_hall` prereqs include `viking.shipyard`); no other faction's elite requires a trade node.
  - Each faction's magic node proposes a distinct school (dwarf Earth/Rune, orc Lightning/Blood, viking Water/Storm, human Frost) — distinct `spell.*` unlock ids.
- **AT-D2 Shared-system reuse.** Each faction root carries `control.settlement`; each has exactly one `service.tavern_hero` service carrier; each has at least one `economy.market` and one `siege.defense` building — proving structural parity with the Human proof without copying content.

---

## Expected-fail register (honest state)

The following tests are EXPECTED TO FAIL until the additive code in `implementation_order_and_dependencies.md` lands:
- AT-E3, AT-E4 — require generalizing the hardcoded `human.market` / human-spell dispatch.
- AT-E2 for non-core units — require real unit ids/meshes (only *_warrior per faction is wired).
- Any apex/beast assertion — requires an apex roster/animation pipeline (none in snapshot).

Validation (A), construction/timing (B), service unlock (AT-E1), and save (D) are expected to PASS with pure data authoring + the generic service path that already exists.
