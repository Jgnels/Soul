# Soul Lane 3 — Skill Domains

Snapshot authority: `handoff/soul-kiro-20261010` @ `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c` (VERIFIED resolves).

Labels used throughout:
- **VERIFIED FROM CURRENT SNAPSHOT** — reads directly from frozen source.
- **DESIGN PROPOSAL** — additive, not yet in code.
- **LOCAL RUNTIME/ASSET CHECK REQUIRED** — needs an in-editor check against the local Unreal project.

---

## 1. What exists today (VERIFIED FROM CURRENT SNAPSHOT)

Hero skills are a flat `TMap<FName,int32> FSoulHeroState::Skills` (`Source/SoulCore/Public/SoulHero.h`). The only engine that reads/writes it:

- `FSoulHeroRules::SpendSkillPoint(Hero, SkillId, MaxRank=3)` — generic rank-up, caps at `MaxRank` (`Source/SoulCore/Private/SoulHero.cpp`).
- `USoulFounderPlaytestStateSubsystem::ChooseSkill(FName Id)` (`Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp`) forwards `MaxRank=2` and gives special effects to exactly **two** ids:
  - `Adventure` → `Economy.MaxActionPoints++`, `ActionPoints++`; and in `MovePlayerTo`, `Skills.Adventure >= 2` (and hero `IsAvailable`) makes a strategic move cost **0 AP**.
  - `Magic` → `Hero.MaxMana += 10`, `Hero.Mana += 10`.
- `war_magic` appears only in a unit test (`SoulMechanicsTests.cpp`, default cap 3) and has **no gameplay effect**.

So the live, shipping skill surface is **two skills with a rank cap of 2**. Everything below is a proposal that must stay backward compatible with those two ids and with existing saves.

---

## 2. Domain model (DESIGN PROPOSAL)

Six small domains, each 2–4 skills, rank cap **3**. Namespaced stable ids `domain.skill`. The two legacy ids are preserved as aliases so no save or test breaks:

| Legacy id (VERIFIED) | New canonical id | Verified effect kept |
|---|---|---|
| `Adventure` | `logistics.pathfinder` | +1 Max/Current AP per rank; rank≥2 → free strategic move |
| `Magic` | `magic.attunement` | +10 MaxMana (+10 current) per rank |

> **Compatibility rule:** `ChooseSkill` must accept BOTH the legacy id and the new id, applying identical effects. On load, legacy-keyed `Skills` entries are read as-is (no migration needed because the map is keyed by `FName` and both ids resolve to the same effect handler).

### 2.1 `command` — leading an army
- `command.rally` (cap 3): bounded morale/initiative bump for the hero's army. Presentation-independent; feeds RBCombat as an additive morale modifier (bounded, per AGENTS.md "bounded morale/luck").
- `command.hold_the_line` (cap 3): reduces routing chance on HOLD; coordinates with Lane 10 HOLD behavior.
- `command.vanguard` (cap 2): small CHARGE impact bonus; must not force melee if a ranged posture is valid (Lane 10 constraint).

### 2.2 `martial` — personal combat
- `martial.duelist` (cap 3): hero's own attack vs enemy heroes/apex.
- `martial.bulwark` (cap 3): hero's own defense / wound resistance. **Does not** grant immunity to capture; a 0-survivor army still captures the hero (VERIFIED `ApplyBattleInjury`).

### 2.3 `magic` — spellcasting (mana-governed)
- `magic.attunement` (cap 3) = legacy `Magic`: +10 MaxMana/rank.
- `magic.channeling` (cap 3): +1 daily mana regen/rank (VERIFIED base regen is +6/day in `AdvanceDay`). Deterministic.
- `magic.focus` (cap 2): bounded reduction of a spell's effective mana cost (clamped ≥ 1). Must route through the existing spell/mana authority, NOT a second magic authority (VERIFIED magic admission in `SoulRealtimeBattleArena.cpp` / `SoulBattlePlayerActions.cpp` and `ConfigureHeartlandMagic`). **LOCAL RUNTIME/ASSET CHECK REQUIRED** to confirm where per-cast cost is finally read in RBCombat.

### 2.4 `logistics` — strategic movement & supply
- `logistics.pathfinder` (cap 3) = legacy `Adventure`.
- `logistics.forced_march` (cap 2): improves `FSoulLogisticsRules::ForceMarch` outcome / reduces fatigue. VERIFIED logistics authority: `Source/SoulCore/Public/SoulLogistics.h`, `FSoulLogisticsRules`.
- `logistics.quartermaster` (cap 2): slows supply decay on travel.

### 2.5 `diplomacy` — negotiation
- `diplomacy.envoy` (cap 3): improves diplomacy preview/decision scoring. VERIFIED gate: `CanPerformDiplomacy == IsAvailable`; hooks `PreviewDiplomacy`/`ExecuteDiplomacy` in the subsystem and `SoulDiplomacy.h`.
- `diplomacy.negotiator` (cap 2): improves ransom/exchange leverage. **Hand-off to Lane 4** (prisoner/ransom lifecycle) — this lane only declares the skill id and that its effect is read by Diplomacy V1 scoring, never a second diplomacy authority.

### 2.6 `stewardship` — settlement governance
- `stewardship.builder` (cap 3): reduces construction time or cost **through the existing Soul.Settlements authority only** (VERIFIED authority: `SoulSettlement.h` / `FSoulSettlementRules`, and `USoulSettlementStateSubsystem`). Never a parallel economy.
- `stewardship.steward` (cap 2): small +gold/day when the hero is assigned Governor to a region. V2 (see companion_roles.md).

---

## 3. Determinism & bounds (DESIGN PROPOSAL)

- Every skill effect is a pure integer function of `Skills.FindRef(id)`. No RNG, no float accumulation.
- All combat-facing modifiers are **bounded** (per AGENTS.md and the veterancy precedent: `FSoulVeterancy::CombatBonusPermille <= 100`). Suggested bounds: command/martial modifiers ≤ +100‰ total across ranks; magic cost reduction clamps cost ≥ 1.
- Skill effects apply **only while the hero `IsAvailable` (Healthy)** for army/diplomacy effects, matching the existing `Adventure` free-move gate (`FSoulHeroRules::IsAvailable(Hero) && Skills.Adventure>=2`). Stewardship (Governor) effects may optionally persist while Wounded (hero is at a settlement, not in the field) — flagged as a design choice for Codex.

## 4. Explicit unknowns for this file
- **LOCAL RUNTIME/ASSET CHECK REQUIRED:** exact RBCombat read-site where per-cast mana cost is finalized, to attach `magic.focus` without creating a second magic authority.
- **LOCAL RUNTIME/ASSET CHECK REQUIRED:** whether `command.rally` morale can be injected additively into the current RBCombat morale pipeline (`SoulRealtimeBattleRules` / tactics) without changing deterministic damage tests.
