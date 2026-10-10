# LANE 7 HANDOFF — Siege V1 Research: Ladders, Engines, Repair, Defender Depth

**Lane:** `07_SIEGE_V1_LADDERS_ENGINES_REPAIR`
**Branch:** `kiro/siege-v1-ladders-engines-repair-20261010`
**Frozen source:** `Jgnels/Soul` @ `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c` (branch `handoff/soul-kiro-20261010`) — verified exact.
**Asset metadata source:** `Jgnels/Copperlight-Asset-Catalog` @ `main`.
**Mode:** research/design only. **No** runtime `Source/`, `Config/`, `Content/`, `.uproject`, map or save edits. All lane output is under `Evidence/KiroParallel-20261010/07_SIEGE_V1_LADDERS_ENGINES_REPAIR/`.

Evidence labels used everywhere: **VCS** verified-from-current-snapshot · **VOA** verified-owned-asset-metadata · **LRC** local-runtime/asset-check-required · **DP** design-proposal.

---

## Artifacts in this directory

| File | Purpose |
|---|---|
| `siege_v1_feature_order.md` | The phased V1 build order (A doorway → B prep/breach → C ladders → D defenders → E engines → F prep-economy/AI → G repair), with the minimum "V1 founders will feel" cut. |
| `ladder_requirements.json` | Machine-readable ladder gate list (deploy actor, wall-top nav, landing socket, climb traversal, animation, defender counter, prep wiring). |
| `siege_engine_requirements.json` | Ram / catapult-mangonel / wall-segment-damage / siege-tower feasibility, ordered by risk, plus the keystone preparation-transport enabler (B1). |
| `fortification_repair_spec.md` | Costed/timed repair design on the EXISTING damage/repair primitives; fixes the dead `RepairRoot` scaffolding branch and permanent-scar lifecycle. |
| `ai_siege_plan.md` | Deterministic AI siege *preparation* accrual + approach selection + anti-degeneracy, extending `FSoulStrategyAI` without a second authority. |
| `local_validation_checklist.md` | The "mesh != functionality" enforcement: every nav/collision/socket/animation/perf check per feature, with the single highest-risk blocker called out. |
| `acceptance_tests.md` | Test specs per phase in the existing UE-automation style, with a V0 regression lock run on every phase. |
| `verified_references.json` | Exact current file/symbol/line references for every claim. |
| `owned_asset_metadata.json` | VOA ownership + local-presence for siege-relevant assets, with the unresolved gate-donor flag. |

---

## Current state of truth (VCS) — the base V1 builds on

**Human Capital Siege V0 is qualified and frozen.** It is exactly one assault:
one native portcullis (`/Game/CastleTown/.../SM_Portcullis`) lowered to a measured floor, damaged by
**melee only** (`CommitSiegeHit` rejects ranged), hidden at zero integrity, then a single physical
courtyard capture ring (15 s uncontested, living defender contests). Aftermath flows into the existing
`Soul.Settlements` + campaign save domains as wall-integrity loss + an optional `human.gate.breached`
scar. Fortification level gates admission (`FortificationLevel>0`; level≥2 ⇒ gate max 1300). Wounded/
captured commanders reject before mutation. F5/F9 and fresh-process restore are exact.
(Files: `SoulSiege.{h,cpp}`, `SoulHumanCapitalSiege.cpp`, `SoulCampaignBattleBridge.h`,
`SoulFounderPlaytestStateSubsystem.cpp`, `SoulSiegeAftermath.cpp`, `SoulSettlement.cpp`,
`SoulStrategyAI.cpp`; tests `SoulGateAssaultTests.cpp`, `SoulSiegeCampaignTests.cpp`,
`SoulHumanCapitalSiegeQualification.cpp`; data `city_siege_blueprints.json`, `EnvironmentRegistry.json`;
V0 handoff `Evidence/HumanCapitalSiege-20261010/HANDOFF.md`.)

**The V0 handoff itself names the V1 scope** (known limits): *no ladders/engines/wall-top combat;
crowded, slow doorway; one gate/one objective; no gate repair; no rescue/ransom; no mid-battle save.*
Its "Next action" is explicit: **improve this one assault on founder feedback before adding ladders,
engines or another city.** This lane's ordering honors that.

---

## Key conclusions

1. **The biggest V1 lever is already in the code and switched off.** `FSoulSiegePreparation`
   (ladders/ram/tower/breach/reinforced-gate/ammo/ward/barricades) is fully declared and `CanAssaultWalls`
   already honors it — but the arena begins every siege with `FSoulSiegeRules::Begin({})`
   (empty prep; `SoulRealtimeBattleArena.cpp:1264`) and the campaign descriptor
   (`FSoulCampaignBattleDescriptor`) carries **no preparation**. **Transporting preparation (feature
   B1) is the single keystone that unlocks ladders AND every engine with one clean seam and no new
   authority.** Do B1 first among the "new vector" work.

