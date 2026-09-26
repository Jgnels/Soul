# Soul Founder Overmap — Headless Traversal Check

Status: **PASS**

- Shortest approach distance: 3 region moves.
- Battle commitment: +1 AP from an adjacent approach.
- Equal-action shortest approaches: 3.

## Shortest approaches

- Human Capital -> Crossroads -> River Ford -> Orc Watch -> BATTLE — 4 actions, logistics 18, roads 3, chokepoints 1.
- Human Capital -> Crossroads -> Forest Edge -> North Pass -> BATTLE — 4 actions, logistics 22, roads 1, chokepoints 1.
- Human Capital -> Crossroads -> Forest Edge -> Orc Watch -> BATTLE — 4 actions, logistics 22, roads 1, chokepoints 0.

## Deliberate detours

- **quarry_resource_loop**: Human Capital -> Crossroads -> Old Quarry -> Crossroads -> River Ford -> Orc Watch -> BATTLE — 6 actions, logistics 30, ~2 days at 3 AP/day.
- **forest_resource_flank**: Human Capital -> Crossroads -> Forest Edge -> North Pass -> BATTLE — 4 actions, logistics 22, ~2 days at 3 AP/day.
- **shrine_exploration_loop**: Human Capital -> Crossroads -> Old Quarry -> Ancient Shrine -> Old Quarry -> Crossroads -> Forest Edge -> North Pass -> BATTLE — 8 actions, logistics 50, ~3 days at 3 AP/day.

## Fog/exploration

- Step 0 at Human Capital: reveals nothing new; explored total 2.
- Step 1 at Crossroads: reveals Forest Edge, Old Quarry, River Ford; explored total 5.
- Step 2 at River Ford: reveals Orc Watch; explored total 6.
- Step 3 at Orc Watch: reveals Orc Stronghold; explored total 7.

## Interpretation

- Three equal-action founder approaches prevent one mandatory attack lane.
- Road approach minimizes logistics cost; forest/pass routes trade efficiency for terrain.
- Old Quarry and Forest Edge are optional strategic resource decisions, not gates.
- Ancient Shrine is deliberately a deeper hero/magic progression detour.
- Stronghold becomes explored from either final approach before battle commitment.
