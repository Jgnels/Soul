# LANE 2 HANDOFF — AI Army Composition, Recruitment & Recovery

- **Lane:** `02_AI_ARMY_COMPOSITION_AND_RECRUITMENT`
- **Source snapshot (frozen):** `Jgnels/Soul` @ `handoff/soul-kiro-20261010` = **`2c3a9055d71a87ed3ed8b8abf08503efa1218f7c`** (verified exact match at session start).
- **Noncanonical work branch:** `kiro/ai-army-composition-recruitment-20261010`
- **Scope boundary honored:** No edits to `Source/`, `Config/`, `Content/`, `Soul.uproject`, maps, or saves. All output under this Evidence directory only. No PR, no merge, no purchases, no invented `/Game` paths.
- **Asset cross-reference:** `Jgnels/Copperlight-Asset-Catalog` (read-only, authenticated).

Evidence tags used throughout: **VERIFIED FROM CURRENT SNAPSHOT**, **VERIFIED OWNED ASSET METADATA**, **LOCAL RUNTIME/ASSET CHECK REQUIRED**, **DESIGN PROPOSAL**.

---

## Artifacts in this directory

| File | Purpose |
|---|---|
| `ai_army_composition_schema.json` | Machine-readable schema: current-state model, role taxonomy, AI army profile, recruitment decision, migrations, recovery, determinism contract. |
| `ai_recruitment_scoring.md` | The explainable integer role-utility model, worked example, reason codes, exact Codex integration points. |
| `faction_role_targets.json` | Per-faction role weights, min floors, ceilings, verified runtime cost anchors, asset-support notes. |
| `anti_degeneracy_rules.md` | Rules AD-0..AD-15 + recovery rules preventing recruit loops / suicide / recapture churn / stranded commanders / monoculture. |
| `campaign_sim_test_plan.md` | Two-seed 30-turn deterministic validation plan reusing the existing alpha harness and logs. |
| `HANDOFF.md` | This file. |

---

## Key conclusions (what the snapshot actually is)

1. **VERIFIED — There ARE two AI layers, and the live one is `RunNextAlphaAction`, not `FSoulStrategyAI`.**
   - `FSoulStrategyAI::Choose` (`SoulCore/Private/SoulStrategyAI.cpp`) is a clean deterministic movement/attack/siege scorer with **no recruitment/treasury/diplomacy inputs**, and it is **only called by `SoulMechanicsTests.cpp`** — dead for the campaign.
   - The **running** enemy AI is `USoulFounderPlaytestStateSubsystem::RunNextAlphaAction` (`Soul/Private/SoulFourFactionAlpha.cpp`), driven each 0.75s by `ASoulFounderPlaytestGameMode::Tick`. It **does recruit, move, capture, withdraw, and attack** for Dwarves/Orcs/Vikings.
   - ⚠️ **Correction to a widespread assumption:** "the AI never recruits / `AdvanceEnemyAI` is a no-op" is only true for the **six-faction** and **single-enemy founder** profiles. In the **four-faction alpha** profile the AI recruits with real gold. Integrate there.

2. **VERIFIED — Recruitment already pays real costs and respects stock, buildings, diplomacy, fog-of-war, and a force-ratio gate.** `FSoulCampaignRules::Recruit/CanAfford/SpendAction` deduct gold and decrement finite pools; `FSoulTownRules`/`IsOperational` gate by operational buildings; `DiplomacyAllowsHostility` and `FSoulWorldRules::IsVisible` are enforced; attacks are skipped below ~85% force ratio. **These must be reused, not re-implemented.**

3. **VERIFIED — The core degeneracy is monoculture.** Each AI faction is seeded with ONE pool (a clone of the human pool renamed to its strategic unit), so **every AI army is 100% one unit type**. Mixed companies are impossible today.

4. **VERIFIED — There is no reinforcement/recovery of damaged companies.** `balance_lab_tuning_ranges.json` marks `defeated_army_replacement_delay_days`, `..._retinue_fraction`, and `..._readiness_permille` as `"undefined"`, and the whole file is `PROPOSAL_ONLY_NOT_RUNTIME`. Recovery today = manually re-recruit from pools; the human player is explicitly told "No free reinforcements" (`HumanRecoveryGuidance`).

5. **VERIFIED — The campaign army is mono-unit at the struct level.** `FSoulCampaignArmyState` has a single `UnitId` + `TroopCount`. Mixed companies require an additive multi-unit field (MIG-1). This is the one unavoidable `Source/` change and it belongs to Codex.

