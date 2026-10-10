# LANE 8 — Strategic Events, Neutral Sites, Quests & Map Activity — HANDOFF

**Lane:** `08_STRATEGIC_EVENTS_NEUTRALS_AND_QUESTS`
**Branch:** `kiro/strategic-events-neutrals-quests-20261010`
**Source snapshot (frozen, verified):** `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c` (`handoff/soul-kiro-20261010`)
**Scope boundary honoured:** writes ONLY under `Evidence/KiroParallel-20261010/08_STRATEGIC_EVENTS_NEUTRALS_AND_QUESTS/`. No runtime `Source/`, `Config/`, `Content/`, `.uproject`, maps or saves edited.

## Deliverables in this directory
| File | Purpose |
|------|---------|
| `strategic_activity_schema.json` | Machine-readable schema for the activity/site/event/quest data model, persistence contract, determinism contract, integration points. |
| `heartland_activity_expansion.json` | 7 concrete activity-site instances placed on **real** region ids (plus the 3 existing sites restated for backward-compat). |
| `event_pool_v0.json` | 6 deterministic seeded regional events (war/trader/refugee/neutral) with trigger & effect grammar. |
| `quest_chain_templates.md` | 5 compact quest-chain templates (A–E) built only on verified verbs. |
| `ai_site_interest.md` | How activities/events feed the existing `FSoulStrategyAI` without a second AI. |
| `owned_asset_candidates.json` | VERIFIED OWNED ASSET METADATA mapping to activity kinds (ownership only; import to Soul UNVERIFIED). |
| `acceptance_tests.md` | Deterministic test matrix (A–G) mirroring existing Soul test suites. |
| `HANDOFF.md` | This file. |

---

## 1. The single most important finding (VERIFIED FROM CURRENT SNAPSHOT)

**There is no generic strategic-site, event, or campaign-quest system in Soul.** The windmill/quarry/shrine are a thin, Heartland-scoped feature:

- Data model: `FSoulHeartlandSite { FName Id,Region,Effect; FString Name; int32 Amount,ActionCost; }` — `Source/Soul/Public/SoulHeartlandContent.h`.
- Loader: `FSoulHeartlandContent::Load` — `Source/Soul/Private/SoulHeartlandContent.cpp` (reads `Data/SettlementEnvironments/HeartlandDevelopment.json` `"sites"`; validates `Effect ∈ {gold,mana}`).
- Collection/control: `USoulFounderPlaytestStateSubsystem::InteractHeartlandSite` — `Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp:983`. "Control" = own the region AND stand a living company on it. Throttle = `TMap<FName,int32> HeartlandSiteDays` (header line 109), one collect per site per `Economy.Day`.
- `SoulHeartlandSites.{h,cpp}` is **presentation only** (its own comment: "no site authority").
- `SoulObjectives.*` is **tactical-battle only** (`ESoulObjectiveKind {EliminateEnemy,HoldHexes,CaptureHex,ReachExit,SurviveRounds,CaptureKeep}`) — NOT a campaign quest framework.
- There is **no event bus**; the only "something happened" campaign hook is `USoulCampaignBattleBridge::OnBattleResolved` → `HandleBattleResolved`.

**Therefore the correct architecture is to EXTEND this one pattern additively — not build a second system.** All six deliverables are designed around that constraint.

## 2. The exact authority to extend (VERIFIED anchors)