2. **Fix the doorway before adding vectors.** V0's own top limit is the congested single-file breach.
   Phase A (multi-lane aperture tuning, capture-ring readability, defender inner-line) uses NO new
   assets and is the prerequisite mindset for everything. Shipping ladders on top of a traffic jam
   wastes the ladders.

3. **Ladders are the sharpest "mesh != functionality" trap, and they hinge on one unknown.** The
   owned *Free Ladder Animation Set* (VOA) does not make a ladder work. Ladders need a **capsule-walkable
   battlement in `L_HumanCapital_Authored`** (checklist `C-NAV-1`, BLOCKING). That surface is UNPROVEN
   from source — V0 only proved the ground approach + courtyard. **Run `C-NAV-1` in-editor before any
   ladder/tower/defender engineering.** If it fails, V1 collapses to the ground-vector subset
   (doorway + ram + catapult-to-gate + repair + prep/AI) until a battlement is authored.

4. **Engines rank cleanly by risk:** battering ram (reuses the qualified `ApplyGateDamage` path →
   lowest risk, ship first) → catapult/mangonel (owned VOA, but needs a NEW projectile + in-battle
   wall-segment damage model, ONE engine of this class only) → siege tower (highest nav/collision/
   animation risk, depends on the same wall-top as ladders → qualify last).

5. **Repair is the cleanest feature to land** because the primitives already exist
   (`DamageWalls/RepairWalls/DamageBuilding/RepairBuilding`, `WallIntegrityPermille`, `PermanentScars`,
   `ASoulFortificationSegmentActor` with a `RepairRoot` scaffolding branch). What is missing is a
   costed/timed repair job (`AdvanceDay` ignores repair today), a `Repairing` building condition, and
   wiring the **dead** scaffolding branch + a scar-heal lifecycle.

6. **AI "besieges" but never "prepares."** `FSoulStrategyAI::SiegeSettlement` is a deterministic snap
   decision (readiness≥600, ≥70% force ratio). It never acquires ladders/engines and the resulting
   battle runs with empty preparation. V1 adds a deterministic `PrepareSiege` accrual action that
   feeds the transported preparation, with flyer-bypass preserved and memory-bias anti-churn reused.

---

## Contradictions against current assumptions (flag for Astra/Codex)

- **C1 — "Preparation affects the siege."** *Assumed, false today.* `FSoulSiegePreparation` is declared
  and test-adjacent, implying it drives sieges. **VCS:** it is never transported; sieges always run
  with `{}`. Any design that assumes "the AI brought ladders" is currently fiction. (B1 fixes it.)
- **C2 — "RepairWalls / RepairBuilding are the repair system."** *Assumed costed/gradual/scar-aware
  per `city_siege_blueprints.json` + plan doc; actually instant, free, scar-blind stubs.* `AdvanceDay`
  does no repair at all. The design intent and the code disagree; the code is a stub.
- **C3 — "Walls show damage/repair states."** *Partly false.* The town-view presentation DOES show
  intact/damaged/breached, but `ApplyWallState` is always called with `bRepairing=false`, so the
  `RepairRoot`/`RepairActors` scaffolding branch and the `Repairing` building condition are **dead** —
  repair visuals are specified but never rendered.
- **C4 — "A breach is physical and persists."** *True at the gate only.* In-battle wall damage exists
  ONLY for the single portcullis; there is no in-battle wall-SEGMENT integrity. Catapult/wall-breach
  vectors require a new segment-damage model (`ENGINE-WALLSEG`).
- **C5 — "Scars may remain after repair."** *Currently "scars ALWAYS remain forever."* `PermanentScars`
  is never cleared; there is no heal lifecycle. "May remain" needs the active-vs-historical scar split.
- **C6 — "Multiple approaches exist."** *Data says yes, map proves no.* `city_siege_blueprints.json`
  lists `main_road/side_wall/flying` for the capital, but only the main gate approach + courtyard is
  nav-proven (VCS). Side-wall/flying approaches are unauthored/unproven (LRC). AI approach-selection
  cannot be qualified until a second physical approach exists.
- **C7 — "Owning the ladder/mangonel assets means we can build the feature."** *False.* Both are
  `LocallyAvailable:false`/`imported:false` (VOA), licenses UNKNOWN, and the gate-donor kit
  (`/Game/CastleTown`) is an unresolved local import, not a confirmed Fab product. Ownership ≠ working
  feature; the gameplay RULE must pass independent of asset polish.

---

## Implementation order (compressed; full in `siege_v1_feature_order.md`)

