# Nature / Dark — Lighter Future Settlement Notes (NOT ACTIVATED)

Lane 05 — DWARF / ORC / VIKING SETTLEMENT DEVELOPMENT TREES
Status for everything in this file: **DESIGN PROPOSAL — DO NOT ACTIVATE.**

The lane requires "lighter future concepts for Nature/Dark without activating them." Per shared rules, Nature/Dark passivity must stay compatible with the current four-faction game (Human/Dwarf/Orc/Viking). These notes exist so that when Nature/Dark are eventually promoted, the building-tree authoring is already shaped to the proven system — but **no tree here is authored into a DataAsset, registry, or proof, and no building IDs below should be wired into recruitment, effects, or save.**

---

## Why "lighter"

- Nature and Dark are currently passive. They have no walkable settlement, no DevelopmentProof, no EnvironmentRegistry entry.
- `settlement_blueprints.json` marks `nature_candidate` as `"enabled": false` and `dark` as `"enabled": true` but with `ACQUIRED_PAYLOAD_PENDING_UE_CACHE` donors and no authored map.
- `FSoulCampaignRules::CanonicalFactions()` includes `nature` and `dark`, and `AdmittedStrategicUnit(nature) == nature_bear_warrior`, but **`AdmittedStrategicUnit(dark)` returns `NAME_None`** (Source/SoulCore/Private/SoulCampaign.cpp:9-17). Dark has no signature strategic unit at all in the current snapshot — a hard blocker for any recruit-building tree.

So these are **4-5 node skeleton sketches**, not full 11-12 node trees like the active factions, deliberately kept minimal.

---

## Nature — skeleton sketch (future)

- Faction id: `nature`. Concept (from blueprints): forest/tree settlement, canopy bridges, great tree.
- Signature unit present: `nature_bear_warrior` (AdmittedStrategicUnit). This is the ONLY nature unit grounded in current code.
- Suggested minimal node set (civic + 1 military + 1 economy + 1 defense):
  - `nature.great_tree` — civic root, `control.settlement` + `service.hero_services`. Timing: capstone-feel but is the seed (days 0 pre-built).
  - `nature.tribal_hall` — military (1 day normal), `recruitment.nature_bear_warrior` (only wired unit). RequiredBuildingId binding.
  - `nature.trade_clearing` — economy (1 day normal), `economy.market`.
  - `nature.root_barrier` — fortification (2 day significant), `siege.defense`. Living/regrowing walls theme (could reuse RepairWalls over days).
  - `nature.druid_circle` — magic (2 day significant), proposed Nature/Life school. DEFERRED.
- Deliberately omitted until promotion: centaur/animal/beast dwellings, Treefort canopy platforms (donor `TREEFORT_REQUIRED` / `ROSTER_PENDING` in blueprints).

## Dark — skeleton sketch (future)

- Faction id: `dark`. Concept: otherworldly dark fortress.
- **Blocker:** no signature strategic unit (`AdmittedStrategicUnit(dark) == NAME_None`). A Dark recruit-building tree cannot bind a single `RequiredBuildingId` to a real unit yet. Any Dark tree is therefore even lighter than Nature — civic/economy/defense only, no recruitment, until a Dark roster exists.
- Suggested minimal node set:
  - `dark.throne_sanctum` — civic root, control + hero services.
  - `dark.crypt_market` — economy (1 day), `economy.market`.
  - `dark.outer_works` — fortification (2 day), `siege.defense`. Can borrow Ravenhold damaged grammar.
  - `dark.ward_spire` — defense/magic ward (2 day), effect-only (no unit) — safest Dark node because it needs no roster.
- Explicitly DO NOT sketch recruit dwellings (black_guard_bastion, dread_gallery, demon_gate, etc.) as buildable until a Dark unit roster is admitted. Listing their IDs for continuity only.

---

## Design invariants to preserve for the future promotion

1. **Same system, no new authority.** Nature/Dark trees, when promoted, must author `USoulSettlementScenarioData` + a parallel `*Development.json` (schema=1) and pass `ValidateDefinition` — identical to Dwarf/Orc/Viking. No second settlement/save/combat authority.
2. **ID namespacing.** Keep `nature.*` / `dark.*` prefixes (consistent with human/dwarf/orc/viking) so faction association by id-namespace + `FSoulSettlementState.FactionId` keeps working.
3. **Civic root always first.** Each faction's control/hero-service building is the DAG root (human.keep, dwarf.kings_hall, orc.warchief_hall, viking.great_hall, nature.great_tree, dark.throne_sanctum).
4. **No recruit node without a wired unit.** A recruit building must bind to a real strategic unit id. Nature has exactly one (`nature_bear_warrior`); Dark has none. Respect that.
5. **Founder timing stays 1/2/3.** Normal/significant/capstone = 1/2/3 days.
6. **Passivity preserved.** These sketches do not add Nature/Dark to any start-state ownership, AI, or diplomacy. They are inert until a dedicated activation lane.

---

## Explicit non-activation checklist (for reviewers)

- [ ] No `NatureDevelopment.json` / `DarkDevelopment.json` authored by this lane. (Confirmed: none written.)
- [ ] No `USoulSettlementScenarioData` for nature/dark referenced. (Confirmed.)
- [ ] No EnvironmentRegistry entry added for nature/dark. (Confirmed — this lane writes only under its Evidence dir.)
- [ ] No recruitment/effect/save wiring for nature.*/dark.* IDs. (Confirmed.)
