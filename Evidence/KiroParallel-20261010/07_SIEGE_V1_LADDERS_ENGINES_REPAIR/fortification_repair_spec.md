# Fortification Repair Spec — Soul Siege V1 (Phase G)

Lane 7. Research/design only; no runtime edits. Labels: **VCS** verified-from-snapshot,
**VOA** verified-owned-asset, **LRC** local-check-required, **DP** design-proposal.

Repair is the **cleanest** V1 feature to land because the damage/repair *primitives already exist* —
what is missing is the campaign economy, the over-time progression, and the in-battle→town-view
scar/repair wiring.

---

## 1. What already exists (VCS)

### Settlement wall + building state and operations
`Source/SoulCore/Public/SoulSettlement.h` / `Source/SoulCore/Private/SoulSettlement.cpp`:

| Primitive | Signature | Current behavior (VCS) |
|---|---|---|
| Wall integrity | `FSoulSettlementState::WallIntegrityPermille` (default 1000) | Persisted state. |
| Damage walls | `FSoulSettlementRules::DamageWalls(State, DamagePermille, ScarId)` | Clamps 0..1000; adds a **permanent** scar to `PermanentScars` if `ScarId` set. |
| Repair walls | `FSoulSettlementRules::RepairWalls(State, RepairPermille)` | Clamps 0..1000. **Instant, free, no cost, no time, does NOT remove scars.** |
| Building integrity | `FSoulBuildingState::IntegrityPermille` + `ESoulBuildingCondition {Unbuilt,Building,Intact,Damaged,Ruined}` | Persisted. |
| Damage building | `DamageBuilding(State, Id, Dmg, ScarId)` | Clamps; sets Damaged/Ruined; adds permanent scar. Rejects Unbuilt/Building. |
| Repair building | `RepairBuilding(State, Id, RepairPermille)` | **Instant, free**; sets Intact at >=1000 else Damaged. Rejects Unbuilt/Building. **Note: a Ruined (0 integrity) building is NOT rejected and becomes Damaged on any repair — so ruin recovery is already possible but uncosted.** |
| Day advance | `AdvanceDay(State)` | **Only** counts down construction `ConstructionDaysRemaining`; completes buildings to Intact/1000. **Does nothing for wall or building repair over time.** |
| Projection | `Project(State)` | Emits VisibleBuildings/DamagedBuildings/RuinedBuildings + wall integrity + scars for presentation. |

### Siege → settlement aftermath (VCS)
`Source/SoulCore/Private/SoulSiegeAftermath.cpp` (`FSoulSiegeAftermathRules::Apply`):
- Applies `WallDamagePermille` via `DamageWalls` using the lexically-first scar id.
- Applies per-building damage via `DamageBuilding` with id `siege_<building>`.
- Adds every `ScarId` to `PermanentScars`.

Campaign wiring (VCS, `Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp:555-562`): on siege
resolve, `WallDamagePermille = max(0, Town->WallIntegrityPermille - (SiegeGateRemaining*1000/GateMaximum))`
and, if the gate reached zero, scar `human.gate.breached` is added.

### Presentation (VCS)
`Source/Soul/Private/SoulSettlementPresentationController.cpp:108-123` reads `GetWallIntegrity` +
scars and calls `ASoulFortificationSegmentActor::ApplyWallState(integrity, /*bRepairing=*/false)`.
Building actors get a `Repairing` condition path (`:100`, `ApplyIntegrity(..., bConstructing=Condition==Building||Repairing)`).

### The gaps (VCS)
1. **`RepairWalls`/`RepairBuilding` have no cost and no time.** Any caller could restore walls for
   free, instantly.
2. **`AdvanceDay` never progresses repair.** There is no "repairing" wall lifecycle.
3. **`ApplyWallState` is ALWAYS called with `bRepairing=false`.** The segment actor's `RepairRoot` /
   `RepairActors` scaffolding branch (VCS present in `SoulFortificationSegmentActor.cpp`) is **dead —
   nothing ever shows scaffolding.**