6. **VERIFIED — Determinism is already a first-class invariant** (stable `FName` ordering, CRC-of-(target+faction+seed+day) tie-break, caller-seeded `FRandomStream`, no hidden mutable RNG, `AlphaSeed` restored on load). Any new logic must obey it.

7. **VERIFIED OWNED ASSET METADATA — faction visual coverage is uneven.** Vikings and Humans have owned character/weapon/animation products (e.g. *Viking Warrior*, *Norse Shield maiden*, *Crimson Knight Warrior*, *92 Animations For Warrior*, *Weapon And Shield Animations*; some marked Imported). **Dwarf and Orc character bodies were NOT found** in catalog search (orc hammer weapon props exist, but a weapon is not a unit). Troop visuals for dwarf/orc ranged companies are an open content dependency — **LOCAL RUNTIME/ASSET CHECK REQUIRED**, and no `/Game` paths are asserted here.

---

## Contradictions against current assumptions

| Assumption likely in play | Reality in frozen snapshot | Evidence |
|---|---|---|
| "The enemy AI never recruits (AdvanceEnemyAI is a no-op)." | FALSE for four-faction alpha — it recruits/moves/attacks with real gold. True only for six-faction/single-enemy profiles. | `SoulFourFactionAlpha.cpp::RunNextAlphaAction`; `AdvanceEnemyAI` branches |
| "FSoulStrategyAI is the strategic AI brain." | It is dead code for the campaign (only `SoulMechanicsTests.cpp` calls it). The live brain is the subsystem. | `grep FSoulStrategyAI` → only tests + self |
| "AI armies are already mixed companies." | Every AI army is 100% one unit; AI factions have only one pool each. | `InitializeFourFactionAlpha` pool clone |
| "Recovery/reinforcement exists." | Confirmed ABSENT; replacement tuning is `undefined`. | `balance_lab_tuning_ranges.json` |
| "Recruitment could be free for AI to simplify." | Would break parity; AI already pays the SAME 140-gold price as the player. | `InitializeFourFactionAlpha` clones human cost/income |
| "Owning Viking/weapon assets means dwarf/orc troops are covered." | No dwarf/orc character body confirmed; ownership is per-product and uneven. | Copperlight `products.json` search |

---

## Implementation order (for Codex — exact symbols)

Ordered so each step is independently testable and never creates a second authority.

1. **STEP 1 — MIG-1: additive multi-unit army.** Add `TMap<FName,int32> Companies` to `FSoulCampaignArmyState` (`SoulCore/Public/SoulCampaign.h`); keep `UnitId`/`TroopCount` as a derived compat view (`TroopCount = sum(Companies)`). Extend the save payload in `CaptureRBSaveDomain_Implementation` / `RestoreRBSaveDomain_Implementation` (same `Soul.Campaign` domain — **no new save domain**). *Test:* existing save/load qualifications still pass (T-DET-2).

2. **STEP 2 — MIG-2: seed per-role AI pools.** In `InitializeFourFactionAlpha` (`SoulFourFactionAlpha.cpp`), seed each `ActiveAI()` faction with melee_line + ranged pools (and support where targeted) per `faction_role_targets.json`, using the SAME cost/growth/capacity style already cloned from the human pool. *Test:* T-COST-3 parity; `role_pool_absent` reasons disappear.

3. **STEP 3 — the role-utility recruiter.** Replace the single-unit recruit branch in `RunNextAlphaAction` (`if(F.Army.TroopCount<30)`) with the utility model from `ai_recruitment_scoring.md`: pick best unit id by `Utility(u)`, qty-clamp to role gap, route through the existing `PrepareControlledRecruitment`/`ExecuteControlledRecruitment` → `FSoulCampaignRules::Recruit`. Emit `SOUL_RECRUIT_DECISION`. *Test:* T-COMP-1/2, T-COST-1/2, T-DEG-RECRUITLOOP.

4. **STEP 4 — MIG-3: visible-enemy role summary.** Extend `ArmyCountAtRegion` (or add a sibling) to return per-role counts for *visible* armies only (respect `FSoulWorldRules::IsVisible`). Wire `CounterScore`. *Test:* T-DEG-SUICIDE composition-aware variant.

5. **STEP 5 — anti-degeneracy guards.** Add AD-10 recapture cooldown (reuse `FSoulMemoryRules`), AD-11 garrison-before-advance (extend `CapitalThreat`), AD-12 hold-ability check (reuse `Distance(...,Supply)`). *Test:* T-DEG-CHURN, T-DEG-STRANDED.

