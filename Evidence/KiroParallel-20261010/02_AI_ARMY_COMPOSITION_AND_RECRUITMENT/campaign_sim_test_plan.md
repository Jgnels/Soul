# Campaign Simulation Test Plan — AI Army Composition & Recruitment (Lane 2)

Source snapshot: `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c`.
Purpose: a reproducible, two-seed, 20–30-turn validation that the new deterministic recruiter produces **mixed companies that pay real costs**, recovers correctly, and never triggers the degeneracies in `anti_degeneracy_rules.md`.

Tags: **[VERIFIED]** (exists in snapshot) / **[PROPOSAL]** (new).

---

## 1. Harness — reuse existing, do not invent

**[VERIFIED] runtime entry points that make this testable headlessly:**
- Profile flag: `-SoulFourFactionAlpha` plus `-SoulHeartland` (set `bFourFactionAlpha` / `bHeartlandEnabled`). Driven by `InitializeFourFactionAlpha`.
- Deterministic seed: `-SoulAlphaSeed=<N>` → `AlphaSeed` (clamped 0..1,000,000). **[VERIFIED]** `FParse::Value(..., TEXT("SoulAlphaSeed="), AlphaSeed)`.
- Turn driver: `ASoulFounderPlaytestGameMode::Tick` calls `RunNextAlphaAction()` every 0.75s while alpha is active. **[VERIFIED]**
- Qualification harness pattern: `-SoulAlphaQualification` → `TickFourFactionAlphaQualification` (see `SoulFourFactionAlphaQualification.cpp`); controlled-battle harness `-SoulControlledBattle=<faction>` → `TickControlledBattleQualification` (`SoulControlledBattleQualification.cpp`) already advances days and inspects faction armies via `InspectFactionArmy`. **[VERIFIED]** Model the new sim as a sibling qualification tick so it uses the SAME save/economy authority.
- Per-action log already emitted: `SOUL_ALPHA_ACTION day=.. faction=.. action=.. region=.. troops=.. gold=.. ap=.. cursor=..` and `SOUL_ALPHA_REJECT ...`. **[VERIFIED]** These are the primary machine-readable sim outputs — parse them, do not add a parallel telemetry path.

**[PROPOSAL]** Add one new log line per recruit decision: `SOUL_RECRUIT_DECISION day=.. faction=.. unit=.. qty=.. util=.. reasons=[..] gold_before=.. gold_after=.. pool_after=..`, emitted from the extended recruit branch. This is the composition-level evidence the current `SOUL_ALPHA_ACTION` line lacks.

---

## 2. Seeds & scenarios

Run the SAME scenario twice with two seeds to prove determinism-per-seed and behavioral variety across seeds.

| Run | Flags |
|---|---|
| A | `-SoulFourFactionAlpha -SoulHeartland -SoulAlphaSeed=1701` (1701 is the current default `AlphaSeed`) **[VERIFIED default]** |
| B | `-SoulFourFactionAlpha -SoulHeartland -SoulAlphaSeed=20260925` |

Each run: advance **30 campaign days** (≈30 four-faction turn cycles; Dwarves/Orcs/Vikings active, Nature/Dark neutral — **[VERIFIED]** `ActiveAI()`), driving `AdvanceDay()` → `RunNextAlphaAction()` to cursor completion each day.

Start state anchors (**[VERIFIED]**): each AI faction starts with a single mono-unit pool clone (Available 8, growth 4, cap 24, cost 140), daily income 450 gold, capitals `dwarf_hold`/`orc_camp`/`viking_harbour`. Human player held passively (scripted to Hold) so the measurement is of AI-vs-AI/neutral behavior, not player skill.

---

## 3. Determinism gate (must pass before any behavioral assertion)

- **T-DET-1 [PROPOSAL]** Run A twice. The full ordered sequence of `SOUL_ALPHA_ACTION` + `SOUL_RECRUIT_DECISION` lines must be **byte-identical**. Fail = hidden RNG/unsorted iteration (violates the `Consider` tie-break invariant). **[VERIFIED invariant]**
- **T-DET-2 [PROPOSAL]** Save at day 10 (reuse `CaptureRBSaveDomain_Implementation`), reload, continue to day 30. Result must equal the uninterrupted run. Proves recruiter state is fully derived from saved fields (`AlphaSeed`, `Economy.Day`, pools, army). **[VERIFIED]** `RestoreRBSaveDomain_Implementation` restores `AlphaSeed/AlphaTurnDay/AlphaNextFaction`.
- **T-DET-3 [PROPOSAL]** Runs A and B must differ in at least one captured region or composition by day 30 (seeds actually matter), but each must be internally reproducible.

---

## 4. Composition & cost assertions

