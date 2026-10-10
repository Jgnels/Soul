# Soul Lane 3 — Companion & Assignment Roles

Snapshot authority: `handoff/soul-kiro-20261010` @ `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c` (VERIFIED resolves).

## 1. What exists today (VERIFIED FROM CURRENT SNAPSHOT)

File: `Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp` (+ header).

- The playable campaign holds **two hero entities**:
  - `FSoulHeroState Hero` — the player hero, display name **Aurora** (party leader; see `CompanionStatus()`), id `human_founder_hero`.
  - `FSoulHeroState DwarfCommander` — Heartland non-player commander, id `dwarf_king_commander`.
- A **second hero** ("Rowan") is hireable at the tavern:
  - `HireTavernHero()`: requires player at `GetDevelopmentRegion()`, tavern operational, not already hired, ≥ 1200 gold; spends 1200 gold; sets `bSecondHeroHired=true` and `bCompanionAssigned = bHeartlandEnabled`.
  - `AssignHeartlandCompanion(bool)`: Heartland-only; toggles `bCompanionAssigned` when a second hero exists and no battle is pending.
  - `CompanionStatus()`:
    - not hired → `"Rowan / available at the tavern"`
    - hired + assigned → `"Rowan / COMPANION / assigned to Aurora's party"`
    - hired + unassigned → `"Rowan / UNASSIGNED / assign to Aurora's party"`
- `bCompanionAssigned` is **persisted** (`heartland_companion_assigned` in the Soul.Campaign save; VERIFIED in `CaptureRBSaveDomain_Implementation` / restore path).

So **ArmyCompanion already exists in V0** as a boolean assignment of a second hero to the player's party. There is currently **no** independent-army or governor concept for a player hero, and **no** `FSoulHeroState` created for Rowan — `bSecondHeroHired`/`bCompanionAssigned` are plain bools, not a full hero with level/skills.

> **Contradiction against a likely assumption:** one might assume the "companion" is already a full progressing hero. It is **not** — Rowan has no `FSoulHeroState`, no XP, no skills. Only Aurora (`Hero`) and the DwarfCommander are real `FSoulHeroState`s, and only Aurora currently earns XP.

## 2. Role model (DESIGN PROPOSAL)

Three assignment roles. Keep V0 identical to the shipping boolean; layer V1/V2 additively.

### ArmyCompanion — **V0 (exists)**
- Hero travels/fights inside an army. Contributes `command`/`martial`/`magic` skill effects to that army's RBCombat.
- Backing state today: `bCompanionAssigned` bool. **PROPOSAL:** to let the companion *progress*, promote Rowan to a real `FSoulHeroState` (see §4) so it can hold level/skills; keep the existing bool as the "assigned to Aurora's party" flag.
- Wound/capture: a companion inside the army is wounded/captured by the same `ApplyBattleInjury` call path as the player hero when the army is wiped. (VERIFIED injury rule is per-hero; a companion hero would need its own `ApplyBattleInjury` call — see dependencies.)

### IndependentCommander — **V1 (proposal)**
- Hero leads a *separate* small army and can act on the strategic map on its own.
- Requires an army-ownership binding for player heroes that **does not exist today** (the player's army is a single `TMap<FName,int32> PlayerArmy` tied to `PlayerRegion`, not to a hero). This is the biggest new plumbing item and is called out as an unknown/dependency.
- Uses existing `FSoulHeroRules::CanCommandSiege` / `IsAvailable` gates unchanged.

### Governor — **V2 (proposal)**
- Hero assigned to a settlement region; grants a passive stewardship bonus (`stewardship.*`).
- **MUST** apply through the existing Soul.Settlements authority (`USoulSettlementStateSubsystem` / `FSoulSettlementRules`) — e.g. a small per-day gold add mirroring the VERIFIED Heartland market bonus in `AdvanceDay` (`+100 gold` when `human.market` L≥2 operational), or a small build-days reduction. **Never a second economy authority** (AGENTS.md).
- A Governor hero is out of the field: design choice whether its skill effects persist while Wounded (recommended yes, since it is physically at the settlement).

## 3. Role transitions (DESIGN PROPOSAL)
- Transitions cost nothing but take effect at the next `AdvanceDay` boundary (deterministic; no mid-day re-assignment exploits).
- A Captured hero (VERIFIED: `Condition==Captured`, does not auto-recover) cannot be assigned any role until released — this is **Lane 4's** prisoner lifecycle. Lane 3 only reads `IsAvailable`.
- Only one hero may be Governor of a given region at a time (one-to-one), enforced by the settlement authority side.

## 4. Minimal state to make companions progress (DESIGN PROPOSAL)
- Add `FSoulHeroState Companion;` (Rowan) to the subsystem, created in `HireTavernHero()` with `HeroId=TEXT("rowan")`, mirroring how `Hero` and `DwarfCommander` are constructed.
- Serialize it the same way `DwarfCommander` is already serialized under `heartland_dwarf_commander` — i.e. an **optional** nested object `heartland_companion` with `level/xp/points/mana/max_mana/condition/...`. Optional-with-default keeps schema at **v1** (VERIFIED tolerance pattern in `SoulCampaignManaTests` legacy test).
- Companion earns XP from the **same** battle-win hook as Aurora, but only when `bCompanionAssigned` (so an unassigned tavern companion does not farm XP). Deterministic, one-shot per encounter via `ResolvedEncounters`.

## 5. Explicit unknowns
- **LOCAL RUNTIME/ASSET CHECK REQUIRED:** whether RBCombat can present a second player-side hero actor (companion) in the arena, or whether V0 companion contribution must be modeled as a pre-battle army buff only. (Arena currently exposes `PlayerHero`, `bPlayerHero`, `bNonPlayerHero` — see `SoulRealtimeBattleArena.h`; a second friendly hero actor is unverified.)
- **VERIFIED OWNED ASSET METADATA / LOCAL CHECK:** owned character packs exist in Copperlight ("Creative Characters FREE", "Fantasy FREE - Low Poly 3D Models") but no specific companion/portrait asset binding is proven; any companion visual is **LOCAL RUNTIME/ASSET CHECK REQUIRED** before claiming a `/Game` path.