4. **Scars are permanent and never cleared.** `city_siege_blueprints.json` rule says
   "scars MAY remain after functional repair" and the plan doc says "Repair can restore function
   before the visual scar completely disappears" — but there is no mechanism to *ever* clear a scar,
   so "may remain" is currently "always remains forever".
5. **No "Repairing" ESoulBuildingCondition.** The enum has no `Repairing` member even though the
   presentation controller string-compares `TEXT("Repairing")`. So a repairing *building* visual is
   also effectively unreachable unless a subsystem injects that condition name out-of-band.

---

## 2. Design proposal (DP)

### 2.1 Repair as a costed, timed campaign order
Introduce a repair *job*, not a free setter. Keep the authority in `FSoulSettlementRules`
(the existing settlement authority) — **no new repair authority.**

```
struct FSoulRepairJob {              // DP — lives in SoulSettlement.h
  FName Target;                      // NAME_None = walls; else building id
  int32 RemainingDays = 0;
  int32 PermilleRestoredPerDay = 0;  // from repair profile
  int32 ReservedCost = 0;            // treasury already committed
};
```

- Add `TArray<FSoulRepairJob> ActiveRepairs;` to `FSoulSettlementState`.
- New `FSoulSettlementRules::BeginRepair(State, Target, Profile, Treasury&, Error)` validates:
  target is Damaged/Ruined (reuses `Condition`), treasury can pay (reserves cost), not already repairing.
- Extend `AdvanceDay` to tick `ActiveRepairs`: each day apply `PermilleRestoredPerDay` via the
  **existing** `RepairWalls`/`RepairBuilding`, decrement `RemainingDays`, and when a *building* repair
  starts, set its condition to a new `ESoulBuildingCondition::Repairing` so the existing presentation
  path lights up.

### 2.2 Scar lifecycle (fix "may remain")
- Keep `PermanentScars` as the *historical* record, but add a separate transient
  `TSet<FName> ActiveBreachScars` that drives the breached-segment visual.
- On functional repair completion, **move** the scar from active to a `HealedScars` set (or clear from
  `ActiveBreachScars`) so the segment stops rendering `BreachedRoot` and the town reads repaired —
  while the permanent historical scar still exists for narrative/aftermath. This realizes
  "function restored before the visual scar disappears": during `Repairing`, show scaffolding; on
  completion, drop the breach visual.
- `SoulSettlementPresentationController` keys `bPersistentlyBreached` off `ActiveBreachScars`
  instead of `PermanentScars`, and passes `bRepairing=true` while a matching `FSoulRepairJob` is
  active — **finally exercising the dead `RepairRoot` branch.**

### 2.3 Repair profile data (DP, new data file — NOT a runtime edit by this lane)
A `Data/fortification_repair_profiles.json` (authored later by Codex) parameterizing per-faction:
`wall_repair_days`, `wall_permille_per_day`, `wall_cost`, `building_repair_days`, `scar_heal_policy`.
Faction asymmetry is already specified in `Data/city_siege_blueprints.json` damage_strategy:
- Humans: scaffolding/patches, scars may remain.
- Orcs: "repair adds timber/stakes/scavenged material instead of restoring masonry" — repair should
  bias toward a *patched* visual, never pristine reset.
- Vikings: "fresh boards/scaffolding/fire scars are part of faction history" — repair visual is fresh
  timber, not reset.
- Nature: "regrows/patches gradually" — longer repair time, gradual.
This is **DP**; the data values are a starting proposal, final numbers are design-tunable.

### 2.4 Attacker-side counterpart: gate repair during a *held* siege (DP, optional, V1.5)
V0 handoff lists "no new gate repair" as a known limit. A defender could spend a bounded resource to
restore gate integrity *between* assaults (campaign layer), reusing `RepairWalls`-style logic on the
siege gate's backing `WallIntegrityPermille`. **Not** mid-battle (that would violate
`no_mid_battle_construction`). Recommend deferring to V1.5.