```
A  Doorway throughput + readability         (no asset; no code deps)            [SHIP 1]
B1 Transport FSoulSiegePreparation           (keystone; enables C,E,F)          [SHIP 2]
B2 Wall-breach vector + ENGINE-WALLSEG       (needs B1 + authored breach wall)  [SHIP 3]
G  Costed/timed repair + scaffolding + scars (cleanest hooks; after B/E damage) [SHIP 4]
E1 Battering ram                             (reuses ApplyGateDamage)           [SHIP 5]
F1 Siege preparation cost/time economy       (needs B1; feeds AI)               [SHIP 6]
F2 AI PrepareSiege accrual + approach        (needs B1/F1)                      [SHIP 7]
--- gated on C-NAV-1 (walkable battlement) passing in-editor ---
D  Defender wall positions + ranged + melee  (needs A3; enables C3,E2)          [V1.5]
C  Ladders                                   (needs B1 + wall-top + anim)       [V1.5]
E3 Catapult/mangonel (gate first, then seg)  (needs B1 + ENGINE-WALLSEG)        [V1.5]
E2 Siege tower                               (needs C/D wall-top; highest risk) [V1.5 last]
```

**Minimum "V1 founders will feel" if scope is cut:** `A + B + G + E1 + F1` — all on existing rules
primitives with the lowest new-asset and new-nav risk.

---

## Dependencies & conflicts with other lanes

- **Lane 2 (AI army composition/recruitment/recovery):** shares treasury/stock assumptions. The siege
  preparation economy (F1) and `PrepareSiege` action (F2) must draw from the SAME treasury/stock model
  Lane 2 defines. **Conflict risk:** two lanes adding strategic-AI actions to `FSoulStrategyAI`.
  Coordinate the `ESoulStrategyAction` additions so siege-prep and recruitment compose rather than
  collide; both must stay deterministic and additive.
- **Lane 6 (Dwarf Hold + Viking Harbour walkable settlements):** V1 siege features here target the
  Human Capital only. The preparation/repair/engine *rules* are faction-agnostic, but new siege MAPS
  for dwarf/viking are Lane 6's domain. `city_siege_blueprints.json` already specifies their layers;
  keep siege-engine rules reusable so Lane 6 cities inherit them.
- **Lane 1 (campaign victory/objectives):** siege captures feed site-control / military victory. The
  physical objectives (`gatehouse/forge/witch_collegium/keep`) in `city_siege_blueprints.json` and
  `ESoulSiegeObjective` are the shared surface; do not create a second objective authority.
- **Lane 8 (strategic events/neutral sites):** none direct; avoid double-owning region ownership flips.
- **No conflict** with save/diplomacy authorities as long as everything extends `FSoulSiegeState` /
  `FSoulSettlementState` and rides the existing bridge + save domains.

---

## Explicit unknowns (local Unreal evidence outranks these)

1. **BLOCKING — walkable battlement:** does `L_HumanCapital_Authored` have a capsule-walkable wall-top?
   (`C-NAV-1`.) Gates all wall-top features (ladders, tower, defender depth).
2. **Second approach:** is there any authored side-wall/flying approach with nav, or only the gate?
3. **Breachable wall segments:** do `ASoulFortificationSegmentActor` instances exist in the authored
   map with populated actor arrays and navigable interiors behind them?
4. **Asset readiness:** are the owned ladder animation / mangonel / ballista / spikes locally present
   and import/rig-compatible? (All `LocallyAvailable:false|null`, `imported:false`, license UNKNOWN.)
5. **Gate-donor identity:** which Fab product provides `/Game/CastleTown` (SM_Portcullis)? Unresolved
   in the catalog (local import root only). Do not invent ownership.
6. **Mobile-engine pathing:** can a ram/tower path the authored approach given the known bridge-deck
   gap flagged in `SiegeDeployment`?
7. **DP tuning numbers** (prep turns/costs, repair days/permille, ladder length/climb duration, engine
   damage) are starting proposals pending in-editor measurement and a campaign-sim pass.

---

## Highest-priority integration action for Astra

**Run the single in-editor check `C-NAV-1` first: is the Human Capital battlement capsule-walkable in
`L_HumanCapital_Authored`?** Then land the keystone **B1 — transport `FSoulSiegePreparation` through
`FSoulCampaignBattleDescriptor` into `FSoulSiegeRules::Begin(Prep)`** (replacing the `Begin({})` at
`SoulRealtimeBattleArena.cpp:1264`), regression-locked so empty preparation reproduces V0 exactly.
`C-NAV-1` decides whether V1 is "ground + wall-top" or "ground-only"; B1 is the one change that turns
the already-written, already-tested preparation struct into live gameplay and unblocks ladders and
every engine without creating a second authority.