| Concern | File · symbol | Lane 8 action |
|---|---|---|
| Site data model | `Source/Soul/Public/SoulHeartlandContent.h` · `FSoulHeartlandSite` | Add a superset `FSoulActivitySite` (or extend) — see schema `activity_site_definition`; keep all existing fields so old data loads. |
| Site loader | `SoulHeartlandContent.cpp` · `FSoulHeartlandContent::Load` | Add an `"activities"` array reader (sibling loader `FSoulActivityContent::Load`), reuse `ReadData`. |
| Collection | `SoulFounderPlaytestStateSubsystem.cpp:983` · `InteractHeartlandSite` | Generalise → `InteractActivitySite(FName,FString&)`; `collect` = current path, `clear_then_collect` routes through the battle bridge first. |
| **Turn tick (event hook)** | `SoulFounderPlaytestStateSubsystem.cpp:630` · `AdvanceDay()` | Insert `EvaluateStrategicEvents(Rng)` immediately **after** `FSoulCampaignRules::AdvanceDay(Economy)` (line 643) and **before** `AdvanceEnemyAI()` (line 656). |
| Day increment | `SoulCampaign.cpp` · `FSoulCampaignRules::AdvanceDay` | No change; it owns `Economy.Day`. |
| **Save (persistence)** | `SoulFounderPlaytestStateSubsystem.cpp:717` capture / `:843` restore · domain `"Soul.Campaign"` schema 1 | Add `activity_site_days`, `activity_site_states`, `quest_progress`, `campaign_event_seed`, `fired_events_by_day` — additive, tolerant, mirroring the existing `heartland_site_days` pattern. **Do NOT register a new `IRBSaveDomainProvider`.** |
| World/region | `Source/SoulCore/Public/SoulWorld.h` · `FSoulRegionState`/`FSoulWorldState` | Read-only; sites attach by existing region id. No new per-region field required for V0. |
| AI | `Source/SoulCore/Public/SoulStrategyAI.h` · `FSoulStrategicRegion.ResourceValue`/`Threat`, `ESoulStrategyAction` | Data-fed: add `resource_value_bonus`/`raise_threat` deltas when the snapshot is assembled. Optional single new action `ClearNeutralSite` only if playtest demands. |
| Diplomacy | `Source/SoulCore/Public/SoulDiplomacy.h` · `FSoulDiplomacyRules::Apply`, subsystem `HumanRelations` | Event effect `modify_relation`. |
| Memory | `Source/SoulCore/Public/SoulMemory.h` · `FSoulMemoryRules::RecordPlaceLoss`/`PlaceReclaimBonus` | Site-loss/reclaim hooks keyed on `PlaceId` = region id. |
| RNG | convention at `SoulCampaignTerrain.cpp:1019` `FRandomStream R(FCrc::StrCrc32(*Id.ToString()))` | Seed events `FCrc::StrCrc32(id) ^ seed ^ Day`. |
| Data home | `Data/soul_world_overmap_v1_20260922.json` (36 nodes) + `Data/soul_campaign_start_states_v1_20260922.json` (`neutral_regions`, `value_site_candidates`, `neutral_minor_sites`, `region_control`) | Place activities on these region ids; the `value_site_candidates`/`neutral_minor_sites` are the intended data home. |

## 3. Implementation order (smallest safe slice first)

1. **Schema + loader (data-only, zero gameplay risk).** Add `FSoulActivitySite` superset + `FSoulActivityContent::Load`. Make it read the three existing sites unchanged (prove backward-compat test T-A2, T-B1). *No behaviour change yet.*
2. **Save fields.** Add `activity_site_days` + `activity_site_states` + `campaign_event_seed` to the `Soul.Campaign` blob (tolerant restore). Tests T-G1, T-G2.
3. **Generalise collection** → `InteractActivitySite`, implementing `collect`/`toll`/`investigate` (no battle) first. Ship the recurring/one-shot activities from `heartland_activity_expansion.json` that need no battle (`river_ford` toll, `crossroads` merc camp, `old_quarry` cave as investigate). Tests T-B*.
4. **Event evaluation** in `AdvanceDay()` for the non-combat events (`market_boom`, `caravan_passing` spawn, `refugee`). Tests T-D1..D7.
5. **Quests A & C** (no battle dependency). Tests T-E1, T-E2 (reach/hold/interact).
6. **Clear-then-collect + neutral occupant battles** via the existing bridge (watchtower, ruins, monster_den). This is the first step that touches combat flow → gated behind the LOCAL RUNTIME-ASSET CHECK for neutral-side rosters. Tests T-C*, Quests B/D/E.
7. **AI interest wiring** (data-fed) + optional `ClearNeutralSite`. Tests T-F*.

## 4. Dependencies & conflicts with other lanes

- **Lane 9 (economy):** owns ALL numbers. Every `amount`/`strength`/`chance`/`window`/`resource_value_bonus` here is `BALANCE_PENDING`. **Conflict risk:** if Lane 9 assigns `value_site_candidates.*.economy_resource_id`, the `monster_den.forest_edge` "timber active" reward must align. **Dependency:** Lane 8 provides the sites; Lane 9 prices them.
- **Lane 2 (AI composition):** owns whether neutral/monster occupant stacks are real. `ai_site_interest.md` assumes `GarrisonStrength`-on-neutral-region (no new army). **Conflict risk:** if Lane 2 introduces neutral armies, align the `occupant_faction`/strength model. Also owns whether `AdvanceEnemyAI` is activated (today inert) — AI site contest is untestable in-game until then.
- **Lane 4 (prisoners/Diplomacy V1):** owns relation targeting. `modify_relation` / "nearest same-biome faction" is a placeholder pending V1.
- **Lane 5 (settlement trees):** `value_site_candidates` are shared data; a cleared `monster_den` unlocking a timber site should feed a settlement/economy effect Lane 5 defines.
- **Lane 1 (campaign victory):** quests here are tactical/map chains, NOT victory conditions. **No overlap** — Lane 1 owns macro victory; Lane 8 owns the "reason to detour" micro layer. The `quest_progress` save field is distinct from any victory tracker.
- **Lane 6 (Dwarf Hold/Viking Harbour):** `fort`/`neutral_garrison` kinds are deferred to avoid colliding with their visit-binder/settlement work.
- **Lane 7 (siege V1):** no overlap; Lane 8 does not touch siege. `fort` activities explicitly deferred until siege depth lands.
- **Unreal integration lane:** owns the event-choice modal (refugee yes/no) and any caravan travel presentation. Lane 8 only defines the data/state/determinism.

