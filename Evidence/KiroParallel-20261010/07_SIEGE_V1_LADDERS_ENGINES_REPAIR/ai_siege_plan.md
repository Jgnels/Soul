# AI Siege Preparation & Multi-Approach Plan — Soul Siege V1 (Phase F2 + D)

Lane 7. Research/design only. Labels: **VCS**, **VOA**, **LRC**, **DP** (see other artifacts).

Goal: let AI factions **prepare** a siege (acquire ladders/ram/engine over turns, choose an approach,
garrison defenders on walls) instead of the current single snap decision, without creating a second
strategic authority.

---

## 1. What the AI does today (VCS)

`Source/SoulCore/Private/SoulStrategyAI.cpp` — `FSoulStrategyAI::Evaluate` /
`FSoulStrategyAI::Choose` (`SoulStrategyAI.cpp:173-199, ~205-240`):

- `ESoulStrategyAction::SiegeSettlement` is one of seven candidate actions scored each turn:
  `Hold, Recover, DefendOwnedRegion, CaptureResourceRegion, AttackVisibleArmy, SeizeExposedRegion,
  SiegeSettlement`.
- Legality for a siege (VCS, exact):
  - region must be `bAdjacent && bSettlement`, not owned by the actor;
  - actor `Snapshot.Readiness >= 600`;
  - force gate: `Snapshot.Strength * 100 >= max(1, R.GarrisonStrength) * 70` (i.e. ≥70% of garrison).
- Score: `600 + R.Value + R.Feasibility + clamp((Strength - GarrisonStrength)/10, -400, 400) - R.TravelCost`.
- Reasons tagged: `adjacent_enemy_settlement`, `siege_force_ratio_acceptable` (or `no_viable_siege_target`).
- `Choose` picks the best legal candidate; it is deterministic with a lexical tie-break on target id.

**VCS conclusion:** AI siege is a *pure, deterministic strategic score*. There is **no** siege
*preparation* concept at all — no ladders/ram/engine acquisition, no prep time, no approach selection.
When the ratio is met the AI simply initiates. The battle then runs with `FSoulSiegeRules::Begin({})`
(empty preparation) exactly like the player path.

---

## 2. Design: deterministic, explainable siege preparation (DP)

Keep the existing single-authority, additive-component, deterministic scoring style. Add a
*preparation accrual* layer that sits **between** choosing to besiege and committing the assault.

### 2.1 Preparation as strategic turns, not a toggle
A new strategic sub-state per besieging army (DP), parameterized by data, not hard-coded:

```
struct FSoulSiegePlan {                 // DP — SoulStrategyAI side-state
  FName TargetRegion;
  int32 PrepTurnsInvested = 0;
  FSoulSiegePreparation Prepared;       // REUSE the existing struct (SoulSiege.h) — no new flags
  int32 CommittedCost = 0;
};
```

- Introduce a new action `ESoulStrategyAction::PrepareSiege` (DP) scored like `SiegeSettlement` but
  representing "invest a turn acquiring siege capability against an adjacent fortified settlement."
- Each prep turn, deterministically unlock the next preparation flag by a fixed priority ladder
  (cheapest/most-useful first), paying real cost from treasury (ties into Phase F1 economy):
  `bLadders -> bBatteringRam -> (bWallBreach via engine) -> bSiegeTower`.
- `SiegeSettlement` becomes legal to *commit* only when the plan satisfies `CanAssaultWalls`
  (VCS: `FSoulSiegeRules::CanAssaultWalls(Prep, bHasFlyingUnit)` already returns true if any of
  ladders/tower/breach/ram OR a flying unit). This makes the **existing rule** the gate: an AI with
  no preparation and no flyers *cannot* meaningfully assault walls, which is already the intent.

### 2.2 Explainable scoring additions (DP)
Add additive reason-tagged components so the decision stays debuggable (matching the existing
`AddComponent` + `C.Reasons.Add` style):

| Component | Meaning | Sign |
|---|---|---|
| `prep_readiness` | +N per preparation flag already acquired | + |
| `defense_hardness` | fortification level / wall integrity of target | − to commit, + to prep |
| `engine_available` | owns required asset capability (ties to prep spend) | + |
| `prep_time_cost` | turns already spent (sunk-cost dampener to avoid dithering) | − |
| `rival_history` | reuse existing `FSoulMemoryRules::RivalBias` (VCS already used in AttackVisibleArmy) | ± |

### 2.3 Flying bypass stays honored (VCS)
`CanAssaultWalls(..., bHasFlyingUnit=true)` already lets flyers assault without engines. The AI prep
ladder must therefore **skip preparation entirely** for armies with a flying unit (griffon/dragon)
and go straight to commit — do not force a dragon army to build ladders. (VCS rule, DP wiring.)

### 2.4 Defender-side AI preparation (DP)
Symmetric: a defending AI with a fortified capital should prefer `DefendOwnedRegion` and spend to set
defender preparation already modeled in `FSoulSiegePreparation`:
`bReinforcedGate` (VCS: already drives GateMaximum 1300 and is set at FortificationLevel>=2),
`bAmmoStores` (archer resupply — needs runtime consumer, see engine spec), `bDefenderBarricades`
(inner-line slowdown — needs runtime consumer), `bMagicalWard` (reduce engine/magic — needs consumer).
Defender depth (Phase D) gives these runtime meaning.

