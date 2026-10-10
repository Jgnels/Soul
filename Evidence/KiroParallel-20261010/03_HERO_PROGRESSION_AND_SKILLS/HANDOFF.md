# LANE 3 — HERO XP, LEVELS, SKILLS, CLASSES & COMPANION ROLES — HANDOFF

**Lane:** `03_HERO_PROGRESSION_AND_SKILLS`
**Branch:** `kiro/hero-progression-skills-20261010`
**Source snapshot (VERIFIED):** `Jgnels/Soul` `handoff/soul-kiro-20261010` = `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c` (resolved exactly; no mismatch).
**Scope boundary honored:** only files under `Evidence/KiroParallel-20261010/03_HERO_PROGRESSION_AND_SKILLS/` were written. No runtime `Source/`, `Config/`, `Content/`, `.uproject`, maps or saves were touched.

Label legend: **VERIFIED FROM CURRENT SNAPSHOT** · **VERIFIED OWNED ASSET METADATA** · **LOCAL RUNTIME/ASSET CHECK REQUIRED** · **DESIGN PROPOSAL**.

---

## 1. Headline conclusion

**Soul already has a working, deterministic, save-persisted hero-progression spine.** This is not greenfield. The frozen snapshot contains:

- `FSoulHeroState` with `Level / Experience / UnspentSkillPoints / Mana / MaxMana / Skills / KnownSpells / Kind` (and an unused `ArtifactSlots`). — `Source/SoulCore/Public/SoulHero.h`
- `FSoulHeroRules` with a quadratic XP curve, level-up with +1 skill point, generic skill rank-up, spell learning, mana spend/restore, and wound/capture + recovery. — `Source/SoulCore/Private/SoulHero.cpp`
- Live XP sources (region capture +140, battle win +360), daily mana regen (+6), anti-farm one-shot gates (`RewardedRegions`, `ResolvedEncounters`), and full save round-trip through the single `Soul.Campaign` domain. — `Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp`
- Two live, UI-wired skills: `Adventure` (AP) and `Magic` (mana), rank-capped at 2.
- A V0 **companion** mechanic (hire "Rowan", assign to "Aurora's party"), persisted as `bCompanionAssigned`.
- A recruitment pipeline for new heroes gated by a physical venue (`FSoulHeroRecruitmentRules`). — `Source/SoulCore/*/SoulHeroRecruitment.*`

**Therefore Lane 3 is an EXTENSION design, not a rebuild.** The whole lane is deliberately additive and backward-compatible: it preserves the two legacy skill ids, keeps the save schema at **v1** via optional-with-default fields, and adds no second save/magic/economy authority.

## 2. Deliverables in this directory
| File | Purpose |
|---|---|
| `hero_progression_schema.json` | Data contract: verified struct/rules + proposed curve, archetype binding, additive fields, assignment roles. |
| `hero_archetypes.json` | 6 archetypes bound by read-only data table; verified current heroes (Aurora, DwarfCommander, Rowan). |
| `xp_curve_and_sources.json` | Verified curve + sources + anti-farming; proposed scaled/consolation/siege/site/diplomacy sources; worked 30-day projections. |
| `skill_domains.md` | 6 small domains (command/martial/magic/logistics/diplomacy/stewardship); legacy alias map; bounds. |
| `companion_roles.md` | ArmyCompanion (V0, exists) / IndependentCommander (V1) / Governor (V2); minimal state to make companions progress. |
| `ui_and_save_contract.md` | Single `Soul.Campaign` save authority; verified serialized hero fields; additive optional keys with no schema bump; UI surface. |
| `acceptance_tests.md` | 30 deterministic checks incl. explicit "MUST stay green" regression guards. |
| `HANDOFF.md` | This file. |

## 3. Exact current source structs & symbols (VERIFIED FROM CURRENT SNAPSHOT)

Hero core:
- `Source/SoulCore/Public/SoulHero.h` — `FSoulHeroState`, `ESoulHeroKind {Hero,Paragon}`, `ESoulHeroCondition {Healthy,Wounded,Captured}`, `FSoulHeroRules` (`IsAvailable`, `CanCommandSiege`, `CanPerformDiplomacy`, `ApplyBattleInjury`, `AdvanceRecovery`, `ExperienceForLevel`, `AddExperience`, `SpendSkillPoint`, `LearnSpell`, `SpendMana`, `RestoreMana`).
- `Source/SoulCore/Private/SoulHero.cpp` — the only implementation of the above. **This is the smallest clean authority for the progression curve and skill-point mechanics.**

Recruitment:
- `Source/SoulCore/Public/SoulHeroRecruitment.h` / `.cpp` — `FSoulHeroCandidate`, `FSoulHeroRecruitmentState`, `FSoulHeroRecruitmentRules` (venue-gated hiring). **Archetype→hero binding data should live beside `FSoulHeroCandidate`.**

