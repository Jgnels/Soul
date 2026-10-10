# Soul — 10-Lane Kiro Parallel Pack

Source snapshot authority:
- Repo: `Jgnels/Soul`
- Required source branch: `handoff/soul-kiro-20261010`
- Expected snapshot: `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c`
- Also clone/read: `Jgnels/Copperlight-Asset-Catalog`

## Shared rules for every lane

You are one of ten concurrent Kiro frontier-model lanes for RefinedBadger Soul.

If `handoff/soul-kiro-20261010` does not resolve to the expected SHA, STOP and report the mismatch. Do not silently work from stale `main`.

Current solved baseline includes: Human Heartland, walkable authored Human Capital, infantry/archers/shield guards, Rowan city embodiment, strategic windmill/shrine/quarry, woodland/river-bridge battles, HOLD/MOVE/CHARGE/FOLLOW, visible settlement development, Mage Guild + affinities, Kenney UI/cursors, Diplomacy V0, hero wound/capture, and true Human Capital Siege V0.

Do not reopen Titan/R10/terrain bakeoffs/encounter matrices/Human Capital replacement. Do not create a second combat/save/diplomacy authority. Do not recommend Dragon Graveyard as a universal fallback.

You are NOT the main Unreal integration lane. Do not edit runtime `Source/`, `Config/`, `Content/`, `.uproject`, maps or saves. Write only under:
`Evidence/KiroParallel-20261010/<LANE_NAME>/`

Produce:
- `HANDOFF.md`
- machine-readable JSON/CSV/schema where useful
- exact source/data file and symbol references
- implementation order
- tests/acceptance criteria
- conflicts/dependencies
- explicit unknowns

Every recommendation must distinguish:
- VERIFIED FROM CURRENT SNAPSHOT
- VERIFIED OWNED ASSET METADATA
- LOCAL RUNTIME/ASSET CHECK REQUIRED
- DESIGN PROPOSAL

No invented `/Game/...` paths. No invented ownership. Default to no purchases.

At completion:
- commit ONLY your lane's Evidence directory
- push to `kiro/<lane-name>-20261010`
- do NOT open a PR
- do NOT merge

Report branch, commit SHA, HANDOFF path and a concise integration summary.

---

# LANE 1 — CAMPAIGN VICTORY, OBJECTIVES, QUEST ARC AND REPLAYABILITY

Spend 3–4 hours designing the actual campaign macro-structure Soul still lacks.

Audit current campaign/world/faction/quest/save/AI code and data. Design a V0→V2 objective framework with 3–5 viable victory approaches such as military domination, alliance/federation, magical supremacy, economic/site control, and another only if justified. Keep Nature/Dark passivity compatible with the current four-faction game.

Define loss/recovery states, Days 1–7 onboarding goals, Days 8–20 midgame, endgame escalation, and faction/start-doctrine replayability. Integrate settlements, diplomacy, heroes, captured heroes, strategic sites and sieges. Use the current 36-region structure; do not redesign geography.

Output:
- `campaign_victory_schema.json`
- `campaign_progression_v0_v2.md`
- `early_mid_endgame_objectives.json`
- `starting_doctrines.md`
- `acceptance_tests.md`
- `HANDOFF.md`

Give Codex exact current files/classes where the smallest clean authority should live.

---

# LANE 2 — AI ARMY COMPOSITION, RECRUITMENT AND RECOVERY

Spend 3–4 hours auditing current AI military decisions for Dwarves, Orcs and Vikings.

Design deterministic mixed-company recruitment that pays real costs, respects stock, avoids all-one-role armies, considers enemy composition, buildings, treasury, replenishment, siege plans, diplomacy and strategic sites, and prevents recruit loops/suicide/recapture churn/stranded commanders.

Create a simple explainable role-utility model, reinforcement/recovery rules, and a two-seed 20–30-turn test plan.

Output:
- `ai_army_composition_schema.json`
- `ai_recruitment_scoring.md`
- `faction_role_targets.json`
- `anti_degeneracy_rules.md`
- `campaign_sim_test_plan.md`
- `HANDOFF.md`

Identify exact current functions/classes Codex should modify.

---

# LANE 3 — HERO XP, LEVELS, SKILLS, CLASSES AND COMPANION ROLES

Spend 3–4 hours designing persistent hero progression around current hero combat, magic affinity, wounds/capture and companions.

Design HeroXP, HeroLevel, progression curve, XP sources, anti-farming rules, 4–6 hero archetypes, small skill domains (command, martial, magic, logistics, diplomacy, stewardship), wound/capture interactions, and assignment roles such as Army Companion / Independent Commander / Governor if justified.

Keep it deterministic and save-friendly. Avoid giant perk trees and routine permanent death.

Output:
- `hero_progression_schema.json`
- `hero_archetypes.json`
- `skill_domains.md`
- `xp_curve_and_sources.json`
- `companion_roles.md`
- `ui_and_save_contract.md`
- `HANDOFF.md`

Call out exact current source structs and additive fields required.

---

# LANE 4 — PRISONERS, RANSOM, EXCHANGE, RESCUE AND DIPLOMACY V1

Spend 3–4 hours extending existing HEALTHY/WOUNDED/CAPTURED + Diplomacy V0 into a coherent prisoner lifecycle.

Design captured-hero ledger, deterministic ransom valuation, ransom offer/acceptance scoring, prisoner exchange, relationship/memory effects, AI behavior, release locations, deterministic escape rules, lightweight rescue hooks, captured-commander effects, siege-capture consequences and peace-with-prisoners interactions.

