# Soul World Overmap Approach Handoff - 2026-09-22

## Purpose

Make strategic geography causally affect the battle that follows it without moving battle authority into the map presentation layer.

The world graph already owns travel topology, roads, chokepoints, biome, landform and destination battlefield hints. This layer converts each undirected strategic edge into two deterministic directed approach profiles so entering the same region from different directions remains distinguishable.

Data:
- `Data/soul_overmap_approach_profiles_v1_20260922.json`
- `Evidence/WorldOvermap/founder_approach_matrix.md`
- `Evidence/WorldOvermap/approach_profile_validation.json`

## Design-reference synthesis

- Mount & Blade II: Bannerlord is useful as a reference for a campaign world that feels physically traversed rather than selected from a menu.
- Total War: Warhammer is useful as a reference for making roads, terrain, settlement approaches and chokepoints legible before battle.
- Heroes III is useful as a reference for compact adventure-map decisions where a site, resource or route visibly matters.

Soul does not copy their maps or runtime logic. Its handoff remains deterministic Soul data feeding its own hidden-hex tactical layer.
## Directed profile contract

Every strategic edge exports both directions. Each profile carries:
- source and destination stable region IDs;
- route type, road flag, chokepoint flag, AP cost and logistics cost;
- destination entry direction derived from the runtime import;
- destination biome, landform, feature and elevation band;
- settlement/resource transition type where relevant;
- destination battlefield recipe ID and current qualification status;
- normalized approach tags suitable for later UE import or debugging.

Weather and time are deliberately absent from authored values. `battlefield_recipes.json` requires them as strategic context, so runtime must inject the current values when battle is committed.

The profile does not decide combat, modify AP, infer movement legality, or choose a different battlefield recipe. Those remain with the existing world graph, strategy mechanics and battle data.

## Founder slice consequences

The nine-region Human-Orc slice now has 20 directed internal approaches across its 10 links. That preserves meaningful differences such as:
- road entry through River Ford toward Orc Watch versus the woodland trail from Forest Edge;
- the trail into North Pass versus the pass descent from North Pass into the Orc Stronghold;
- a resource-site arrival at Old Quarry versus a landmark arrival at Ancient Shrine.

This gives the future UE lane explicit inputs for camera entry, road alignment, deployment-side selection, dressing, and battlefield-context debugging without inventing geography in Blueprint.