---

## 3. Multiple approaches (ties to Data authority)

`Data/city_siege_blueprints.json` (VCS) already enumerates per-city approaches, e.g. Human Capital:
`"approaches": ["main_road","side_wall","flying"]` and physical objectives
`["gatehouse","forge","witch_collegium","keep"]`.

- AI should **choose an approach** as part of `FSoulSiegePlan` based on prepared capability:
  `main_road` (ram/gate), `side_wall` (ladders/tower/breach), `flying` (if flyer present).
- Approach choice is deterministic from prepared flags + target geometry value (data-driven), tagged
  with a reason (`chose_side_wall_due_to_ladders`, etc.).
- **LRC:** the authored map must actually expose a distinct side-wall approach with nav; today only
  the main gate approach + courtyard is proven (VCS, `SoulHumanCapitalSiege.cpp` route survey). Until
  a second approach is authored and nav-proven, AI side-wall selection is a plan with no battlefield —
  so Phase A/B/C/D must land the physical second approach before AI approach-selection is enabled.

---

## 4. Anti-degeneracy (deterministic, mirrors existing discipline)

- **No instant-engine cheese:** AI cannot possess ladders/ram/tower without having spent prep turns +
  treasury (prevents a sudden fully-equipped siege).
- **No prep-dithering loop:** `prep_time_cost` sunk-cost dampener + a hard cap on prep turns forces a
  commit-or-abandon decision; abandonment releases committed cost per existing treasury rules.
- **No suicide assault:** keep the existing ≥70% force ratio AND require `CanAssaultWalls` true;
  an under-prepared, under-strength AI holds/recovers instead (existing `Recover` action).
- **No recapture churn:** reuse `FSoulMemoryRules` rival bias so an AI that just lost a siege does not
  immediately re-besiege the same wall; the memory layer already dampens repeat attacks (VCS).
- **Deterministic:** all additions are additive integer components with lexical tie-breaks, exactly
  like the current `Evaluate`/`Choose`. No randomness introduced.

---

## 5. Two-faction test sketch (feeds campaign-sim expectations; detail in acceptance_tests.md)

- **Seed 1 — Dwarves besiege Human Capital (ground army, no flyer):** over turns the AI must accrue at
  least `bLadders` or `bBatteringRam` before `SiegeSettlement` becomes the chosen action; assert the
  committed battle descriptor carries non-empty preparation (post-B1); assert no commit while
  `CanAssaultWalls` is false and strength <70% garrison.
- **Seed 2 — Orcs with a War Elephant / a flyer-capable faction:** assert flying bypass skips prep;
  assert deterministic approach selection differs from Seed 1; assert memory bias prevents immediate
  re-siege after a defeat.
- Both seeds assert **determinism** (same seed → identical candidate ordering and reasons) and
  **explainability** (every commit carries the prep + approach reason tags).

---

## 6. Exact files/symbols for Codex

| Concern | File / symbol (VCS) | Change kind |
|---|---|---|
| Siege scoring | `Source/SoulCore/Private/SoulStrategyAI.cpp` `Evaluate(SiegeSettlement)` + `Choose` + `Actions[]` | add `PrepareSiege` action + prep components |
| Action enum | `Source/SoulCore/Public/SoulStrategyAI.h` `ESoulStrategyAction` | add `PrepareSiege` |
| Prep struct reuse | `Source/SoulCore/Public/SoulSiege.h` `FSoulSiegePreparation` | REUSE (no new struct) |
| Wall-assault gate | `Source/SoulCore/Private/SoulSiege.cpp` `CanAssaultWalls` | REUSE as the commit gate |
| Memory bias | `Source/SoulCore/Private/SoulMemory.cpp` `FSoulMemoryRules::RivalBias` | REUSE |
| Prep → descriptor | `Source/Soul/Private/SoulControlledCampaignAction.cpp` + `SoulFounderPlaytestStateSubsystem.cpp` `ApplySettlementEnvironment` | populate transported preparation (needs B1) |

## 7. Explicit unknowns / contradictions

- **CONTRADICTION (VCS):** The AI "can besiege" the moment force ratio is met, but the realtime siege
  always begins with empty preparation — so the AI never actually "brings" ladders/engines even though
  the strategic layer decided to besiege. V1 must connect the decision to the preparation.
- **LRC:** A second physical approach (side-wall) does not exist in the authored map yet; AI
  approach-selection cannot be meaningfully qualified until Phase B/C/D author it.
- **DP numbers unknown:** prep turn counts, costs, and the priority ladder are starting proposals;
  final tuning is a design decision for a campaign-sim pass (coordinate with Lane 2 AI army lane for
  shared treasury/stock assumptions — see conflicts section of HANDOFF.md).
- `bAmmoStores`/`bDefenderBarricades`/`bMagicalWard` have no runtime consumers today (VCS); defender
  AI prep is only meaningful after Phase D gives them effect.
