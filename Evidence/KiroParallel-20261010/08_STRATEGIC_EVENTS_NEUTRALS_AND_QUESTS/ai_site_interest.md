# AI Site Interest — Lane 8

**Authority label:** DESIGN PROPOSAL (built on VERIFIED `SoulStrategyAI`)
**Source snapshot:** `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c`

---

## 1. What the AI knows today (VERIFIED FROM CURRENT SNAPSHOT)

`Source/SoulCore/Public/SoulStrategyAI.h`:

- `FSoulStrategyAI::Choose(const FSoulStrategySnapshot&) -> FSoulStrategyDecision` is a **pure, deterministic scorer**. No RNG, no hidden state.
- Action set: `ESoulStrategyAction { Hold, Recover, DefendOwnedRegion, CaptureResourceRegion, AttackVisibleArmy, SeizeExposedRegion, SiegeSettlement }`.
- Per-region input: `FSoulStrategicRegion { Id, OwnerFactionId, Threat, Value, ResourceValue, Feasibility=650, TravelCost=40, GarrisonStrength, bAdjacent, bOccupiedByPlayer, bExposed, bSettlement }`.
- **Crucial gap:** the AI has **no awareness of Heartland sites or any activity layer**. `CaptureResourceRegion` keys off `ResourceValue`, an integer, with no concept of a site, den, caravan, or quest.
- In the shipped founder/alpha/six-faction campaigns, `AdvanceEnemyAI()` (SoulFounderPlaytestStateSubsystem.cpp:658) is effectively **inert** ("garrisons hold; no synthetic strategic army bypasses encounter resolution"; six-faction says strategic AI is OFF). `FSoulStrategyAI::Choose` is exercised mainly in tests.

**Consequence:** the cleanest, lowest-risk way to give the activity layer AI relevance is to **feed site/event value into the existing `FSoulStrategicRegion.ResourceValue` and `Threat` fields** when the strategy snapshot is built — NOT to invent new actions or a second AI. A single new action is optional and justified only for one case (below).

---

## 2. Interest model (DESIGN PROPOSAL)

Each activity carries `ai_interest.interest_class` and `ai_interest.resource_value_bonus` (see `strategic_activity_schema.json`). When a faction's strategy snapshot is assembled, add the bonus to the hosting region's `ResourceValue`, and for threat-raising events add to `Threat`:

| interest_class       | snapshot effect | which AI action it feeds |
|----------------------|-----------------|--------------------------|
| `ignore`             | +0 | none (flavour/XP sites the AI needn't contest) |
| `opportunistic`      | `ResourceValue += bonus` when `bAdjacent` only | `CaptureResourceRegion` / `SeizeExposedRegion` |
| `contested_priority` | `ResourceValue += bonus` even at range (within TravelCost budget) | `CaptureResourceRegion` |
| `defend_if_owned`    | if `bOccupiedByPlayer==false && OwnerFactionId==self`: `Value += bonus` | `DefendOwnedRegion` |

### Bonus mapping (BALANCE_PENDING, Lane 9 owns final numbers)

| activity_kind     | default interest_class | resource_value_bonus | reason |
|-------------------|------------------------|----------------------|--------|
| resource_node     | defend_if_owned        | 50–70  | recurring income; AI should hold, rarely raid |
| crossing          | defend_if_owned        | 50     | strategic chokepoint income |
| mercenary_camp    | defend_if_owned        | 65     | recruitment value tied to ownership |
| monster_den       | contested_priority     | 110    | clearing unlocks a value site -> both sides want it |
| ruins             | opportunistic          | 90     | one-shot gold; worth a detour if adjacent |
| watchtower        | opportunistic          | 60     | vision value |
| cave              | defend_if_owned        | 70     | one-shot, inside owned territory |
| caravan (event)   | opportunistic          | 70     | time-limited intercept opportunity |
| resource_convoy   | contested_priority     | 90     | denying enemy income is high-value in war |
| shrine            | ignore / opportunistic | 20–40  | mana value only matters to magic-leaning factions |
| artifact_cache    | — BLOCKED —            | —      | no item authority |

---

## 3. Threat events (DESIGN PROPOSAL)

Events that call `raise_threat` (e.g. `event.war.border_raid`, `event.neutral.den_stirs`) add to `FSoulStrategicRegion.Threat`. This naturally:

- raises `DefendOwnedRegion` score for the threatened owner;
- suppresses expansion near an uncleared `monster_den` (an AI will not walk a weak stack past a den the design flags as dangerous).

No new action is needed; `Threat` already participates in the scorer.

---

## 4. The one justified new action (OPTIONAL, DESIGN PROPOSAL)

`ESoulStrategyAction::ClearNeutralSite` — only if the AI should actively **clear monster dens / neutral garrisons** rather than merely value the region. Justification: `CaptureResourceRegion` assumes the region can be captured by moving in; a `contested` activity requires winning a battle vs a **neutral** occupant, which is semantically distinct from capturing a rival faction's region.

- **Smallest-risk alternative (preferred for V0):** do NOT add an action. Model the den as a `GarrisonStrength` on a neutral region so the existing `CaptureResourceRegion`/`SeizeExposedRegion` path handles it via the battle bridge. Add the new action only if playtest shows the AI never attempts neutral clears.
- If added, place it adjacent to `CaptureResourceRegion` in the enum and give it an `Evaluate` branch mirroring `CaptureResourceRegion` but reading the activity's `occupant_strength_permille`.

---

## 5. Faction-specific interest (DESIGN PROPOSAL)

Keep Nature/Dark passivity compatible:

- **Nature / Dark:** `interest_class` is down-weighted (treat `opportunistic`→`ignore` for ruins/caravans) so they don't become aggressive map-grabbers. Only `defend_if_owned` remains, matching current low-aggression modelling. (VERIFIED: `dark` has no admitted strategic field unit — `FSoulCampaignRules::AdmittedStrategicUnit` returns `NAME_None` for dark; nature returns `nature_bear_warrior`.)
- **Orcs / Vikings:** up-weight `monster_den` and `resource_convoy` (raider doctrine). Dwarves up-weight `cave`/`resource_node` (industry).
- Encoded as an optional per-faction multiplier table (BALANCE_PENDING); default multiplier 1.0 keeps behaviour identical to today if the table is absent.

---

## 6. Determinism & anti-degeneracy

- All interest math is integer and added to an already-deterministic snapshot; `FSoulStrategyAI::Choose` stays pure. Equal scores break ties by stable region id (AGENTS.md).
- The AI never spawns synthetic armies for this (preserves the current "garrisons hold; no synthetic strategic army bypasses encounter resolution" guarantee). Site interest only re-weights **existing** candidate actions.
- No recruit/suicide/recapture loop is introduced because interest only nudges `ResourceValue`/`Threat`; feasibility and travel-cost gates already present in the scorer still apply.

---

## 7. Integration anchors (targets are VERIFIED)

- Snapshot assembly: wherever `FSoulStrategySnapshot.Regions` is built for a faction (today primarily in tests and the dormant `AdvanceEnemyAI`), add the activity `resource_value_bonus` / event `Threat` deltas before calling `Choose`.
- Data source: `ai_interest` block on each activity in `heartland_activity_expansion.json`; event deltas in `event_pool_v0.json` effects (`raise_threat`).
- No change to `SoulStrategyAI.h/.cpp` is required for V0 (data-fed). The optional `ClearNeutralSite` action is the only code change, and only if playtest demands it.

## 8. Explicit unknowns

- `AdvanceEnemyAI` is inert in shipped campaigns, so AI site contest is **untestable in-game today** without first activating a strategic AI turn (out of Lane 8 scope; coordinate with Lane 2).
- Faction interest multipliers are BALANCE_PENDING (Lane 9 / Lane 2).
- Whether neutral-garrison battles can be raised against a `NAME_None` occupant through the existing bridge is a **LOCAL RUNTIME-ASSET CHECK REQUIRED** (the bridge's roster source for a neutral side is unverified).
