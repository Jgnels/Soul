# Siege V1 Feature Order — Ladders, Engines, Repair, Defender Depth

Lane 7 of the 10-lane Kiro parallel pack. **Research/design only.** No runtime `Source/`, `Config/`,
`Content/`, `.uproject`, map or save edits are made by this lane. Every item below is a plan for the
main Unreal integration lane (Codex) to execute after founder feedback on Human Capital Siege V0.

Source snapshot authority: `Jgnels/Soul` @ `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c`
(branch `handoff/soul-kiro-20261010`). Asset metadata: `Jgnels/Copperlight-Asset-Catalog` @ `main`.

Evidence labels used throughout:
- **VCS** = VERIFIED FROM CURRENT SNAPSHOT (read directly from the frozen Soul tree).
- **VOA** = VERIFIED OWNED ASSET METADATA (read directly from the Copperlight catalog).
- **LRC** = LOCAL RUNTIME/ASSET CHECK REQUIRED (needs an Unreal editor/cooked check on Jeff's machine).
- **DP**  = DESIGN PROPOSAL (new design this lane recommends).

---

## 0. What V0 already is (do not reopen)

**VCS.** Human Capital Siege V0 is qualified and must not change. Its shape:

- One encounter: opt-in assault at the authored Human Capital, gated behind `-SoulSiegeV0` and a
  fortified (`FortificationLevel>0`) hostile capital with an eligible (non-wounded/non-captured)
  commander. `Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp:944-957` (`ApplySettlementEnvironment`).
- One blocker: the single native portcullis `SM_Portcullis` at the authored gate, lowered to its
  measured floor at runtime, melee-only damage, hidden+collision-disabled at zero integrity.
  `Source/SoulRealtimeBattle/Private/SoulHumanCapitalSiege.cpp` (`SetupSiege`, `CommitGateMelee`,
  `CommitSiegeHit`, `OpenSiegeGate`).
- One victory beyond rout: a single physical courtyard capture ring 9.5 m behind the gate,
  15 s uncontested occupancy, contested by a living defender.
  `Source/SoulCore/Private/SoulSiege.cpp` (`AdvanceCourtyard`), `TickSiege`.
- Rules core: `FSoulSiegeRules` / `FSoulSiegeState` / `FSoulSiegePreparation` in
  `Source/SoulCore/Public/SoulSiege.h` + `.cpp`.
- Aftermath into existing settlement domain: gate loss -> wall integrity + optional scar via
  `FSoulSiegeAftermathRules::Apply` (`Source/SoulCore/Private/SoulSiegeAftermath.cpp`) and
  `SoulFounderPlaytestStateSubsystem.cpp:555-562`.

**VCS — V0 explicitly deferred (`Evidence/HumanCapitalSiege-20261010/HANDOFF.md`, "Known limits"):**
> one gate/one objective; crowded and slow doorway; simple gate disappearance; no ladders/engines/
> wall-top combat; Human tactical defending siege not independently qualified; no new gate repair or
> captive rescue/ransom; no mid-battle save/resume.

That list is the exact scope this lane plans forward. The V0 "Next action" is also explicit:
> Improve this one assault based on [founder] feedback before adding ladders, engines or another city.

**Therefore the ordering principle below puts "make the one existing assault better" strictly
before any new attack vector.** Ladders/engines are valuable but secondary to fixing the congested
doorway that V0 itself flagged.

---

## Guiding constraints (all VCS unless noted)

1. **Do not treat meshes as functionality.** A climbable ladder is nav + collision + socket +
   animation + an accepted-contact traversal rule, not a static mesh. Every mesh-bearing item below
   carries a `local_validation_checklist.md` entry. (Lane instruction.)
2. **No second combat/save/diplomacy authority.** All new state extends `FSoulSiegeState` /
   `FSoulSettlementState` and rides the existing `USoulCampaignBattleBridge` descriptor/result and
   the existing `Soul.Settlements` / campaign save domains. (Shared rules + VCS of bridge in
   `Source/SoulCore/Public/SoulCampaignBattleBridge.h`.)
3. **`FSoulSiegePreparation` already exists but is unused at runtime.** `SoulSiege.h` declares
   `bLadders / bBatteringRam / bSiegeTower / bWallBreach / bDefenderBarricades / bReinforcedGate /
   bAmmoStores / bMagicalWard`. **VCS:** the arena begins siege with a *default* preparation —
   `SiegeState=FSoulSiegeRules::Begin({})` at `SoulHumanCapitalSiege`/arena `BeginPlay`
   (`SoulRealtimeBattleArena.cpp:1264`). Only `GateMaximum`/`GateIntegrity` are then overwritten from
   the descriptor. So the preparation struct is a latent, test-covered hook with **no transport and
   no gameplay effect yet.** V1 should light it up rather than invent a parallel struct.
4. **Objectives must be physical; no mid-battle construction; no pocket ladders.**
   (`Data/city_siege_blueprints.json` `rules`.) Ladders/engines must be *deployed objects that
   physically exist and are physically traversed/operated*, not instant UI toggles.
5. **Local Unreal evidence outranks cloud guesses.** The authored map `L_HumanCapital_Authored`
   (`/Game/Soul/Maps/...`) is Soul-owned content not present in this sandbox; wall-top nav, socket
   availability and climbable surfaces are all LRC.

---

## Phase ordering

Each phase is independently shippable and independently qualifiable, mirroring the V0 discipline
(one assault, qualified, then the next). Phases are ordered by **(a) honoring the V0 "fix the
doorway first" directive, (b) dependency, (c) risk, (d) founder-visible value.**

### Phase A — Doorway throughput & readability (no new vector) — HIGHEST PRIORITY
**Why first:** V0's own handoff names "crowded and slow doorway" as the top limit and makes founder
feedback on exactly this the gate to everything else. Ladders/engines are pointless if the single
breach is a traffic jam.

- A1. **Multi-lane gate traversal tuning (DP, no new mesh).** Widen the measured aperture waypoint
  logic in `SiegeDriveGroup`/`TickSiege` so a breached gate admits a 2–3 wide column, not a single
  file. Pure tuning of existing `SiegeForward`/`SiegeSide` offsets and the bounded RVO window.
  *Depends on nothing. No asset. LRC: throughput measured in cooked runtime.*
- A2. **Capture-ring and breach-state readability (DP).** Clearer on-field notice + camera framing
  for "gate open, now take the ring"; reuse `PushBattleNotice`. *No asset.*
- A3. **Defender inner-line behavior polish (DP).** V0 defenders only `Hold` behind the gate
  (`SiegeDriveGroup`, Side==1 closed-gate branch). Make the contest at the ring deliberate rather
  than incidental. *Prereq for Phase D defender depth.*

**Exit test:** founder can run the same assault and the doorway reads as a fight, not a queue.

### Phase B — Preparation transport + wall-breach as a second vector
**Why second:** this is the smallest new *vector* and reuses the already-tested preparation struct
and the already-existing `OpenBreach`/`WallBreaches` + `ESoulSiegeLayer::Walls` path.

- B1. **Transport `FSoulSiegePreparation` through the bridge (DP).** Add preparation fields to
  `FSoulCampaignBattleDescriptor` and pass to `FSoulSiegeRules::Begin(Prep)` instead of `{}`.
  Campaign decides prep from siege-preparation spend (Phase F). *This is the keystone enabler — see
  `siege_engine_requirements.json`.*
- B2. **Wall breach point (DP + LRC).** A second physical opening in a wall segment, distinct from
  the gate, that opens when `bWallBreach` prep is present or an engine produces a breach. Reuses
  `ASoulFortificationSegmentActor` Intact/Damaged/Breached branches (VCS: that actor already models
  breach visuals but **has no in-battle damage caller** — only the town-view presentation controller
  drives it). *LRC: a breachable wall segment with authored nav behind it in `L_HumanCapital_Authored`.*

**Exit test:** a prepared attacker can enter through a wall breach as an alternative to the gate;
`CanAssaultWalls` already returns true for `bWallBreach` (VCS, `SoulSiege.cpp`).

### Phase C — Ladders (first true multi-approach vector)
**Why third:** ladders are the canonical "second approach" and the owned **Free Ladder Animation
Set** (VOA) exists, but ladders are the highest *animation/nav/socket* risk and must not precede the
preparation transport (B1) they depend on.

- C1. **Ladder deployment object (DP).** A placeable ladder actor that attaches to an authored wall
  edge and exposes a climb volume. Driven by `bLadders` prep.
- C2. **Ladder climb traversal (DP + LRC).** Accepted-contact climb that moves a unit from ground to
  a wall-top nav island. **The entire value depends on an authored wall-top the AI/player can stand
  and fight on** — LRC.
- C3. **Wall-top melee + shove-off (DP).** Battlement combat once up. Depends on Phase D defender
  positions existing.

See `ladder_requirements.json` for the full gate list. **Ladders are explicitly the point where
"mesh != functionality" bites hardest.**

### Phase D — Defender depth (wall positions, ranged defense, battlement combat)
**Why fourth:** meaningful ladders/towers need defenders on the wall to climb *into*. Reuses the
existing `Ranged` role (bow reach 2600, VCS `SoulRealtimeBattleArena.cpp:398`) which V0 forbids from
touching the gate (`CommitSiegeHit` rejects `bRanged`).

- D1. **Defender wall-position anchors (DP + LRC).** Authored stand points on the battlement; wire
  Side==1 formations to garrison them instead of only holding behind the gate.
- D2. **Ranged wall defense (DP).** Let defender archers fire down the approach using the existing
  projectile/ranged component. Attacker ladders/towers become the counter.
- D3. **Battlement melee resolve (DP).** Clearing a wall section becomes a mini-objective feeding the
  existing `AdvanceLayer` (`OuterField->Walls->InnerSettlement->Keep`), which V0 defines but only the
  gate path currently drives.

### Phase E — Siege engines (tower, ram, catapult/trebuchet)
**Why fifth:** highest cost, highest asset risk, and only worthwhile once doorway + ladders +
defenders make a city hard enough that an engine is the answer. Feasibility differs sharply per
engine — see `siege_engine_requirements.json`.

- E1. **Battering ram (DP).** Reuses the gate-damage path (`ApplyGateDamage`) but as a multi-crew
  operated object with far higher accepted damage per contact; `bBatteringRam` prep.
  *Lowest-risk engine: it only needs to drive the existing gate integrity down faster.*
- E2. **Siege tower (DP + LRC).** A mobile high platform delivering troops to the wall-top. Highest
  nav/collision/animation risk of any item; depends on Phase C/D wall-top being real. **Likely the
  last thing to qualify.** `bSiegeTower` prep.
- E3. **Catapult / mangonel / trebuchet (DP + VOA + LRC).** Owned **Medieval Mangonel – Catapult
  Siege Weapon** (3DreaMax, VOA) exists but is `LocallyAvailable:false` / not imported. Ranged wall
  or gate damage from the outer field; produces breaches feeding Phase B/D. *Feasibility caveat:
  projectile arc + wall-segment damage model is new; recommend mangonel as the single engine of this
  class, not trebuchet+catapult+ballista together.*

### Phase F — Siege preparation cost/time + AI siege preparation
**Why last to *finish*, but partly built in B1:** the campaign-side economy that decides which prep
booleans are set. Must exist for AI to "prepare" a siege rather than instantly possessing engines.

- F1. **Preparation cost/time model (DP).** Spend treasury/days to acquire ladders/ram/tower/engine
  before the assault; writes the preparation that B1 transports. See `siege_engine_requirements.json`
  and `ai_siege_plan.md`.
- F2. **AI siege preparation (DP).** Extend `FSoulStrategyAI` `SiegeSettlement` scoring (VCS
  `SoulStrategyAI.cpp:173-199`) so the AI accrues preparation over turns instead of the current
  single readiness≥600 / 70%-force-ratio snap decision. See `ai_siege_plan.md`.

### Phase G — Post-battle fortification repair
**Why can run in parallel after B/F:** repair is campaign-layer and does not need the battle vectors,
but it is only *meaningful* once walls/gates take persistent, varied damage (Phases B–E). Repair
mechanics have the cleanest existing hooks of any V1 feature. See `fortification_repair_spec.md`.

---

## Dependency summary (compressed; full matrix in HANDOFF.md)

```
A (doorway)        -> prerequisite mindset for everything; no code deps
B1 (prep transport)-> enables B2, C, E (keystone)
B2 (wall breach)   -> needs B1 + authored breachable wall (LRC)
C  (ladders)       -> needs B1 + D1 wall-top (LRC) + owned ladder anim (VOA)
D  (defenders)     -> needs A3; enables C3, E2
E1 (ram)           -> needs B1; reuses ApplyGateDamage (lowest risk engine)
E2 (tower)         -> needs C/D wall-top; highest risk
E3 (catapult)      -> needs B1 + wall-segment damage model + mangonel import (VOA/LRC)
F  (prep economy)  -> needs B1; feeds campaign + AI
G  (repair)        -> needs B/E persistent damage; cleanest existing hooks
```

## Recommended minimum "V1 that founders will feel"

If scope must be cut, ship **A + B + G + the ram half of E (E1) + F1**. That delivers: a readable
multi-lane doorway, a real second entry (wall breach / ram), a reason to prepare (cost/time), and
visible post-siege repair/scars — all on *existing* rules primitives with the *lowest* new-asset and
new-nav risk. Ladders (C), full defender depth (D), tower (E2) and catapult (E3) are the "V1.5"
tier that each need a dedicated local qualification pass like V0 got.
