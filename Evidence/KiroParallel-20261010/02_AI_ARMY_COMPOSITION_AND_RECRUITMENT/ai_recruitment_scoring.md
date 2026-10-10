# AI Recruitment Scoring — Explainable Role-Utility Model (Lane 2)

Source snapshot: `Jgnels/Soul` @ `handoff/soul-kiro-20261010` = `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c`.

Every claim below is tagged:
- **[VERIFIED]** = VERIFIED FROM CURRENT SNAPSHOT (read directly from the frozen commit).
- **[PROPOSAL]** = DESIGN PROPOSAL authored by this lane.
- **[OWNED-ASSET]** = VERIFIED OWNED ASSET METADATA (Copperlight catalog).
- **[LOCAL-CHECK]** = LOCAL RUNTIME/ASSET CHECK REQUIRED.

---

## 1. What exists today (the ground truth we must not re-pave)

**[VERIFIED]** There are two layers:

1. `FSoulStrategyAI::Choose` (`Source/SoulCore/Private/SoulStrategyAI.cpp`) — a pure deterministic **movement/attack/siege** scorer. It evaluates `Hold, Recover, DefendOwnedRegion, CaptureResourceRegion, AttackVisibleArmy, SeizeExposedRegion, SiegeSettlement`. It has **no recruitment, treasury, army-composition, or diplomacy inputs**. Its only snapshot fields are `Readiness, Strength, Regions, VisibleEnemyArmies, Commander` (`FSoulStrategySnapshot`, `SoulStrategyAI.h`). **It is never called by the live campaign — only by `SoulMechanicsTests.cpp`.**

2. `USoulFounderPlaytestStateSubsystem::RunNextAlphaAction` (`Source/Soul/Private/SoulFourFactionAlpha.cpp`) — the **actually-running** four-faction-alpha enemy AI. This is the real recruiter and must be the integration site. It:
   - Recruits while `F.Army.TroopCount < 30`, up to **4 troops per action** (`FMath::Min(4, Pool->Available)`), scoring urgency `350` when `<18` else `130`. **[VERIFIED]**
   - Recruits via `PrepareControlledRecruitment` → `ExecuteControlledRecruitment` → `FSoulCampaignRules::SpendAction` + `FSoulCampaignRules::Recruit`, which **actually deducts gold** and decrements pool stock. **[VERIFIED]** This is the real cost path; recruitment is NOT free.
   - Moves/captures/withdraws/attacks adjacent regions with a force-ratio gate: skips a hostile target when `F.Army.TroopCount*100 < Defenders*85`. **[VERIFIED]**
   - Respects diplomacy via `DiplomacyAllowsHostility` and fog-of-war via `FSoulWorldRules::IsVisible`. **[VERIFIED]**
   - Defends its capital: raises move score to `300` when `CapitalThreat` and the target is its own `Capital(Id)`. **[VERIFIED]**
   - Is fully deterministic: a CRC-of-(`target+faction+seed+day`) tie-break feeds a `Consider` lambda; `AlphaSeed` is restored on load. **[VERIFIED]**
   - **Monoculture by construction:** each AI faction gets only ONE pool — a clone of the human pool renamed to its strategic unit (`InitializeFourFactionAlpha` ~79-86). So every AI army today is **100% one unit**: an all-one-role army. **[VERIFIED]** This is exactly the degeneracy the lane must fix.

**[VERIFIED]** The `AdvanceEnemyAI()` log-string no-op path (`SoulFounderPlaytestStateSubsystem.cpp:658-659`) applies only to the **six-faction** profile (`"Six-faction strategic AI OFF"`) and the single-enemy founder profiles ("garrisons hold"). The four-faction alpha path **does** act. **This corrects a common mis-reading that "the AI never recruits" — it does, but only one unit type.**

> **Integration verdict:** The smallest clean authority is to extend `RunNextAlphaAction`'s recruit branch (and seed richer pools in `InitializeFourFactionAlpha`) rather than wiring up `FSoulStrategyAI`. Reusing `FSoulControlledCampaignAction` keeps a single save/economy authority.

