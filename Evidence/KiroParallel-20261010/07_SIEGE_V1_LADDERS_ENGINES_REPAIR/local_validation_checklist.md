# Local Validation Checklist — Soul Siege V1

Lane 7. Research/design only. **This checklist is the explicit enforcement of the lane rule:
"Do not treat meshes as functionality. Flag every local animation / collision / socket / nav check."**

Everything here is **LRC (LOCAL RUNTIME/ASSET CHECK REQUIRED)** unless noted. Each item must be
confirmed in a local Unreal editor and/or a cooked runtime on Jeff's machine **before** the matching
V1 feature can be called qualified. A mesh existing (in `Content/` or in the owned Fab library) is
NOT evidence that the feature works.

Context (VCS): V0 proved exactly ONE physical surface — the ground approach to the native portcullis
and a 9.5 m courtyard ring behind it — via measured line/sweep traces and a cooked-runtime capsule
survey (`Source/SoulRealtimeBattle/Private/SoulHumanCapitalSiege.cpp` `SetupSiege`;
`Evidence/HumanCapitalSiege-20261010/gate-diagnosis.json`). Nothing above the ground / behind the
courtyard is proven. The authored map `L_HumanCapital_Authored` (`/Game/Soul/Maps/Settlements/...`)
is Soul-owned content NOT present in this sandbox, so none of the below can be answered from source.

Fail-closed discipline to mirror (VCS): V0's `SetupSiege` logs `SOUL_SIEGE_SETUP_FAIL` and returns
false on ambiguous/missing/moved gate, bad floor grade, unreviewed threshold drop, route misses or
route blocks, and a non-blocking closed gate. **Every new siege object must fail closed the same way.**

---

## A. Doorway throughput (Phase A) — no new asset, still needs runtime checks

| ID | Check | Pass condition | Blocking |
|---|---|---|---|
| A-NAV-1 | Multi-lane aperture | A 2–3-wide column traverses the breached gate without deadlock in cooked runtime | yes |
| A-COL-1 | Aperture collision | Bounded RVO window (as in `TickSiege`) still prevents fall-through / water-seating (the authored bridge-deck gap noted in `SiegeDeployment`) | yes |
| A-PERF-1 | Throughput perf | No new frame-time regression at the funded FPS cap (V0 functional evidence was 10-FPS capped; this is NOT a perf-qualified claim) | no |

## B. Wall breach vector (Phase B)

| ID | Check | Pass condition | Blocking |
|---|---|---|---|
| B-NAV-1 | Behind-breach nav | A navigable interior exists behind a breachable wall segment so a breach is actually enterable | yes |
| B-COL-1 | Breach collision opens | Setting a segment to Breached opens a real passage (capsule passes) and Intact blocks it | yes |
| B-SEG-1 | Segment actor present | `ASoulFortificationSegmentActor` instances exist in the authored map with populated Intact/Damaged/Breached actor arrays | yes |
| B-SCAR-1 | Scar persists to town | A battle breach writes a scar that the town-view presentation controller renders breached (round-trip) | no |

## C. Ladders (Phase C) — HIGHEST mesh!=functionality risk

| ID | Check | Pass condition | Blocking |
|---|---|---|---|
| C-NAV-1 | **Wall-top walkable** | The battlement surface has navmesh/authored path, capsule-wide (~40r/90h). **If this fails, ladders deliver zero value — STOP.** | **yes (gates all of Phase C/E2/D)** |
| C-SOCKET-1 | Ladder-base + top transforms | Deterministic deploy anchor at the wall base and a landing transform at the top, measurable from authored actors, fail-closed if absent | yes |
| C-COL-1 | Climb collision | Climber does not clip/fall through ladder or wall; no global collision disable | yes |
| C-ANIM-1 | Climb animation | Owned **Free Ladder Animation Set** (VOA; owned, NOT confirmed local, NOT imported) clips are locally present AND retarget to the RB skeletal rig via `ResolveVisualAnimation`/`PlayAnimation`; else rule-only climb fallback | no (rule must pass without anim) |
| C-SHOVE-1 | Defender shove-off | A wall defender can remove climbers on a ladder; contested wall-top does not advance `ESoulSiegeLayer::Walls` | yes (for ladder to have a cost) |

## D. Defender depth (Phase D)

| ID | Check | Pass condition | Blocking |
|---|---|---|---|
| D-NAV-1 | Wall-position anchors | Authored stand points on the battlement reachable by Side==1 formations | yes |
| D-RANGE-1 | Ranged line of fire | Defender archers (existing `Ranged` role, reach 2600 VCS) have clear LOS down the approach from the wall | yes |
| D-COL-1 | Battlement melee space | Enough wall-top room for attacker+defender melee capsules to resolve | yes |

## E. Siege engines (Phase E)

### E1 — Battering ram (lowest risk)
| ID | Check | Pass condition | Blocking |
|---|---|---|---|
| E1-NAV-1 | Ram pathing | Multi-crew ram pathes to the measured gate base across the authored approach + known bridge gap | yes |
| E1-COL-1 | Ram collision | Ram body collides solidly; crew occupy it without clipping | yes |
| E1-ANIM-1 | Ram/crew animation | Push/strike animation present; rule (accepted gate damage) passes without it | no |

