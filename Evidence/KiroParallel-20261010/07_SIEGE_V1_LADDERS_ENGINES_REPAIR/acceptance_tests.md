# Acceptance Tests — Soul Siege V1

Lane 7. Research/design only. These are the test specifications Codex should implement alongside each
V1 feature, in the **existing** test style (UE automation tests like
`Source/SoulCore/Private/Tests/SoulGateAssaultTests.cpp`,
`Source/Soul/Private/Tests/SoulSiegeCampaignTests.cpp`). No tests are added by this lane.

Labels: **VCS/VOA/LRC/DP** as elsewhere. "Native" = pure C++ rules test (no editor). "Runtime" = cooked
local check (LRC). Each test names the smallest existing authority it extends.

---

## 0. Non-negotiable V0 regression lock (run on EVERY V1 phase)

- **T-REG-1 (native):** `FSoulSiegeRules::Begin({})` with empty preparation produces exactly the V0
  state (GateMaximum 1000; 1300 only with `bReinforcedGate`). Preserves `SoulGateAssaultTests.cpp`
  assertions verbatim. (VCS baseline.)
- **T-REG-2 (native):** Breach-alone-does-not-win, courtyard contest/decay, 15 s uncontested capture,
  invalid/oversized/negative damage rejection — all current `SoulGateAssaultTests.cpp` cases still
  pass unchanged.
- **T-REG-3 (integration):** The full `SoulSiegeCampaignTests.cpp` admission/result/save/cold-restore
  suite passes unchanged (fortification gating, wounded/captured commander rejection, atomic malformed
  rejection, exact F5/F9 restore).
- **T-REG-4 (runtime, LRC):** The V0 cooked siege qualification
  (`-SoulSiegeV0 -SoulSiegeQualification`) still passes end to end (`SoulHumanCapitalSiegeQualification.cpp`).

---

## 1. Preparation transport (Phase B1 — keystone)

- **T-PREP-1 (native):** New `FSoulCampaignBattleDescriptor` preparation fields default to all-false;
  `IsValid()` unchanged for non-siege. (Extends `SoulCampaignBattleBridge.h`.)
- **T-PREP-2 (native):** `FSoulSiegeRules::Begin(Prep)` is called with the transported preparation (not
  `{}`); each flag maps to its documented effect (`bReinforcedGate`→1300, `bAmmoStores`→`bArmoryActive`,
  `bMagicalWard`→`bMagicWardActive`). (VCS mapping in `SoulSiege.cpp` Begin.)
- **T-PREP-3 (integration):** Preparation round-trips through F5/F9 and a fresh-process restore with no
  new save domain / schema bump; malformed partial preparation rejects atomically; missing fields clear
  stale preparation cleanly. (Mirror the optional siege-result-summary pattern in
  `SoulFounderPlaytestStateSubsystem.cpp:711-714,880` and its tests.)
- **T-PREP-4 (native):** `CanAssaultWalls(Prep, bHasFlyingUnit)` is the single commit gate; false
  preparation + no flyer ⇒ cannot assault walls. (Reuse VCS rule.)

## 2. Wall breach vector (Phase B2 + ENGINE-WALLSEG)

- **T-WALL-1 (native):** A new per-segment integrity in `FSoulSiegeState` drives `OpenBreach`/
  `WallBreaches` and advances `ESoulSiegeLayer::Walls`; a non-breached segment blocks the layer advance.
- **T-WALL-2 (native):** A segment at zero integrity feeds `FSoulSiegeAftermath.ScarIds`, so resolve
  writes a persistent breach into `FSoulSettlementState` via `FSoulSiegeAftermathRules::Apply` (VCS).
- **T-WALL-3 (runtime, LRC):** Setting a segment Breached opens a capsule-passable passage with
  navigable interior behind it (checklist B-NAV-1/B-COL-1/B-SEG-1).

## 3. Ladders (Phase C)

- **T-LAD-1 (native):** No ladder spawns when `bLadders` false; bounded ladder count when true.
- **T-LAD-2 (native):** Climb traversal has a valid start gate (attacker side, non-ranged, ladder
  intact, at base) and reaches a landing state; interrupted climb returns to ground or dies via
  existing rout semantics (no synthetic kills).
