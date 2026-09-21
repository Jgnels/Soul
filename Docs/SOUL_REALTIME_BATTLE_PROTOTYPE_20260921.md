# Soul Real-Time Fantasy Battle Prototype — 2026-09-21

## Objective
Prove that Soul can replace turn-based tactical execution with bounded real-time
fantasy battles while preserving Heroes-style campaign structure and Soul's
persistent regiment/commander consequences.

This worker is isolated from the founder-playtest lane:
- worktree: D:\RefinedBadger\Worktrees\Soul-realtime-battle-20260921
- branch: astra/soul-realtime-battle-prototype-20260921
- base: d1d8659
- no canonical merge or production replacement is authorized here.

## Existing evidence
RB Combat already has a packaged four-group / 64-fighter physical proof.
The retained acceptance recorded 3,887 frames over 30.001 seconds, 129.6 mean
FPS, p95 tick delta 13.22 ms, 60 injuries and 54 casualties in its simple
development arena. This does not qualify Soul terrain, art, magic or shipping
performance, but it removes the need to invent group combat before prototyping.

## Prototype battle
First target:
- 24 active bodies per side, then 32 per side if the first gate passes.
- 4–6 RB Combat groups per side; never exceed RB Combat's 20-member group cap.
- hero present as a directly controllable combatant;
- melee line, shock/heavy, ranged, support and one apex representation;
- reserves remain strategic/regiment strength rather than hidden individual NPCs;
- reinforcement waves enter as formations, not one replacement per death;
- default wave size 8 and active-population cap 32 per side.

Small fights deploy all combatants. Reinforcements are a scale valve, not a rule
that forces every encounter into waves.

## Reinforcement gate
A side becomes reinforcement-eligible when active bodies fall below 70% of its
active cap and living reserves remain. A wave may restore at most 8 bodies and
may never exceed the active cap. Empty formations are prioritized so roster
variety remains visible instead of one cheap infantry type consuming every slot.

## Fantasy power model
Physical headcount and strategic power are intentionally decoupled. An apex
creature or hero may carry several ordinary soldiers' combat power while using
one Actor slot. This is how Soul should look larger than its simultaneous body
count.

The first rules module now records PowerPerBody separately from ActiveCount and
ReserveCount.

## Terrain magic
Battlefield metadata may provide school multipliers. Initial volcanic profile:
- Fire: 120%
- Ice: 80%
- Water: 85%
- Air/Earth/Electric: unchanged unless another modifier applies.

Keep environmental bonuses bounded. They should change spell choice, not decide
a battle before it begins.

## Legendary regiment champion (later gate)
Do not make every rank-and-file soldier persistent. When a regiment reaches its
maximum veterancy rank, it may unlock one Champion identity:
- persistent name/portrait tied to the regiment;
- one signature trait or active ability;
- one small command/aura bonus;
- optional limited equipment;
- no independent campaign movement, full hero skill tree, diplomacy or commander
  memory.
This creates "hero-lite" emergence without turning every soldier into an NPC sim.

## TCAT seam
Tactical Crowd AI Toolkit is optional for the baseline and desirable for the A/B
scale test. It should supply spatial influence/navigation guidance only:
formation pressure, danger, congestion, attraction/avoidance and spell hazards.
Soul retains battle/formation intent; RB Combat retains physical combat
execution. TCAT must not become a second strategic AI authority.

The toolkit is owned but was not found in the UE 5.8 Marketplace plugins or the
local Epic VaultCache locations checked on the upstairs machine. Install/stage it
before the TCAT A/B gate.

## Acceptance
1. Baseline real-time fight completes without allied targeting or deadlock.
2. Reinforcement waves enter as intact groups and respect active caps.
3. Hero can participate without owning rank-and-file persistence.
4. Ranged and melee groups both produce accepted physical consequences.
5. At least Firebolt, Chain Lightning and Blizzard can be layered onto the fight.
6. Volcanic metadata visibly changes Fire/Ice/Water effectiveness.
7. One apex unit participates without requiring general free-flight AI.
8. 24v24 passes on a Soul battlefield; if healthy, repeat at 32v32.
9. Capture game-thread/frame-time, stuck-agent, nav failure and spell-cost evidence.
10. TCAT-off versus TCAT-on comparison is measured once the plugin is installed.
