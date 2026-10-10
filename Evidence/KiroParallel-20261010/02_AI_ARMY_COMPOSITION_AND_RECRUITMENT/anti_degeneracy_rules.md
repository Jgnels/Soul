# Anti-Degeneracy Rules — AI Recruitment & Recovery (Lane 2)

Source snapshot: `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c`.
Tags: **[VERIFIED]** / **[PROPOSAL]** / **[LOCAL-CHECK]**.

These rules exist to stop the four known failure modes the lane prompt calls out: **recruit loops, suicide attacks, recapture churn, and stranded commanders** — plus the composition degeneracy (all-one-role armies) that the current build exhibits.

---

## 0. Degeneracies present in the CURRENT build (must be fixed)

- **D0 — Monoculture armies [VERIFIED].** Each AI faction gets a single mono-unit pool (`InitializeFourFactionAlpha`), so every AI army is 100% one unit. This is the primary thing this lane fixes (see rules AD-1..AD-3).
- **D1 — No reinforcement/recovery [VERIFIED].** `balance_lab_tuning_ranges.json` marks `defeated_army_replacement_*` as `"undefined"`. A defeated AI army has no deterministic path back; it only passively grows a pool it may never spend.

---

## 1. Composition degeneracy controls

### AD-1 — Role minimum floors (prevents all-one-role)
**[PROPOSAL]** Before weight-closing, enforce `min_role_counts` from `faction_role_targets.json`. If `actual(melee_line) < min`, the recruiter is **forced** to pick a `melee_line` unit (GapScore += 1000). Symmetrically for `ranged` where `min>0`. Only after all floors are met does normal utility ordering apply.
- **Interaction with stock:** if the required role has no pool/stock (MIG-2 not done), emit `role_pool_absent:<role>` and fall back to the next affordable role — never deadlock.

### AD-2 — Single-role ceiling (65%)
**[PROPOSAL]** No recruit may push any one role above `max_single_role_fraction_permille=650` of the army total, unless that role's own min-floor is still unmet (floors win over ceilings). Enforced by `DegeneracyPenalty=2000` in the utility.

### AD-3 — Apex gating
**[PROPOSAL]** `apex_max_per_army=1` and only above `apex_troop_floor=20`. Apex never counts toward the melee_line floor. **[VERIFIED]** matches the existing roster rule (exactly 1 apex in the 7 core slots, `FSoulFactionRules::Validate`).

---

## 2. Recruit-loop prevention

### AD-4 — Spend-forward, never re-evaluate the same gold twice per turn
**[VERIFIED mechanism]** `RunNextAlphaAction` runs **once per faction per day** (gated by `AlphaNextFaction` cursor advancing and `AlphaTurnDay>=Economy.Day` in `AdvanceEnemyAI`). Recruitment consumes an action point via `SpendAction` and gold via `Recruit`. **[PROPOSAL]** The recruiter MUST take at most one recruit action per faction-activation, bounded by `recruit_cap_per_action=4`. No while-loop that recruits until broke.
- **[VERIFIED]** The current code already does `FMath::Min(4, Pool->Available)` and `while(Count>0 && !PrepareControlledRecruitment(...))--Count;` — a bounded descent, not an unbounded loop. Preserve this shape.

### AD-5 — No oscillation between recruit and move
**[PROPOSAL]** Recruit and move are scored in the SAME `Consider` comparison (as today). The higher-utility action wins once; there is no second pass that could flip the decision within one activation. Determinism via the CRC tie-break guarantees the same winner on replay. **[VERIFIED]** tie-break scheme.

### AD-6 — Target-gap stop
**[PROPOSAL]** `qty` is clamped to the remaining role gap, so once a role reaches its target the recruiter stops buying it even if stock/gold remain. Prevents buying the 61st body into a 60-cap army.

---

## 3. Suicide-attack prevention

### AD-7 — Force-ratio gate (reuse existing)
**[VERIFIED]** `RunNextAlphaAction` already skips a hostile target when `F.Army.TroopCount*100 < Defenders*85` (attacker needs ~85% of defender count). Keep this. **[PROPOSAL]** Make it composition-aware once MIG-3 lands: raise the required ratio to 100% when the defender is >60% ranged and the attacker has no ranged of its own (a pure-melee army walking into archers should be more cautious).

### AD-8 — Readiness/supply floor before committing
**[VERIFIED]** `FSoulStrategyAI` gates siege on `Readiness>=600` and biases `Recover` below 700; `FSoulLogisticsRules::ForceMarch` refuses below `Readiness<350 || Supply<250`. **[PROPOSAL]** The AI must not initiate an attack when its logistics state would fail a `ForceMarch`-equivalent threshold; instead it recovers (EndDay at friendly settlement restores +300 readiness). This routes a battered army home rather than into a second losing fight.