## 5. Contradictions against current assumptions (flagged)

1. **"Sites are a system" — FALSE.** Any assumption that Soul has a reusable strategic-site or quest framework is wrong; it has one Heartland-scoped feature + a battle-only objective struct. Plan accordingly (extend, don't reuse a nonexistent base).
2. **"Owned assets are usable" — OVERSTATED.** The Copperlight catalog proves *ownership*, but reports `Imported=false` for every activity-relevant pack, and its `imported_unreal_summary.json` is for a **different project** (Lineage.uproject). The only verified in-Soul /Game art is `Medieval_Megapack` / `Viking_Village` / `JustBStudios/Water_City` (from `Data/environment_asset_bindings.json`). **Every new-pack use is LOCAL RUNTIME-ASSET CHECK REQUIRED.**
3. **"The AI will contest sites" — NOT TODAY.** `AdvanceEnemyAI` is inert in all shipped campaigns ("garrisons hold; no synthetic strategic army…"). Site interest is designed but **unexercisable in-game** until a strategic AI turn is activated (Lane 2).
4. **"Events can damage/capture automatically" — NO.** By design, no event auto-resolves combat or captures regions; this preserves the single battle authority and keeps low-aggression/passive play viable. This may contradict a mental model of "random bad things happen to you."
5. **`value_site_candidates` are unbalanced sentinels** (`economy_resource_id: null`, `status: BALANCE_PENDING`) — do not treat them as live economy today.

## 6. Explicit unknowns (hand to the relevant lane)
- **LOCAL RUNTIME-ASSET CHECK REQUIRED:** import/presence in `Soul.uproject` of ruin/tower/cave/den/wagon/mercenary meshes; skeletal mesh + anim + collision + nav for any monster_den occupant.
- **LOCAL RUNTIME-ASSET CHECK REQUIRED:** can the existing battle bridge raise a battle against a `NAME_None` neutral occupant (roster source unverified)?
- **DESIGN gap:** no item/relic authority → `artifact_cache` / `relic_item` BLOCKED.
- **UI gap:** founder HUD exposes site-interaction buttons but no event-choice modal.
- **BALANCE_PENDING:** all tunable numbers (Lane 9).

## 7. Integration summary for Astra / Codex

Build the next map-activity layer as a **strict additive extension of the Heartland-site pattern inside `USoulFounderPlaytestStateSubsystem`**, not a new subsystem. The cheapest correct first commit is: (a) a superset `FSoulActivitySite` + loader that reads the current three sites unchanged, (b) `activity_site_days`/`activity_site_states` added to the existing `Soul.Campaign` save blob, (c) `InteractActivitySite` generalising `InteractHeartlandSite` for non-combat interactions, and (d) a seeded `EvaluateStrategicEvents` call inside `AdvanceDay()` after the day increment. Everything combat-touching (clear-then-collect, neutral dens, AI contest) stays behind a LOCAL RUNTIME-ASSET CHECK and the still-inert strategic AI. Keep all randomness in one seeded `FRandomStream(FCrc::StrCrc32(id) ^ campaign_event_seed ^ Day)` so save/load stays deterministic.

### Highest-priority integration action
**Add the superset activity data model + loader that re-expresses the existing windmill/quarry/shrine with zero behaviour change (deliverable `strategic_activity_schema.json` → `activity_site_definition`, test T-A2/T-B1), and extend the existing `Soul.Campaign` save blob with `activity_site_days`/`activity_site_states` (test T-G1/T-G2).** This is the keystone: it unlocks every other Lane 8 feature while touching no runtime behaviour and creating no second authority — the safest possible first merge.
