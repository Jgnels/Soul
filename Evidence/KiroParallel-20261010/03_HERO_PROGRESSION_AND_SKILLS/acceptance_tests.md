# Soul Lane 3 — Acceptance Tests

Snapshot: `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c` (VERIFIED resolves).

These are the deterministic checks Codex should satisfy. Format mirrors existing `IMPLEMENT_SIMPLE_AUTOMATION_TEST` suites (`Soul.Core.*`, `Soul.Integration.*`). **Lane 3 writes no runtime code** — these are the contract the integration lane must make green. Tests marked **(exists)** already pass on the current snapshot and MUST remain green (regression guard).

## A. Curve & levelling (unit, `Soul.Core.Hero.*`)
1. **(exists)** `Soul.Core.Hero.ProgressionAndMagic`: AddExperience(300) levels; grants a skill point; SpendSkillPoint works; LearnSpell once; mana spend/restore clamp. — MUST stay green.
2. `ExperienceForLevel` matches table: L2=250, L3=750, L4=1500, L5=2500, L10=11250. (Pure function, no RNG.)
3. AddExperience across multiple thresholds in one call grants exactly one skill point per level crossed (e.g. 0→760 XP ⇒ Level 3, +2 points).
4. Negative/zero XP is a no-op and returns false.
5. **PROPOSAL cap:** at Level 10, further AddExperience accumulates Experience but grants no additional skill point beyond the cap rule.

## B. Skills (unit + integration)
6. `SpendSkillPoint` respects per-skill cap: cannot exceed cap; fails when `UnspentSkillPoints==0`; fails on `NAME_None`.
7. **Legacy compatibility:** `ChooseSkill(TEXT("Adventure"))` still grants +1 Max/Current AP; `ChooseSkill(TEXT("Magic"))` still grants +10 MaxMana. — regression guard.
8. `ChooseSkill(TEXT("logistics.pathfinder"))` applies the identical effect as `Adventure`; `ChooseSkill(TEXT("magic.attunement"))` identical to `Magic`.
9. A save written with legacy skill ids restores and still applies the correct runtime effects.
10. **Determinism:** two heroes given the same `(Experience, Skills)` produce identical derived modifiers (no RNG, no float drift).

## C. Wound / capture (unit, VERIFIED behavior)
11. **(exists behavior)** `ApplyBattleInjury` with survivors ⇒ Wounded, RecoveryDays=3; does not restart an existing wound clock.
12. `ApplyBattleInjury` with 0 survivors ⇒ Captured, RecoveryDays=0, CaptorFaction/Region set; a Captured hero is not re-injured.
13. `AdvanceRecovery` decays only Wounded; at 0 → Healthy. Captured does NOT auto-recover (Lane 4 owns release).
14. `IsAvailable`/`CanCommandSiege`/`CanPerformDiplomacy` are false while Wounded or Captured.
15. Skill effects that require availability (e.g. `logistics.pathfinder` free move) do nothing while Wounded/Captured. — **(exists)** for Adventure gate.

## D. XP sources & anti-farming (integration, `Soul.Integration.*`)
16. **(exists)** Region capture pays 140 XP (and 250 gold) exactly once per region (`RewardedRegions`).
17. **(exists)** Battle win pays 360 XP once per encounter (`ResolvedEncounters`).
18. Re-capturing the SAME region yields diminishing XP (140 → 70 → 35 → 17 → …) and never resets to full.
19. Spending/regaining mana grants no XP; casting is not an XP source.
20. AI-vs-AI (`bSixFactionProfile`) resolution grants the player hero no XP. — regression guard (current code only AddExperience on the Human path).

## E. Companion (integration)
21. **(exists)** `HireTavernHero` requires region/tavern/gold and is one-shot; `AssignHeartlandCompanion` toggles `bCompanionAssigned`; `CompanionStatus()` text is correct for the three states.
22. **PROPOSAL:** a hired+assigned companion (Rowan as `FSoulHeroState`) earns XP from the same battle-win hook; an unassigned companion earns none.
23. **PROPOSAL:** assigning Governor requires `IsAvailable`; a Captured hero cannot be assigned.

## F. Save round-trip (integration, VERIFIED authority) — strongest regression wall
24. **(exists)** `Soul.Integration.Vertical.ManaPersistsAcrossBattleReloadAndDay`: mana persists across battle, reload, and day; day grants exactly +6; second battle does not reset mana to 80. — MUST stay green.
25. **(exists)** `Soul.Integration.Vertical.ResourceReceiptMigrationAndAtomicRejection`: invalid optional receipts reject atomically (checkpoint byte-identical); legacy checkpoint without `result_mana`/`hero_kind` still loads; hero defaults to Kind=Hero. — MUST stay green AFTER new optional keys are added.
26. **PROPOSAL:** capture→restore of hero `level/xp/points/skills/spells` is exact; adding new optional keys (`hero_assignment`, `heartland_companion`) does not break a legacy save that lacks them.
27. **PROPOSAL:** an out-of-range new optional key (e.g. negative companion level, invalid condition ordinal, unknown assignment name) rejects the whole restore atomically, leaving the prior checkpoint unchanged. (Mirror of test 25.)
28. **PROPOSAL:** schema version remains 1 (no bump) because all new fields are optional-with-default.

## G. UI (presentation, non-authoritative)
29. Hero sheet reads `FSoulHeroState` only and never writes canonical values; skill buttons disabled when `UnspentSkillPoints==0` or rank at cap.
30. XP-to-next shown equals `ExperienceForLevel(Level+1) - Experience`.
