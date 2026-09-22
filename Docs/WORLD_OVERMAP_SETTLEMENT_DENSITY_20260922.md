# Soul World Overmap Settlement Density — 2026-09-22

## Goal

Keep the 36-region world geographically readable while avoiding a campaign map where only six distant capitals matter.

The density layer adds **candidate settlement roles to existing strategic regions**. It does not add graph nodes or lock a full campaign-start state.

## Reference synthesis

- Bannerlord: towns, castles and villages make the physical world feel inhabited between major faction seats.
- Total War: major and minor settlements distribute strategic pressure without requiring every location to be a capital.
- Heroes III: towns are major anchors, while useful adventure-map sites make the space between them worth traversing.

Soul keeps its own persistent physical-city and region-state architecture underneath those references.
## Major seats — locked identity

The six current major seats remain:
- Human Capital
- Viking Harbour
- Dwarf Hold
- Orc Stronghold
- Nature Treehold
- Dark Fortress

These bind to the current city/siege blueprints and target full visitable scenes.

## Minor settlement candidates

Eight existing regions receive lightweight settlement candidates:
- Crossroads — Human market town
- Northwest March — neutral trade post
- Viking Forest Track — village
- Dwarf Forge Approach — forge outpost
- Orc War Camp — war-camp settlement
- Nature Forest Clearing — grove village
- Dark Castle Approach — ward bastion
- Coastal Ruins — neutral ruined settlement
## Production rule

Minor sites should normally be **bounded scenes or strategic proxies**, not capital-scale city builds.

They may provide a small service bundle such as:
- market / caravan / rest;
- light recruitment;
- repair / supplies;
- healing;
- exploration / salvage;
- intelligence / warding.

Use existing environment donors and the global Soul rule of interactive space + static/non-interactive clutter.
A minor settlement does not justify bespoke simulation of decorative props.

## Authority guardrail

The eight minor sites are `CANDIDATE_NONCANONICAL`.
They can be removed, renamed, moved between minor roles, or reduced to proxy-only presentation without changing the world topology.

Data:
- `Data/soul_overmap_settlement_slots_v1_20260922.json`

Validation:
- `Evidence/WorldOvermap/settlement_slot_validation.json`
