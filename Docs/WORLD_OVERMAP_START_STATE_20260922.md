# Soul World Overmap Start-State Layer - 2026-09-22

## Purpose

Separate **world geography** from **scenario ownership**.

The structural overmap describes where places are, how they connect, and which faction identity they visually belong near.
This start-state layer answers a different question: who actually controls what when a campaign begins?

Two scenarios are preserved independently:
- the current Human-vs-Orc founder micro-campaign;
- a noncanonical six-faction sandbox candidate for broad-map testing.

## Founder micro-campaign

The current prototype remains unchanged:
- Humans: Human Capital + Crossroads.
- Orcs: Orc Watch + Orc Stronghold.
- Other founder-slice regions begin neutral.
- Player hero begins at Human Capital.
- Current founder fog/visibility seed remains authoritative for this slice.
## Six-faction sandbox candidate

Each faction begins with:
- one major faction seat;
- one directly adjacent aligned minor settlement;
- one starting field army at the capital;
- one secondary garrison at the minor settlement.

Candidate pairs:
- Humans: Human Capital + Crossroads.
- Vikings: Viking Harbour + Forest Track.
- Dwarves: Dwarf Hold + Forge Approach.
- Orcs: Orc Stronghold + War Camp.
- Nature: Nature Treehold + Forest Clearing.
- Dark: Dark Fortress + Castle Approach.

The remaining 24 strategic regions begin neutral in this candidate.
Northwest March and Coastal Ruins remain neutral minor-settlement opportunities.
## Opening pressure result

The candidate intentionally does **not** force equal frontier width.

Immediate neutral choices:
- Humans: 5.
- Vikings: 3.
- Dwarves: 3.
- Orcs: 3.
- Nature: 2.
- Dark: 2.

Interpretation:
- Humans are central, flexible and exposed.
- Orcs and Dwarves share an early mountain/badlands pressure zone.
- Vikings have coast, forest and snow-pass choices.
- Nature is sheltered behind forest/river geography.
- Dark is the most geographically insulated opening.

That asymmetry is a map feature to playtest, not a locked balance claim.
Numeric army strength, garrison strength and economy values remain owned by the balance lane.
## Value-site candidates

Opening value sites are thematic only; they do not invent economy resource IDs.

Examples:
- Old Quarry - ore quarry.
- Forest Edge - timber/woodland.
- Viking Fjord Ridge - fishing/coastal trade.
- Dwarf High Quarry - ore.
- Orc Badlands - beast/salvage grounds.
- Nature River Woodland - herbs/food/river resources.
- Dark Ash Plain - occult/arcane site.

All remain `BALANCE_PENDING`.

## Required runtime rule

A campaign orchestrator must stop movement on hostile-controlled territory for encounter/battle resolution before occupation or onward traversal.
The current low-level graph only knows adjacency; it should not silently allow an army to walk through an enemy settlement.

## Artifacts

- `Data/soul_campaign_start_states_v1_20260922.json`
- `Evidence/WorldOvermap/start_state_validation.json`
- `Evidence/WorldOvermap/campaign_opening_analysis.json`
- `Evidence/WorldOvermap/campaign_opening_analysis.md`