---

## 2. The role-utility model (deterministic, integer-only)

**[PROPOSAL]** Goal: pick ONE unit id to recruit this action that maximizes a simple, explainable utility. Integer math only (no floats) to match the existing permille style and stay bit-reproducible.

For each candidate unit `u` (a pool the faction owns, with `Available>0` and affordable):

```
Utility(u) = GapScore(role(u))
           + CounterScore(role(u))
           + DoctrineBias(role(u))
           + BuildingReadyBonus(u)
           - CostPenalty(u)
           - DegeneracyPenalty(role(u))
```

All terms are integers in roughly the same magnitude band (hundreds) as the existing `RunNextAlphaAction` scores (`130`, `350`, `300`, `220`, `160`) so the recruit-vs-move comparison stays sane.

### 2.1 GapScore — close the distance to target composition
Let `target_i` = desired count of role `i` (from `faction_role_targets.json` weights × current soft-cap), and `actual_i` = current count.

```
GapScore(i) = clamp( (target_i - actual_i) * 60, -300, 400 )
```
- Positive when the role is under-strength (recruit it).
- Capped at `+400` so one role can never dominate the decision.
- **Hard floor rule (anti-degeneracy):** if `actual(melee_line) < min_role_counts.melee_line`, `GapScore(melee_line) += 1000` (always recruit the first line unit first). Symmetrically for `ranged` where `min>0`.

### 2.2 CounterScore — react to visible enemy composition
**[PROPOSAL, depends on MIG-3]** If a hostile army is visible and adjacent (reuse `ArmyCountAtRegion` + a new per-role summary):
```
CounterScore(ranged)      = +120 if enemy is >60% melee_line
CounterScore(melee_line)  = +120 if enemy is >40% ranged   (close the distance / shield them)
CounterScore(support)     = +80  if friendly army already has >=1 melee_line and >=1 ranged
```
If enemy composition is **not visible**, `CounterScore = 0` for all (deterministic fallback to doctrine). Never guess hidden enemy roles — that would break fog-of-war parity. **[VERIFIED constraint: fog-of-war is already enforced via `FSoulWorldRules::IsVisible` in `RunNextAlphaAction`.]**

### 2.3 DoctrineBias
```
Balanced   : no bias.
Aggressive : +80 melee_line, +40 ranged, -60 support   (push numbers early)
Defensive  : +60 ranged, +40 support, -20 melee_line    (hold ground, trade shots)
SiegeFocused: +100 melee_line (bodies to assault), +40 ranged (clear battlements), apex +60 if troop_floor met
```

### 2.4 BuildingReadyBonus
**[VERIFIED mechanism]** `FSoulTownRules::RecruitFromBuilding` / `IsOperational` already gate building-linked pools. If the unit's pool has a `RequiredBuildingId` that is operational at the faction's recruitment settlement, `+40`. If the building exists but is **not** operational (ruined/under-min-integrity), the unit is **ineligible** (utility = -INF; skip). This naturally shifts the AI toward units it can actually produce after a siege.

### 2.5 CostPenalty — respect treasury
```
CostPenalty(u) = (CostPerUnit['gold'] * recruit_qty) / 10
```
Dividing by 10 keeps 140-gold knights (penalty 14/unit) comparable to the score band. A faction near-broke (treasury < one company) simply fails `CanAfford` and the candidate is dropped — **[VERIFIED]** `FSoulCampaignRules::CanAfford` already enforces this.

### 2.6 DegeneracyPenalty — the 65% ceiling
```
If recruiting role i would push actual_i / (total+qty) > max_single_role_fraction_permille (650):
    DegeneracyPenalty(i) = 2000   (effectively forbidden unless it's the only option AND a min-floor is unmet)
else 0
```

### 2.7 Quantity selection
**[PROPOSAL]** After choosing the best unit `u*`, recruit:
```
qty = clamp( min(recruit_cap_per_action=4, Pool.Available, affordable_by_treasury, target_gap(u*)), 1, 4 )
```
This mirrors the **[VERIFIED]** existing `FMath::Min(4, Pool->Available)` behavior but additionally never over-shoots the role target.