### AD-9 — Don't attack into a treaty
**[VERIFIED]** `DiplomacyAllowsHostility` already blocks hostile moves against peace/non-aggression partners in `RunNextAlphaAction` and `MovePlayerTo`. Keep as the single diplomacy gate; the recruiter must not plan an attack it cannot legally execute.

---

## 4. Recapture-churn prevention

### AD-10 — Memory-biased re-targeting (reuse)
**[VERIFIED]** `FSoulMemoryRules::PlaceReclaimBonus` and `RivalBias` exist (used by `FSoulStrategyAI`), with `rival_defeat_caution_cap=-800` and `rival_memory_decay_turns=7` in the balance lab. **[PROPOSAL]** Apply a **recently-lost-region cooldown**: if a faction captured and then lost region R within the last `rival_memory_decay_turns` days, reduce the move score to re-take R by a decaying penalty. Stops two armies trading the same border region every turn.

### AD-11 — Garrison-before-advance
**[PROPOSAL]** A faction may not vacate its only occupied settlement region to chase a capture if that would leave the settlement undefended and a hostile active-AI army is visible adjacent (reuse the existing `CapitalThreat` computation). **[VERIFIED]** `CapitalThreat` already raises the home-defense move score to 300; extend it to also *suppress* a low-value offensive capture when the capital is threatened.

### AD-12 — No capture of regions you cannot hold
**[PROPOSAL]** Deprioritize capturing a frontier region whose nearest owned supply settlement is unreachable (reuse the `Distance(..., Supply=true)` BFS already in `RunNextAlphaAction`). A region captured with no supply route is churn bait.

---

## 5. Stranded-commander prevention

### AD-13 — Supply-route awareness (reuse)
**[VERIFIED]** `FindOwnedRecruitmentDestination(Id)` and the `Distance(Start, Supply)` BFS already compute whether an owned recruitment settlement is reachable. `RunNextAlphaAction` already labels an army `Stranded` when `TroopCount==0 && FindOwnedRecruitmentDestination(Id).IsNone()` and makes it hold/withdraw rather than act. Keep this.
### AD-14 — Withdraw-toward-supply for depleted armies
**[VERIFIED]** When `TroopCount<18`, the AI scores owned-region moves `220 - Distance(N,true)*10` and penalizes forward moves `-180`, i.e. it retreats toward supply. **[PROPOSAL]** Extend: a `TroopCount==0` commander must prefer the shortest owned route home (already `Withdrawal cannot capture or initiate combat` per the `R.OwnerFactionId!=Id` guard) and, on arrival at an operational recruitment settlement, trigger the recovery top-up (Section 6).
### AD-15 — Human player parity guidance
**[VERIFIED]** `HumanRecoveryGuidance()` already tells the player when the capital is occupied / routed / no owned road reaches home ("No free reinforcements"). The AI recovery rules must mirror this exactly so the AI is never advantaged with free reinforcements.

---

## 6. Recovery / replenishment rules (new, deterministic)

**[PROPOSAL]** (fills the confirmed gap D1):
1. A company can be reinforced ONLY at an operational owned recruitment settlement (same gate as fresh recruitment: `FSoulTownRules`/`IsOperational`). No field reinforcement.
2. Reinforcement pays the normal pool cost per troop from treasury and consumes pool stock — identical to recruitment. There is no cheaper "repair" path for troops.
3. Per-day top-up is capped at `replacement_retinue_fraction_of_starting_strength` (proposed 20%, range 15–25% from balance lab) so a wiped army rebuilds over `replacement_delay_days` (proposed 3) days, not instantly.
4. A reinforced/rebuilt company starts at `replacement_readiness_permille` (proposed 550) — not full — so it can't immediately re-engage.
5. **Veterancy is NOT preserved for annihilated regiments** (balance-lab authority). Reinforcing a *surviving* company keeps that company's existing rank; replacing a *destroyed* one yields Recruit rank. **[VERIFIED]** `FSoulVeterancy::AddExperience` is additive per-regiment, so this is a per-regiment lifecycle rule, not a global one.

---

## 7. Determinism & save-safety invariants (hard constraints)

- **[VERIFIED]** No hidden mutable RNG stream (the `Consider` lambda comment demands this so save/load does not desync). Any new scoring state must be derivable from saved fields (`AlphaSeed`, `Economy.Day`, army/pool state), never from an unsaved counter.
- **[VERIFIED]** All faction/pool/region iteration sorted by `FName` (`FNameLexicalLess`). New role iteration must sort by role enum then unit `FName`.
- **[VERIFIED]** Recruitment goes through the single `FSoulCampaignRules::Recruit` authority; pools are already serialized in `CaptureRBSaveDomain_Implementation` (`pools` object). Any new per-role army counts must be added to that same save payload (MIG-1), never a second save domain.
