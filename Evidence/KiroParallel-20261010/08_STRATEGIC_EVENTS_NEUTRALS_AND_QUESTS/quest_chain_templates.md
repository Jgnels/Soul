# Quest Chain Templates — Lane 8

**Authority label:** DESIGN PROPOSAL (built only on VERIFIED existing systems)
**Source snapshot:** `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c`
**Conforms to:** `strategic_activity_schema.json`, `event_pool_v0.json`

---

## 0. Design stance

Soul has **no campaign quest framework today** (VERIFIED FROM CURRENT SNAPSHOT: `SoulObjectives.*` is tactical-battle-only — `ESoulObjectiveKind {EliminateEnemy, HoldHexes, CaptureHex, ReachExit, SurviveRounds, CaptureKeep}`). So quests here are deliberately **compact, data-light chains** that reuse existing authorities:

- **Progression state** = one integer per quest id in `quest_progress` (a new additive field in the existing `Soul.Campaign` RBSave blob — NOT a new save authority).
- **Steps** = references to existing verbs: interact with an activity site, clear a hostile occupant through the **existing battle bridge**, hold a region for N days, reach a region, or satisfy a diplomacy/memory condition.
- **No new quest UI authority** — a quest is a thin wrapper that lights up existing site-interaction buttons and reports step text through the existing campaign summary string (`BuildSummary()` pattern).

A quest step **never invents** combat, rewards, items, or regions. If a step needs something that has no owning authority (e.g. a relic item), the step is marked `BLOCKED` and the template says why.

---

## 1. Quest state contract

```
quest_progress : TMap<FName,int32>   // quest id -> current step index (0 = not started)
```

- Persisted inside `CaptureRBSaveDomain_Implementation` (SoulFounderPlaytestStateSubsystem.cpp ~717) and restored at ~843, exactly like `heartland_site_days`.
- Tolerant restore: a missing quest id = step 0 (not started), so pre-feature saves load unchanged.
- Deterministic: step completion is evaluated in `AdvanceDay()` and after each site interaction / battle resolution (`HandleBattleResolved`), using only VERIFIED state. No timers, no RNG inside quest advancement (RNG stays in the event pool only).

### Step kinds (all map to VERIFIED verbs)

| step_kind          | satisfied when… | existing authority |
|--------------------|-----------------|--------------------|
| `interact_site`    | `InteractActivitySite(site_id)` succeeds | `InteractHeartlandSite` (generalised) |
| `clear_occupant`   | battle vs the site's occupant resolves as a win | existing campaign battle bridge / `HandleBattleResolved` |
| `hold_region`      | `World.Regions[r].OwnerFactionId == PlayerFaction` for N consecutive Days | `FSoulWorldState` + `AdvanceDay` day counter |
| `reach_region`     | `PlayerRegion == r` | `MovePlayerTo` |
| `relation_at_least`| `HumanRelations[f].RelationPermille >= X` | `FSoulDiplomacyRules` |
| `reclaim_region`   | region re-captured after a `PlaceLoss` memory | `FSoulMemoryRules::PlaceReclaimBonus` |

### Reward kinds (reuse `reward_grammar` from the schema)
`gold` / `mana` are VERIFIED. `recruitment_pool`, `hero_xp`, `vision_reveal`, `diplomacy_relation` map to existing authorities. `relic_item` is **BLOCKED** (no item authority).

---

## 2. Template A — "Clear the March" (onboarding, Days 1–7)

**Purpose:** teach the player that the map rewards leaving the capital. 3 steps, all low-risk.

```
quest.clear_the_march
  step 1  reach_region      region=north_pass
  step 2  interact_site     site=activity.watchtower.north_pass   (clear_then_collect, low risk)
          reward            vision_reveal (adjacent neutral regions)
  step 3  interact_site     site=activity.ruins.northwest_march   (investigate, low risk)
          reward            hero_xp +1
  final reward              gold +120, relation +30 with nearest same-biome faction
```

- **Trigger:** auto-starts on first `AdvanceDay` when `bHeartlandEnabled` and Day>=1.
- **Why it works:** every step is a region the Heartland army can reach in a few days; it chains the two cheapest activities (watchtower, march ruins) into a guided tour. Declining is impossible to fail — there is no time limit on the onboarding arc.
- **Dependencies:** `activity.watchtower.north_pass`, `activity.ruins.northwest_march` from `heartland_activity_expansion.json`.

---

## 3. Template B — "The Den and the Timber" (early midgame, Days 8–20)

**Purpose:** couple the map layer to the **economy and recruitment** lanes.

