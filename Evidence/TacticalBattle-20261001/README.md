# Tactical battle qualification — 2026-10-01

## Fact

- `SoulEditor Win64 Development` built successfully from the worker tree. The build receipt is `build-editor-qualified.log`.
- `Soul.RealtimeBattle` passed 24/24 tests with zero warnings or skips. The machine-readable report is `Automation-Realtime-Qualified/index.json`.
- The full `Soul.` suite passed 74/74 tests with zero warnings, skips, or failures. The machine-readable report is `Automation-Soul-Qualified/index.json`.
- The licensed `/Game/Dragon_graveyard/Level/L_showcase_level` ran with 35 active combatants per side plus 65 reserves per side.
- Runtime created 70 actors across 10 formations and exercised Approach, Maneuver, and Rout/Rally phases.
- Firebolt, Chain Lightning, Blizzard, Tidal Ward, and Tailwind all produced accepted RB Magic casts. Paragon fallback particles were used for the four non-fire presentations.
- The enemy received three physical reinforcement waves from battlefield edges.
- Morale resolved the battle at 20.79 simulated seconds: victory, 91 player survivors, 53 hostile strategic reserves preserved, 320 authoritative contacts, and 42/42 PBIL queries successful.
- The qualification wrapper observed `SOUL_RT_ARENA_PASS`, exit code 0, a clean D3D11 shutdown, and no crash signature. See `Runtime/dragon-35x35-qualified/summary.json`.

## Inspected evidence

- `battle-approach.png`: ten separated formations, Griffin and dragon silhouettes, low-profile beasts, five-spell command HUD.
- `battle-result.png`: routed formations, explicit Rout/Rally state, reserve-aware victory result.

## Limitations / unknown

- A Barghest is the current low-profile crawler proxy because no owned Bracken asset was found on disk.
- Aerial creatures use a hovering visual mesh over ground-authoritative tactical navigation; free three-dimensional flight is not implemented.
- Stone Sentinel is defined but intentionally not player-exposed until summoned bodies are excluded explicitly from the strategic survivor ledger.
- This qualification validates the physical battle and campaign result rules through automation. It does not claim a fresh rendered campaign-to-battle-to-campaign UI traversal in this receipt.