- **T-COMP-1 [PROPOSAL]** By day 15, no active AI army is >65% a single role once that faction's non-melee pool exists (post MIG-2). Measured from per-role army counts. Guards AD-2.
- **T-COMP-2 [PROPOSAL]** Every active AI army with TroopCount≥ (sum of min_role_counts) satisfies all `min_role_counts`. Guards AD-1.
- **T-COST-1 [PROPOSAL]** For every `SOUL_RECRUIT_DECISION`, `gold_after == gold_before - cost*qty` and `pool_after == pool_before - qty`. Proves the recruiter uses `FSoulCampaignRules::Recruit` and never grants free troops. **[VERIFIED]** path exists.
- **T-COST-2 [PROPOSAL]** No faction's gold ever goes negative; no recruit occurs when `CanAfford` would fail. **[VERIFIED]** `CanAfford` already enforces.
- **T-COST-3 [PROPOSAL]** Human and AI pay identical unit costs (parity). Compare human `human_knight` cost vs AI strategic-unit cost in logs. **[VERIFIED]** `InitializeFourFactionAlpha` clones the human pool/cost.

---

## 5. Anti-degeneracy assertions (map 1:1 to anti_degeneracy_rules.md)

- **T-DEG-RECRUITLOOP (AD-4/AD-6) [PROPOSAL]** At most one recruit action per faction per day; cumulative recruited per army per day ≤ 4; no army exceeds `army_troop_soft_cap=60`. **[VERIFIED]** 60 cap enforced in `ValidateControlledRecruitment` (`F.Army.TroopCount+Quantity>60` rejects).
- **T-DEG-SUICIDE (AD-7/AD-8) [PROPOSAL]** Count attacks where attacker count < 85% defender count: must be **0**. **[VERIFIED]** the `*100 < *85` gate exists; the test confirms the extended composition-aware version too.
- **T-DEG-CHURN (AD-10/AD-12) [PROPOSAL]** No border region changes owner more than 3 times across 30 days between the same two factions. Flags recapture churn.
- **T-DEG-STRANDED (AD-13/AD-14) [PROPOSAL]** No commander with TroopCount==0 remains outside an owned supply route for >`replacement_delay_days+2` days without moving toward supply. Parse `withdrew to`/`stranded` verbs. **[VERIFIED]** verbs exist in `RunNextAlphaAction`.
- **T-DEG-TREATY (AD-9) [PROPOSAL]** Zero attacks against a faction in Peace/NonAggression at the time of the move. **[VERIFIED]** `DiplomacyAllowsHostility` gate.

---

## 6. Recovery assertions

- **T-REC-1 [PROPOSAL]** After an AI army loses a battle but keeps its capital, within `replacement_delay_days` it begins recruiting back toward target at an operational settlement; the rebuild rate ≤ `replacement_retinue_fraction` per day. Guards Section 6 of anti-degeneracy.
- **T-REC-2 [PROPOSAL]** A fully-annihilated army's rebuilt companies are Recruit rank (no inherited veterancy). **[VERIFIED authority]** balance-lab note.
- **T-REC-3 [PROPOSAL]** An AI whose recruitment building is ruined (integrity < min) does NOT recruit the gated unit and logs `building_down:<id>`. **[VERIFIED mechanism]** `IsOperational`/`EffectiveWeeklyGrowth` return 0 for ruined buildings.

---

## 7. Pass/fail summary table

| Test ID | Guards | Pass condition | Snapshot support |
|---|---|---|---|
| T-DET-1/2/3 | determinism, save-safety | byte-identical per seed; save-reload equal; seeds diverge | [VERIFIED] seed+tie-break+save |
| T-COMP-1/2 | AD-1, AD-2 | floors met, ≤65% any role | [PROPOSAL] |
| T-COST-1/2/3 | real cost, parity | exact gold/pool deltas, no negatives, equal prices | [VERIFIED] Recruit/CanAfford |
| T-DEG-* | AD-4..AD-14 | zero suicides/treaty-breaks; bounded churn/strand | [VERIFIED] gates + [PROPOSAL] extensions |
| T-REC-1/2/3 | recovery | gated rebuild rate, no free veterancy, building gating | [PROPOSAL] + [VERIFIED] IsOperational |

---

## 8. Explicit unknowns the sim will surface (not assumptions)

1. **[LOCAL-CHECK]** Whether non-melee pools can actually be seeded for AI factions without breaking the battle bridge (`SoulCampaignBattleBridge`) — needs a local Unreal build. Until MIG-2, T-COMP-1/2 are expected to report `role_pool_absent` rather than pass.
2. **[LOCAL-CHECK]** Real map path lengths between capitals (travel cost 8 is a `current_mirror`, not final — balance lab). Churn/strand thresholds may need retuning once the authored overmap distances are measured locally.
3. **[LOCAL-CHECK]** Whether a 30-day run completes within the qualification tick budget headlessly.