6. **STEP 6 — recovery/replenishment.** Implement Section 6 of `anti_degeneracy_rules.md` using the balance-lab defaults (delay 3, retinue 20%, readiness 550, no veterancy for annihilated). Gate on operational owned settlement. *Test:* T-REC-1/2/3.

7. **STEP 7 — sim qualification.** Add a `-SoulRecruitSimQualification` tick sibling to `TickFourFactionAlphaQualification` that runs the two-seed 30-day plan and asserts the pass table. *Test:* all of `campaign_sim_test_plan.md`.

---

## Dependencies & conflicts with other lanes

- **Lane 5 (faction building trees) & Lane 6 (Dwarf Hold / Viking Harbour settlements):** MIG-2 role pools SHOULD be unlocked by faction buildings, mirroring the human `human.barracks`→`human_guard` gate (**VERIFIED** `CanRecruitHumanCompany`). Coordinate the building IDs so `RequiredBuildingId` links are consistent. **Conflict risk:** if Lane 5 renames building IDs, this lane's `BuildingReadyBonus` keys must track them.
- **Lane 9 (economy / 30-day balance):** owns the authoritative costs/income and the `balance_lab_tuning_ranges.json` numbers this lane borrows (recruit price 140, income 450, replacement fractions). **This lane's defaults are provisional and defer to Lane 9's final table.**
- **Lane 10 (unit role tactics / ranged / cavalry):** owns in-battle role behavior. This lane only decides *what to recruit*; Lane 10 decides *how it fights*. The role taxonomy (melee_line/ranged/support/apex) must stay consistent between the two. **No cavalry role is proposed here** because Lane 10 flags mount-pipeline uncertainty — composition targets intentionally avoid a cavalry role until Lane 10 confirms a trustworthy runtime.
- **Lane 4 (prisoners/diplomacy V1):** captured-commander effects may remove an army's commander; the recruiter must treat a commander-less army per Lane 4's rules (do not recruit into an unled stack). Coordinate on the "stranded commander" definition.
- **Lane 7 (Siege V1):** `SiegeFocused` doctrine weights assume siege prep costs/time from Lane 7. **Do not** change Human Capital Siege V0.
- **No conflict with save/combat/diplomacy authorities:** this lane deliberately reuses `FSoulCampaignRules`, the `Soul.Campaign` RBSave domain, `FSoulDiplomacyRules`, and the battle bridge — it creates none of its own.

---

## Explicit unknowns (must be resolved locally, not guessed)

1. **LOCAL RUNTIME/ASSET CHECK REQUIRED — can AI factions carry multiple pools through the battle bridge?** `SoulCampaignBattleBridge` builds encounters from the strategic army; a multi-unit army (MIG-1) must map cleanly to battle regiments. Needs a local Unreal build + `SoulControlledBattle` qualification.
2. **LOCAL RUNTIME/ASSET CHECK REQUIRED — dwarf & orc ranged/character content.** No dwarf/orc character body confirmed in Copperlight. Until confirmed, dwarf/orc ranged targets are aspirational; the recruiter clamps to melee and logs `role_pool_absent`. Do not assert any `/Game/.../DwarfArcher` path.
3. **LOCAL RUNTIME/ASSET CHECK REQUIRED — real overmap travel distances.** Travel cost 8 is a `current_mirror`; churn/strand thresholds depend on actual authored path lengths.
4. **DESIGN PROPOSAL pending Lane 9 — final costs & replacement fractions.** The 140/180/220 gold and 3-day/20%/550 recovery values are provisional.
5. **UNKNOWN — support_magic unit content for any faction.** No `SupportMagic` content unit id was found in the running rosters (only roster *slots* in the validator). Support targets are inert until such a unit exists.
6. **UNKNOWN — whether `apex` should ever be AI-recruited in the four-faction game.** `nature_bear_warrior` is the only apex and nature is passive; dark has no strategic unit at all (`AdmittedStrategicUnit` returns `NAME_None`). Apex recruiting is defined but not activated for the active factions.

---

## Highest-priority integration action for Astra

**Make AI armies mixed before anything else:** land **MIG-1 (multi-unit `FSoulCampaignArmyState.Companies`) + MIG-2 (seed melee_line + ranged pools for Dwarves/Orcs/Vikings in `InitializeFourFactionAlpha`), then swap the single-unit recruit branch in `RunNextAlphaAction` for the role-utility recruiter.** This is the one change that converts the current 100%-one-unit AI into deterministic mixed companies while reusing the existing cost/stock/building/diplomacy authorities — unlocking every downstream composition, anti-degeneracy, and recovery test. Everything else (counter-play, recovery tuning) layers on top without new authorities.