No second save authority. No torture/execution/dynasty systems.

Output:
- `prisoner_state_schema.json`
- `ransom_scoring.json`
- `exchange_rules.md`
- `ai_prisoner_behavior.md`
- `rescue_escape_hooks.md`
- `diplomacy_v1_integration.md`
- `HANDOFF.md`

---

# LANE 5 — DWARF / ORC / VIKING SETTLEMENT DEVELOPMENT TREES

Spend 3–4 hours designing faction-specific development trees using the proven Human settlement system.

Founder timing is hard:
- normal = 1 day
- significant = 2 days
- capstone = 3 days

For Dwarves, Orcs and Vikings define stable IDs, prerequisites, cost, troop/economic/hero/magic/siege effects and physical authored-settlement manifestation. Do not merely rename Human buildings. Add lighter future concepts for Nature/Dark without activating them.

Output:
- `dwarf_building_tree.json`
- `orc_building_tree.json`
- `viking_building_tree.json`
- `physical_manifestation_map.md`
- `nature_dark_future_notes.md`
- `HANDOFF.md`

---

# LANE 6 — NEXT WALKABLE SETTLEMENTS: DWARF HOLD + VIKING HARBOUR

Spend 3–4 hours preparing exact implementation plans for Dwarf Hold and Viking Harbour.

Use current environment registry, repo evidence and Copperlight metadata. Identify current bindings, authored map candidates, visible persistent/sublevel structure, likely spawn/return points, service anchors, battle/siege candidates, dependency risks, exact local checks, and the smallest generic visit-binder architecture that removes Human-only visitation.

Output:
- `dwarf_visit_plan.md`
- `viking_visit_plan.md`
- `visit_registry_extensions.json`
- `dependency_checklist.json`
- `generic_visit_binder_spec.md`
- `HANDOFF.md`

Do not design replacement cities.

---

# LANE 7 — SIEGE V1 RESEARCH: LADDERS, ENGINES, REPAIR AND DEFENDER DEPTH

Human Capital Siege V0 is qualified. Do not change it.

Spend 3–4 hours planning what comes AFTER founder feedback: ladder requirements, siege tower, ram feasibility, trebuchet/catapult, gate/wall damage, post-battle repair, siege preparation cost/time, defender wall positions, ranged defense, battlement combat, multiple approaches and AI siege preparation.

Do not treat meshes as functionality. Flag every local animation/collision/socket/nav check.

Output:
- `siege_v1_feature_order.md`
- `ladder_requirements.json`
- `siege_engine_requirements.json`
- `fortification_repair_spec.md`
- `ai_siege_plan.md`
- `local_validation_checklist.md`
- `HANDOFF.md`

---

# LANE 8 — STRATEGIC EVENTS, NEUTRAL SITES, QUESTS AND MAP ACTIVITY

Spend 3–4 hours designing the next map-activity layer beyond windmill/quarry/shrine.

Cover ruins, forts, watchtowers, caravans, resource convoys, mercenary camps, neutral/monster dens, caves, artifacts/treasure, shrines, crossings and regional war/trader/refugee events.

Each needs reason to detour, risk/reward, persistent state and AI relevance where appropriate. Use deterministic seeded events, not spam. Include compact quest-chain templates using existing systems and owned assets.

Output:
- `strategic_activity_schema.json`
- `heartland_activity_expansion.json`
- `event_pool_v0.json`
- `quest_chain_templates.md`
- `ai_site_interest.md`
- `HANDOFF.md`

---

# LANE 9 — CAMPAIGN ECONOMY, COSTS, PACING AND 30-DAY BALANCE MODEL

Spend 3–4 hours auditing current costs/income/stock/build times across recruitment, buildings, sites, diplomacy gifts, heroes, AI and sieges.

Build deterministic 30-day models for conservative player, expansionist player, AI baseline and war-heavy play.

Analyze expansion speed, company scarcity, gift price, recruitment-vs-development tradeoffs, site value, AI sustainability, snowballing, siege-loss cost and whether 1/2/3-day construction stays meaningful as territory grows.

Output:
- `current_economy_audit.csv`
- `30_day_simulation.json`
- `balance_findings.md`
- `recommended_costs_and_income.csv`
- `snowball_controls.md`
- `HANDOFF.md`

Clearly separate exact current values from recommendations.

---

# LANE 10 — UNIT ROLE TACTICS, RANGED BEHAVIOR, FORMATIONS AND CAVALRY DESIGN

Spend 3–4 hours designing the next tactical layer around existing RBCombat and HOLD/MOVE/CHARGE/FOLLOW.

Define role behavior for infantry, shield guard, archers, shock/heavy, cavalry, magic support, commander and companion: spacing, preferred range, command response, target priorities, terrain preferences and routing.

Ranged should maintain distance where feasible; HOLD may fire locally; CHARGE should not automatically mean melee if a valid ranged posture exists.

Define additive width/depth V1 only if current architecture supports it. Define cavalry runtime requirements and explicit fallback if no trustworthy mount pipeline exists.

Output:
- `unit_role_behavior_schema.json`
- `ranged_behavior_spec.md`
- `formation_width_depth_spec.md`
- `terrain_role_interactions.json`
- `cavalry_runtime_requirements.md`
- `HANDOFF.md`

Give Codex exact existing files/functions where each behavior belongs.
