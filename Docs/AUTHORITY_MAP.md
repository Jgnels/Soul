# Soul authority map

## RB-first authorities
- RB Foundation: composition, capability/authority registration, canonical cross-domain orchestration.
- RB Save: durable storage, generations, repair/migration mechanics.
- RB Item Economy: item/equipment/artifact inventory truth.
- RB Routine: persistent civilians/settlement people and offscreen life simulation when used.
- RB Weather: weather state and weather presentation authority.
- RB Optimization: representation/performance state.
- RefinedBadger Combat: installed foundation combat capability; Soul must not silently duplicate its physical-combat authority.

## Soul-owned domains
- Soul Campaign: regions, day/action budget, strategic ownership, recruitment pools, faction strategic resources.
- Soul Hex: deterministic tactical topology, terrain cell metadata, footprints, path legality.
- Soul Initiative: turn ordering, wait/defend/retaliation bookkeeping, morale/luck hooks.
- Soul Regiment: combat-group identity, counts, veterancy/rank, persistent battle record.
- Soul Commander Memory: subjective commander memories and derived strategic context.
- Soul Strategy AI: campaign-level candidate generation/scoring/explanation. This is intentionally separate from low-level embodied RB AI.
- Soul Siege: preparation, siege layers, objectives, breaches, fallback state.
- Soul Battlefield Recipe: strategic geography -> authored tactical template/context.

## AI boundary
Do not install RB AI merely for uniformity. Its tactical/creature decision layer may be useful later, but Living Strategy's campaign AI already has stronger strategic reasoning and memory-aware scoring. If RB AI is added later, its authority must be narrowed to an explicit embodied/tactical seam so it cannot collide with Soul Strategy AI.