### E2 — Siege tower (highest risk, qualify LAST)
| ID | Check | Pass condition | Blocking |
|---|---|---|---|
| E2-NAV-1 | Mobile tower pathing | A wide tower pathes to a wall face without re-triggering the authored deck gap | yes |
| E2-COL-1 | Tower collision + dock | Tower docks to the wall; ramp-drop lands on C-NAV-1 wall-top | yes |
| E2-SOCKET-1 | Ramp landing transform | Deterministic dock/ramp transform measurable from authored geometry, fail-closed | yes |
| E2-ANIM-1 | Deploy/ramp animation | Present; rule traversal passes without it | no |
| E2-DEP-1 | Depends on wall-top | C-NAV-1 and D-NAV-1 already pass | yes |

### E3 — Catapult / Mangonel (owned asset, new projectile+segment model)
| ID | Check | Pass condition | Blocking |
|---|---|---|---|
| E3-ASSET-1 | Mangonel present | Owned **Medieval Mangonel – Catapult Siege Weapon** (VOA; owned, NOT local, NOT imported) is locally downloaded + imported, or rule works with a placeholder | yes (for the authored version) |
| E3-NAV-1 | Emplacement | Engine sits at an outer-field emplacement with clear arc to gate/wall | yes |
| E3-PROJ-1 | Projectile arc | Arc tuned to authored map scale; extends the existing projectile path (`URBCombatRangedComponent`/`ASoulBattleSpellProjectile`), not a new projectile authority | yes |
| E3-SEG-1 | Wall-segment damage | A gate hit reuses `ApplyGateDamage`; a wall-segment hit drives the new in-battle segment integrity (`ENGINE-WALLSEG`) | yes |
| E3-VFX-1 | Impact VFX | Present; rule correctness independent of VFX | no |

## F. Preparation transport + economy (Phase B1/F)

| ID | Check | Pass condition | Blocking |
|---|---|---|---|
| F-TRANS-1 | Prep transported | `FSoulSiegePreparation` reaches the arena via the descriptor and is passed to `FSoulSiegeRules::Begin(Prep)` (replacing `Begin({})` VCS at `SoulRealtimeBattleArena.cpp:1264`) | yes |
| F-REGRESS-1 | V0 regression | With empty preparation, behavior is byte-for-byte V0 (gate-only, courtyard, aftermath, F5/F9 exact restore) | yes |
| F-SAVE-1 | Prep round-trips | Preparation + any prep economy state survive F5/F9 and fresh-process restore, no new save domain/schema bump, malformed rejects atomically (mirror `SoulSiegeCampaignTests.cpp`) | yes |

## G. Repair (Phase G)

| ID | Check | Pass condition | Blocking |
|---|---|---|---|
| G-TIME-1 | Timed repair | `AdvanceDay` ticks repair jobs; integrity rises by exactly days×permille/day (clamped) | yes |
| G-COST-1 | Costed repair | `BeginRepair` reserves exact treasury/AP; insufficient funds reject atomically | yes |
| G-VIS-1 | Scaffolding shows | While repairing, `ApplyWallState(..., bRepairing=true)` drives the (currently dead) `RepairRoot` branch | yes |
| G-SCAR-1 | Scar heals visually | On completion the active breach scar clears visually while the historical scar remains | no |
| G-SEG-1 | Segment repair data | Authored map segments carry `RepairActors` so the scaffolding visual has geometry | no |

---

## Cross-cutting mandatory checks (apply to every mesh-bearing item)

1. **Nav:** Can a Soul combat capsule actually path the surface the mesh implies? (No nav → no feature.)
2. **Collision:** Is collision solid and non-clipping; does opening/closing actually gate movement?
3. **Socket / transform:** Is every attach/landing/deploy point a deterministic transform measured
   from authored actors at runtime, with a `SOUL_SIEGE_SETUP_FAIL`-style fail-closed if absent?
   (No invented `/Game` paths; resolve actors present in the loaded map.)
4. **Animation:** If an owned animation set is used, is it locally present AND retargeted to the RB
   rig? If not, the gameplay RULE must still pass with a non-animated fallback.
5. **Donor preservation:** No donor bytes, no map writes, no terrain writes — mirror V0's
   `native-donor-preservation.json` / `preservation.json` zero-mismatch discipline.
6. **Perf honesty:** Functional qualification at a capped FPS is NOT a sustained-performance claim
   (V0 explicitly separated 10-FPS functional from 30-FPS manual). State the cap in every receipt.

## The single highest-risk unknown

**C-NAV-1 (walkable battlement).** Ladders (C), siege tower (E2), and all wall-top defender depth (D)
are worthless if `L_HumanCapital_Authored` has no capsule-walkable wall-top. This one editor check
should be run **first**, before any ladder/tower/defender engineering begins. If it fails, the V1
plan collapses to the "ground-vector" subset: doorway (A) + ram (E1) + catapult-to-gate (E3 gate-only)
+ repair (G) + prep/AI (F) — and wall-top features wait for an authored battlement.
