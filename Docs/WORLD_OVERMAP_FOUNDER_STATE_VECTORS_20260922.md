# Soul Founder Overmap Presentation State Vectors — 2026-09-22

## Purpose

Give the bounded Unreal founder-slice proof exact presentation QA fixtures instead of relying on screenshots or subjective recollection.

Primary artifact:
- `Data/soul_founder_presentation_state_vectors_v1_20260922.json`

Builder / validator:
- `Tools/build_soul_founder_presentation_state_vectors.py`
- `Tools/validate_soul_founder_presentation_state_vectors.py`

This is **presentation QA data, not campaign authority**. SoulCore still owns movement, fog/exploration, AP, ownership, battle commitment and AI state.
## Coverage

The fixture exercises:
- all three equal-action Human-to-Orc attack corridors;
- the quarry resource loop;
- the forest resource flank;
- the Ancient Shrine exploration loop;
- 30 total presentation states;
- all eight founder regions that can be active before the Stronghold battle;
- Orc Stronghold as the visible final battle destination.

The two shortest-path branch decisions remain explicit:
- Crossroads -> River Ford or Forest Edge;
- Forest Edge -> North Pass or Orc Watch.
## Fog / information contract

For each state the fixture records:
- active region and strategic-camera focus;
- visible regions;
- explored-but-not-visible terrain memory;
- unexplored regions;
- regions allowed to expose current dynamic state;
- remembered physical routes;
- currently selectable routes and destinations;
- the planned next route/action.

Visible regions may show current armies, settlement condition and interactables.
Explored-but-not-visible regions preserve terrain/anchor memory but must not leak current dynamic state.
Unexplored regions remain obscured.
## Battle commitment

The final state of every corridor or detour stops at a region adjacent to Orc Stronghold.
The next action is `battle_commit`, not movement into the Stronghold node.

The fixture carries the exact directed approach:
- route ID and route type;
- entry direction;
- road/chokepoint flags;
- battlefield recipe ID/status;
- dynamic weather/time requirements.

This lets the UE proof test the campaign-to-battle handshake without inventing approach context in Blueprint.
## High-value visual checks

At Human Capital only Human Capital and Crossroads are initially visible.
At Crossroads the player can read Forest Edge, Old Quarry and River Ford as new choices without exposing the rest of the map.
On the road approach, Orc Stronghold becomes visible from Orc Watch before battle commitment.
On the pass approach, Orc Stronghold becomes visible from North Pass before battle commitment.
The shrine loop must preserve previously explored terrain memory when the party backtracks to Old Quarry and Crossroads.

The fixture should be consumed as expected-state data in the UE presentation proof; it should not become a second fog or movement implementation.
