# Soul Overmap Presentation Contract — 2026-09-22

## Target

A continuous compressed 3D campaign world over Soul's deterministic hidden region graph.
The final player should see terrain, roads, rivers, passes, forests, settlements and moving parties — not a province-node editor.

## Reference synthesis

- Heroes III: readable roads, resources, landmarks, fog/exploration and hero movement choices.
- Total War: Warhammer: terrain-constrained fronts, readable settlement landmarks and strategic chokepoints.
- Bannerlord: continuous physical geography, moving parties, and settlements embedded in terrain.

Soul keeps its own deterministic region/state rules underneath that presentation.

## Locked presentation rules

- Region nodes and province borders are hidden by default.
- Roads/trails are physical geography; route highlight appears only during selection/planning.
- Settlement proxies resemble their actual visitable scenes and surface canonical damage/repair state.
- Hero/army representation is party-level, not hundreds of campaign-map soldiers.
- Explored terrain memory and current visibility remain distinct.
- RB Weather / RB Optimization / RB Save retain authority for their domains.

## Founder-slice visual gate

- nine founder regions use current exact IDs and UE blockout positions.
- three distinct short approaches to Orc Stronghold remain legible.
- Old Quarry and Forest Edge read as resource opportunities.
- River Ford and North Pass visibly read as crossing/chokepoint geography.
- Human Capital and Orc Stronghold are recognizable from their visitable-city silhouettes.
- fog, AP spending, region selection and AI movement remain driven by existing canonical state.

## Battle handshake

The overmap must visibly promise the same biome, landform, feature, approach, road, settlement, weather and siege context used to choose the tactical battlefield.
If the campaign map shows a bridge, pass, river or forest approach, the battle may vary but may not contradict it.

## Performance

Use simplified settlement proxies, party-level armies, HLOD/instancing and RB Optimization before bespoke representation work.
The full visitable settlement and its micro-detail are not kept live on the strategic map.
