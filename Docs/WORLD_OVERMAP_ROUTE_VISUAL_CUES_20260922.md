# Soul World Overmap Route Visual Cues — 2026-09-22

## Purpose

Preserve the strategic meaning already encoded in Soul's 15 route classes when the hidden graph becomes a continuous 3D campaign map.

The runtime import previously exposed 51 correct splines but reduced their presentation to only two broad styles: road and trail. That is adequate for debugging, not for a map where a mountain pass, causeway, trade road, gully track and ordinary foot trail are supposed to inform route choice before selection.

This layer is presentation-only. It does not alter adjacency, AP cost, logistics cost, movement legality, chokepoint state, or route spline geometry.

Data:
- `Data/soul_overmap_route_visual_cues_v1_20260922.json`

Validation:
- `Evidence/WorldOvermap/route_visual_cue_validation.json`
- `Evidence/WorldOvermap/route_visual_cue_validation.md`
## Design-reference synthesis

- Heroes III: roads/trails should be readable at a glance because movement choice is gameplay, not hidden decoration.
- Total War: Warhammer: passes, bridges and constrained approaches should visually announce strategic gates.
- Bannerlord: roads should feel embedded in terrain and settlements rather than drawn as board-game connectors.

Soul keeps its own topology and costs. The references only inform communication quality.

## Route-language rules

Each canonical route class receives a distinct render family and physical vocabulary. Examples include coastal roads, engineered old Dwarf roads, river trails, meadow trails, high passes and stone causeways.

Selected-route feedback brightens or edge-highlights the existing physical route. Permanent giant path lines remain forbidden.

Every chokepoint route also requires a physical constraint cue—rock walls, bridge masonry, ravine edge, pass gate, or equivalent—so its strategic consequence is visible before the player commits movement.
## Founder-slice proof

The three shortest Human-to-Orc approaches remain equal in action count but now have different visual signatures:
- Road/Ford/Watch: road -> road -> road -> road.
- Forest/Pass: road -> trail -> trail -> mountain pass.
- Forest/Watch: road -> trail -> trail -> road.

This preserves the intended tradeoff: the road route reads efficient and maintained; the forest routes read less developed; North Pass becomes visibly distinct from the woodland approach before combat.

## Qualification

The generated layer covers all 51 routes, all 15 semantic classes, all 11 chokepoint routes, and all 10 founder-slice links. Validation also mirrors the canonical route class, road/chokepoint flags, widths, AP costs and logistics costs so presentation cannot silently become a second movement authority.

The remaining UE proof is visual rather than architectural: terrain blending, camera-distance readability, fog/weather legibility, and whether the cues survive RB Optimization/HLOD without becoming noisy.