```
quest.den_and_timber
  step 1  clear_occupant    site=activity.monster_den.forest_edge  (moderate risk, battle bridge)
          reward            recruitment_pool human_knight +2
  step 2  hold_region       region=forest_edge  N=3   (defend the newly opened timber land)
          event hook        event.neutral.den_stirs is suppressed once cleared
  final reward              gold +180, value timber site (forest_edge) marked economically active
```

- **Trigger:** becomes available Day>=4 once `activity.monster_den.forest_edge` is discoverable.
- **Why it works:** step 1 is a real battle (through the existing bridge, not a synthetic model); step 2 teaches that holding newly-won land matters. Ties into Lane 5 (settlement value sites) and Lane 2 (AI contest for the same den — see `ai_site_interest.md`).
- **Contradiction flagged:** `value_site_candidates.forest_edge.economy_resource_id` is `null` / `BALANCE_PENDING` in the snapshot, so the "economically active" final reward is a **DESIGN PROPOSAL** until Lane 9 assigns the resource id.

---

## 4. Template C — "Hold the Crossing" (midgame map-control, repeatable)

**Purpose:** reward map control over city-stacking; reinforces strategic sites.

```
quest.hold_the_crossing
  step 1  reach_region      region=river_ford
  step 2  hold_region       region=river_ford  N=5
          passive income    activity.crossing.river_ford toll (+40 gold/day while held)
  final reward              relation +20 with the faction whose road the ford serves,
                            raise_threat suppression (ford no longer a raid target while garrisoned)
```

- **Trigger:** Day>=1, available once the player can reach `river_ford`.
- **Why it works:** uses the **existing river-bridge battle geography** (VERIFIED authored content) and the toll activity. Repeatable: completing it and later losing the ford re-arms the quest, creating organic back-and-forth without a scripted sequence.
- **Interacts with:** `event.war.border_raid` (holding the ford reduces its raid threat).

---

## 5. Template D — "Rumours of the Deep" (optional one-shot, Days 6+)

**Purpose:** a self-contained treasure chain that demonstrates co-located activities.

```
quest.rumours_of_the_deep
  step 1  interact_site     site=activity.cave.old_quarry  (investigate, 2 AP, moderate risk)
          reward            gold +300
  final reward              commander memory (positive, PlaceId=old_quarry) -> small PlaceReclaimBonus later
```

- **Trigger:** Day>=6 and player owns `old_quarry`.
- **Why it works:** proves recurring `heartland.quarry` and one-shot `activity.cave.old_quarry` coexist on one region. One-shot: after completion the cave is `depleted` and persists so.
- **No BLOCKED steps** (reward is gold + memory, both VERIFIED).

---

## 6. Template E — "Caravan Season" (event-driven, repeatable)

**Purpose:** show quests can be *seeded by the event pool* rather than fixed.

```
quest.caravan_season  (meta-quest, counts completions)
  on event             event.trader.caravan_passing fires -> spawns activity.caravan.transient
  step (repeat)        escort/intercept the transient caravan within its window_days
          reward            gold +160 per success
  milestone            3 successful escorts -> relation +50 with a trader faction, title text only
```

- **Trigger:** passive; advances whenever a caravan event resolves in the player's favour.
- **Why it works:** turns a stream of transient events into a soft long-term goal without scripting individual caravans. Fully deterministic through the event seed.
- **Anti-spam:** bounded by `event.trader.caravan_passing` cooldown (4 days) and the per-day event cap.

---

## 7. Authoring order for Codex

1. Add `quest_progress` to the `Soul.Campaign` save blob (additive, tolerant).
2. Implement step evaluators for the six VERIFIED step kinds (pure functions over campaign state).
3. Wire quest advancement into `AdvanceDay()` (hold/day-based steps) and `HandleBattleResolved` (clear_occupant) and `InteractActivitySite` (interact_site).
4. Author Templates A and C first (no battle dependency beyond existing bridge, no BLOCKED steps) — smallest safe slice.
5. Author B, D, E once the activity sites and event pool land.

## 8. Explicit unknowns / BLOCKED

- **BLOCKED:** any `relic_item` reward — no item authority exists in the snapshot.
- **LOCAL RUNTIME-ASSET CHECK REQUIRED:** whether an event-choice modal exists in the founder HUD; current HUD exposes site-interaction buttons only. Quests that need a yes/no choice (refugees) depend on the Unreal integration lane adding one.
- **BALANCE_PENDING:** all reward amounts and hold-day counts (Lane 9).
- **DESIGN PROPOSAL:** "nearest same-biome faction" relation targeting pending Diplomacy V1 (Lane 4).