Faction/roster separation:
- `Source/SoulCore/Public/SoulFaction.h` / `.cpp` — `FSoulFactionDefinition.HeroIds`; `FSoulFactionRules::Validate` enforces heroes are **separate** from the 7 core unit slots. Do not merge heroes into roster slots.

Integration / save / XP wiring:
- `Source/Soul/Public/SoulFounderPlaytestStateSubsystem.h` + `Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp` — holds `FSoulHeroState Hero` (Aurora) and `FSoulHeroState DwarfCommander`; `ChooseSkill`, `HireTavernHero`, `AssignHeartlandCompanion`, `CompanionStatus`, `MovePlayerTo` (+140 XP), `ApplyBattleResult` (+360 XP, `Hero.Mana` from result), `AdvanceDay` (+6 mana, `AdvanceRecovery`), `RefreshHeartlandSpellLearning`, `ConfigureHeartlandMagic`, and the `Soul.Campaign` `IRBSaveDomainProvider` capture/restore. **This is the smallest clean authority for XP sources, mana regen, companion assignment, and the save contract.**

Magic / affinity (coordinate, don't fork):
- `Source/SoulCore/Public/SoulMagic.h` — `FSoulSpellDefinition`, `FSoulMagicRules`.
- `Data/SettlementEnvironments/HeartlandDevelopment.json` — `hero_affinity` (primary Frost; secondary Water/Air; forbidden Fire/Lightning), spells, sites.
- `Source/Soul/Public/SoulHeartlandContent.h` — `FSoulHeartlandContent::AllowsSchool`.
- `Source/SoulRealtimeBattle/Public/SoulRealtimeBattleArena.h` + `SoulBattlePlayerActions.cpp` — spell admission ("Affinity / Mage Guild locked"), `PlayerManaValue`. Magic cost reduction (`magic.focus`) must attach here, **LOCAL RUNTIME/ASSET CHECK REQUIRED**.

Veterancy (do NOT conflate with hero level):
- `Source/SoulCore/Public/SoulVeterancy.h` — regiment rank is a **separate** five-rank XP authority; hero Level must not alter regiment rank (comment is explicit in header).

Existing tests that lock current behavior:
- `Source/SoulCore/Private/Tests/SoulMechanicsTests.cpp` → `FSoulHeroProgressionTest`, `FSoulVeterancyTest`, `FSoulFactionContractTest`.
- `Source/SoulCore/Private/Tests/SoulHeroRecruitmentTests.cpp`.
- `Source/Soul/Private/Tests/SoulCampaignManaTests.cpp` → mana persistence + atomic receipt rejection + legacy tolerance.
- `Source/Soul/Private/Tests/SoulHeartlandTests.cpp` → affinity/paid progression.

## 4. Additive fields required (DESIGN PROPOSAL — exact)

Preferred **zero new `FSoulHeroState` field** V0:
- Archetype is data-derived from `HeroId` (new read-only `FSoulHeroArchetypeTable`); nothing serialized; schema stays v1.
- Skill ids become namespaced (`domain.skill`) with legacy aliases `Adventure→logistics.pathfinder`, `Magic→magic.attunement`. `Skills` map and save path unchanged.

If assignment roles / progressing companion ship:
- `FSoulHeroState` gains at most **one** optional field: `FName AssignmentRole` (default `NAME_None` == ArmyCompanion). Serialized as optional `"hero_assignment"` string.
- Subsystem gains `FSoulHeroState Companion;` (Rowan), serialized as optional nested `"heartland_companion"` object (mirror of `heartland_dwarf_commander`).
- **No schema bump**: both optional-with-default, validated like `result_mana`/`hero_kind` (atomic reject on out-of-range; absence → safe default).

## 5. Implementation order (for Codex / integration lane)

1. **Namespacing + legacy aliases (zero risk).** Teach `ChooseSkill` to accept new `domain.skill` ids and map `Adventure`/`Magic` aliases to identical effects; widen the hard-coded `MaxRank=2` to a per-skill cap table. Add acceptance tests 7–9. No save change.
2. **Archetype data table (read-only).** Add `FSoulHeroArchetypeDef`/table + hero→archetype bindings in `Data/`; recompute on load; expose archetype label to UI. No save change.
3. **XP source hardening.** Add diminishing-recapture, battle-loss consolation, and scaled-win XP through `AddExperience` only; keep one-shot gates. Tests 16–20, 18.
4. **Skill effects wiring (bounded).** Implement command/martial/magic/logistics/diplomacy/stewardship effects as pure integer modifiers; attach to existing RBCombat/logistics/diplomacy/settlement authorities. Resolve the two LOCAL CHECKS before `command.rally`/`magic.focus`. Tests 6,10,15.
5. **Companion as real hero (optional save key).** Promote Rowan to `FSoulHeroState Companion`; serialize optional `heartland_companion`; grant XP only when assigned. Tests 22,26,27.
6. **Assignment roles.** ArmyCompanion (done) → add Governor (V2, via Soul.Settlements) → IndependentCommander (V1, needs hero-army ownership plumbing). Tests 23.
7. **UI hero sheet.** Presentation-only panel reading `FSoulHeroState`. Tests 29–30.

Order rationale: steps 1–3 are backward-compatible and independently shippable; step 6 (IndependentCommander) is last because it needs new plumbing that no other lane provides.

## 6. Dependencies & conflicts

**Dependencies**
- **Lane 4 (Prisoners/Ransom/Diplomacy V1):** owns Captured-hero release/escape/ransom. Lane 3 only reads `IsAvailable` and declares `diplomacy.negotiator`'s *effect site*. Captured heroes do not auto-recover here by design (VERIFIED `AdvanceRecovery` ignores Captured).
- **Lane 10 (Unit tactics):** `command.*` modifiers must land in the same RBCombat morale/CHARGE pipeline Lane 10 is specifying; CHARGE must not force melee when a ranged posture is valid.
- **Lane 9 (Economy/30-day balance):** XP payouts and skill economic effects (`stewardship`, Governor gold/day) feed Lane 9's model; recapture-diminish coordinates with snowball controls.
- **Lane 5/6 (settlement trees / walkable Dwarf Hold & Viking Harbour):** Governor stewardship bonuses apply through Soul.Settlements building effects those lanes define.
- **Lane 2 (AI army comp/recovery):** anti-degeneracy (no recapture churn, no stranded commanders) aligns with Lane 3's diminishing-recapture XP.

**Conflicts to avoid (hard)**
- Do **not** create a second save authority — extend `Soul.Campaign` only (optional fields, no schema bump).
- Do **not** create a second magic/mana authority — route `magic.*` through existing spell admission and campaign mana.
- Do **not** let hero `Level` alter regiment `Rank` — `SoulVeterancy.h` is a separate authority (header says so explicitly).
- Do **not** promote a hero into one of the 7 core roster slots — `FSoulFactionRules::Validate` forbids it.
- Do **not** introduce routine permanent death or giant perk trees (shared rules + lane brief).

## 7. Explicit unknowns
- **LOCAL RUNTIME/ASSET CHECK REQUIRED:** exact RBCombat read-site that finalizes per-cast mana cost (to attach `magic.focus` without a second authority).
- **LOCAL RUNTIME/ASSET CHECK REQUIRED:** whether `command.rally` morale can be injected additively into the deterministic RBCombat pipeline without perturbing existing damage/initiative tests.
- **LOCAL RUNTIME/ASSET CHECK REQUIRED:** whether RBCombat can present a second friendly hero actor (progressing companion) in the arena, or whether V0 companion value must be a pre-battle army buff. (`SoulRealtimeBattleArena.h` exposes a single `PlayerHero`.)
- **LOCAL RUNTIME/ASSET CHECK REQUIRED:** hero-to-army ownership binding for IndependentCommander does not exist today (player army is a single region-tied `TMap`); V1 needs new plumbing.
- `FSoulHeroState::ArtifactSlots` is declared but unused and unserialized — out of scope, flagged.
- **VERIFIED OWNED ASSET METADATA:** Copperlight lists owned character packs ("Creative Characters FREE - Animated Low Poly 3D Models", "Fantasy FREE - Low Poly 3D Models Pack", "RPG - Crafting & Environment VFX"); Kenney UI is the shared UI baseline. No specific hero portrait/companion asset binding is proven — any `/Game` portrait path is **LOCAL RUNTIME/ASSET CHECK REQUIRED**. No invented paths, no new purchases.

## 8. Contradictions against current assumptions
1. **"Hero progression still needs to be built."** FALSE — a deterministic, save-persisted spine already exists (`FSoulHeroRules` + subsystem wiring). The work is extension/compatibility, not greenfield.
2. **"The companion is already a full progressing hero."** FALSE — Rowan is only two bools (`bSecondHeroHired`, `bCompanionAssigned`); no `FSoulHeroState`, no XP, no skills. Only Aurora earns XP today; the DwarfCommander earns none.
3. **"There are rich skill trees."** FALSE — exactly two gameplay skills (`Adventure`, `Magic`), rank cap 2. `war_magic` is test-only with no effect.
4. **"Adding hero fields needs a save schema bump."** FALSE — the existing restore path already tolerates optional-with-default fields (`result_mana`, `hero_kind`); new keys can be added at schema v1.

## 9. Highest-priority integration action for Astra
Land **Step 1 (skill-id namespacing + legacy aliases + per-skill cap table)** in `ChooseSkill`/`SpendSkillPoint` FIRST. It is zero-risk, fully backward-compatible, keeps all existing tests green, and unlocks every later step (archetypes, domains, companion progression) without any save-schema change. Pair it with regression tests 7–9 and the "MUST stay green" guards 1, 24, 25.