- **T-LAD-3 (runtime, LRC):** Wall-top is capsule-walkable (checklist C-NAV-1, BLOCKING); ladder
  base/top transforms resolve from authored actors, fail-closed if absent (C-SOCKET-1).
- **T-LAD-4 (runtime, LRC):** Owned ladder animation retargets, else rule-only climb still passes
  (C-ANIM-1).
- **T-LAD-5 (native+runtime):** Defender shove-off removes climbers; contested wall-top does not
  advance the layer (mirror AdvanceCourtyard "living defender contests").

## 4. Defender depth (Phase D)

- **T-DEF-1 (runtime, LRC):** Side==1 formations garrison authored wall-position anchors instead of
  only holding behind the gate (checklist D-NAV-1).
- **T-DEF-2 (native):** Defender archers (existing `Ranged` role) can fire at attackers on the approach
  / on ladders; still cannot damage the gate (preserve `CommitSiegeHit` `bRanged` rejection VCS).
- **T-DEF-3 (native):** Clearing a held wall section advances `AdvanceLayer` one step; a still-held
  section does not.

## 5. Engines

### Battering ram (E1)
- **T-RAM-1 (native):** Ram contact drives `ApplyGateDamage` with bounded, finite, replay-protected
  (via `AcceptedContacts`) damage > a normal melee hit; at zero reuses `OpenSiegeGate`.
- **T-RAM-2 (native):** Ram is destructible/haltable; destroyed ram stops producing gate damage.
- **T-RAM-3 (runtime, LRC):** Ram crew pathes to the measured gate base across the authored approach
  (checklist E1-NAV-1).

### Catapult / Mangonel (E3)
- **T-CAT-1 (native):** One ranged engine of this class only; finite ammo (tie to `bAmmoStores`/prep
  count); bounded rate of fire.
- **T-CAT-2 (native):** Gate hit → `ApplyGateDamage`; wall-segment hit → `ENGINE-WALLSEG` integrity.
- **T-CAT-3 (runtime, LRC):** Owned mangonel imported or placeholder; projectile arc tuned; VFX
  optional (checklist E3-ASSET-1/PROJ-1/VFX-1).

### Siege tower (E2 — last)
- **T-TOW-1 (runtime, LRC):** Tower pathes to a wall face, docks, drops a ramp onto the walkable
  wall-top; troops traverse; defenders contest (checklist E2-*, depends on C-NAV-1 + D-NAV-1).

## 6. AI siege preparation (Phase F2)

- **T-AI-1 (native):** New `PrepareSiege` action accrues preparation over turns, paying real cost;
  deterministic (same seed ⇒ identical candidate order + reasons), matching `SoulStrategyAI` style.
- **T-AI-2 (native):** AI cannot commit `SiegeSettlement` until `CanAssaultWalls` is true (prepared or
  flyer) AND force ratio ≥70% (preserve VCS ratio gate).
- **T-AI-3 (native):** Flying-capable army skips preparation and commits directly (VCS flyer bypass).
- **T-AI-4 (native):** `FSoulMemoryRules::RivalBias` prevents immediate re-siege after a defeat
  (no recapture churn); prep-time sunk-cost dampener prevents dithering loops.
- **T-AI-5 (integration):** The committed battle descriptor carries non-empty preparation matching the
  AI plan (depends on T-PREP-*).

## 7. Repair (Phase G)

- **T-REP-1..A7:** See `fortification_repair_spec.md` §3 (REPAIR-A1..A7): regression-safe no-op day,
  costed/atomic `BeginRepair`, timed integrity gain, scaffolding visual via the dead `RepairRoot`
  branch, save round-trip, scar heal-but-remember.

---

## Determinism & honesty requirements (all tests)

- All native rules tests are deterministic integer logic; no randomness.
- Every runtime (LRC) test records the FPS cap and peak temperature and does NOT claim sustained
  performance qualification (mirror V0's explicit 10-FPS-functional vs 30-FPS-manual separation).
- No test asserts a feature works from mesh/asset existence alone; the gameplay RULE is the pass
  condition, with animation/VFX as non-blocking polish.