---

## 3. Acceptance criteria (feed acceptance_tests.md)

- **REPAIR-A1 (VCS-regression):** With no repair job, `AdvanceDay` leaves wall/building integrity
  unchanged except construction — identical to current behavior.
- **REPAIR-A2:** `BeginRepair` on an intact target is rejected; on a Damaged/Ruined target with
  sufficient treasury it reserves the exact cost and creates one job.
- **REPAIR-A3:** `BeginRepair` with insufficient treasury is rejected atomically (no partial reserve),
  mirroring the siege result "reject malformed atomically" discipline (SoulSiegeCampaignTests.cpp).
- **REPAIR-A4:** Over N days, integrity rises by exactly `N * PermilleRestoredPerDay` (clamped 1000),
  and the job clears on completion; condition returns Intact.
- **REPAIR-A5 (visual):** While a wall repair job is active, the presentation controller calls
  `ApplyWallState(integrity, /*bRepairing=*/true)` and the `RepairRoot` scaffolding shows; on
  completion it reverts and the active breach scar is cleared.
- **REPAIR-A6 (persistence):** Active repair jobs + scar sets round-trip through the existing
  `Soul.Settlements` save domain and a fresh-process restore (mirror `SoulSettlementSaveTests.cpp` /
  `SoulSiegeCampaignTests.cpp` exact-restore pattern). No new save domain / schema bump; malformed
  partial repair state rejects atomically; missing fields clear stale repair cleanly.
- **REPAIR-A7:** A healed scar no longer renders breached but remains in the permanent historical
  record; re-breaching re-activates it.

---

## 4. Exact files/symbols for Codex (smallest clean seam)

| Concern | File / symbol (VCS) | Change kind |
|---|---|---|
| Repair job state | `Source/SoulCore/Public/SoulSettlement.h` `FSoulSettlementState` | add `ActiveRepairs`, `ActiveBreachScars`; add `ESoulBuildingCondition::Repairing` |
| Repair rules | `Source/SoulCore/Private/SoulSettlement.cpp` `BeginRepair` (new), `AdvanceDay` (extend) | extend existing authority |
| Aftermath → active scar | `Source/SoulCore/Private/SoulSiegeAftermath.cpp` `Apply` | route scars into `ActiveBreachScars` too |
| Campaign repair order | `Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp` (AP/day advance + settlement authority) | call `BeginRepair`; charge AP/treasury |
| Repairing visual | `Source/Soul/Private/SoulSettlementPresentationController.cpp:109-123` | pass `bRepairing` + key breach off active scars |
| Dead branch now used | `Source/Soul/Private/SoulFortificationSegmentActor.cpp` `RepairRoot`/`RepairActors` | no change needed; finally driven |
| Save round-trip | existing `Soul.Settlements` domain capture/restore in `SoulSettlementStateSubsystem` | serialize `ActiveRepairs`/`ActiveBreachScars` |

---

## 5. Explicit unknowns / contradictions

- **CONTRADICTION (VCS vs design intent):** `RepairWalls`/`RepairBuilding` are documented in
  `city_siege_blueprints.json` and the plan doc as *costed, gradual, scar-aware*, but the current code
  is *free, instant, scar-blind*. V1 must reconcile this; the current code is a stub, not the design.
- **DEAD VISUAL (VCS):** `RepairRoot`/`RepairActors` scaffolding branch and the `Repairing` building
  condition string are referenced but never driven. Repair visuals are specified but non-functional.
- **LRC:** Whether `L_HumanCapital_Authored` actually places `ASoulFortificationSegmentActor`
  instances with populated `RepairActors`/`IntactActors` etc. is unknown from source; the actor
  supports it but authored data presence is a local-editor check.
- **No new asset is required for repair** beyond scaffolding meshes already implied by the segment
  actor's authored arrays; mesh sourcing (if absent) is LRC against owned medieval/castle kits (VOA).