---

## 3. Worked example (deterministic)

Viking army, Aggressive doctrine, soft-cap 60, current = `{viking_axe_warrior: 10}` (all melee), treasury 900 gold, both melee and ranged pools seeded (post MIG-2), ranged cost 180, melee cost 140.

Targets (weights 650/280/70): melee 39, ranged 17, support 4.

- melee gap = 39-10 = 29 → GapScore = clamp(29*60,-300,400) = **400**; but fraction already 100% > 65% → DegeneracyPenalty **2000**; min floor melee (1) already met → no +1000. DoctrineBias +80. Net ≈ 400+80-2000-14 = **-1534**.
- ranged gap = 17-0 = 17 → GapScore = **400**; min floor ranged(1) unmet → **+1000**; DoctrineBias +40; CostPenalty (180*4)/10=72. Net ≈ 400+1000+40-72 = **+1368**.

Decision: recruit **ranged** (viking archer), qty = min(4, stock, 900/180=5, gap 17) = **4**. Explanation reasons emitted: `["role_min_floor_unmet:ranged", "composition_gap:ranged", "doctrine_aggressive"]`. Fully reproducible; no RNG.

---

## 4. Reason codes (for the AI report line, reusing `LastAIReport`)

**[PROPOSAL]** Emit human-readable reasons the same way the existing scorer appends `C.Reasons` and the alpha appends to `LastAIReport`:
`role_min_floor_unmet:<role>`, `composition_gap:<role>`, `counter_enemy_melee`, `counter_enemy_ranged`, `doctrine_<name>`, `building_ready:<id>`, `building_down:<id>`, `role_pool_absent:<role>` (MIG-2 not done), `treasury_limited`, `stock_empty:<unit>`, `degeneracy_block:<role>`.

This keeps the AI **explainable** to the founder in the existing "Day N activity / Faction: recruited K infantry at X" recap, which `RunNextAlphaAction` already builds. **[VERIFIED]**

---

## 5. Exact integration points for Codex

| Concern | Exact current symbol | File |
|---|---|---|
| Recruit branch to extend | `USoulFounderPlaytestStateSubsystem::RunNextAlphaAction` (recruit block `if(F.Army.TroopCount<30)`) | `Source/Soul/Private/SoulFourFactionAlpha.cpp` |
| AI pool seeding (add roles) | `USoulFounderPlaytestStateSubsystem::InitializeFourFactionAlpha` (loop over `ActiveAI()`) | `Source/Soul/Private/SoulFourFactionAlpha.cpp` |
| Cost deduction + stock (reuse, do not duplicate) | `FSoulCampaignRules::Recruit`, `::CanAfford`, `::SpendAction` | `Source/SoulCore/Private/SoulCampaign.cpp` |
| Building-gated recruitment (reuse) | `FSoulTownRules::RecruitFromBuilding`, `::EffectiveWeeklyGrowth` | `Source/SoulCore/Private/SoulTown.cpp` |
| Multi-unit army field (add) | `FSoulCampaignArmyState` | `Source/SoulCore/Public/SoulCampaign.h` |
| Visible-enemy count (extend for roles) | `USoulFounderPlaytestStateSubsystem::ArmyCountAtRegion` | `Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp` |
| Deterministic tie-break (reuse) | `Consider` lambda CRC tie-break; `BetterCandidate` | `SoulFourFactionAlpha.cpp`; `SoulStrategyAI.cpp` |
| Action carrier (reuse) | `FSoulControlledCampaignAction` | `Source/Soul/Public/SoulFounderPlaytestStateSubsystem.h` |
| Save fields for pools (reuse) | `CaptureRBSaveDomain_Implementation` (`pools` object) | `Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp` |

**Do not** route recruitment through `FSoulStrategyAI` — it would create a second strategic authority that the live campaign does not use. Keep one authority: the subsystem + `FSoulCampaignRules`.
