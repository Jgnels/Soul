# Soul Founder Overmap UE Staging — 2026-09-22

## Purpose

Provide flat, deterministic staging files for the eventual bounded Unreal founder-slice proof.

These files are **not a new gameplay authority**. SoulCore and the richer source JSON contracts remain authoritative. The CSV layer exists only to make Unreal DataTable/import work straightforward and inspectable.

Exporter:
- `Tools/export_soul_founder_ue_staging.py`

Validator:
- `Tools/validate_soul_founder_ue_staging.py`
## Staging files

Under `Data/UEImport/`:

- `soul_founder_regions_v1_20260922.csv` — 9 region rows.
- `soul_founder_routes_v1_20260922.csv` — 10 route/spline rows.
- `soul_founder_approaches_v1_20260922.csv` — 20 directed approach rows.
- `soul_founder_presentation_states_v1_20260922.csv` — 30 corridor/detour QA states.
- `soul_founder_ue_import_manifest_v1_20260922.json` — source/output hashes and counts.

Every CSV uses stable `Name` as its first column for Unreal DataTable-style row identity.
## Import order

1. Regions: position, selection radius, starting fog/ownership, anchor and battlefield identity.
2. Routes: endpoints, semantic route class, AP/logistics metadata, three-point spline and visual width/family.
3. Directed approaches: entry direction and battlefield handoff context.
4. Presentation states: expected fog, memory, selection and battle-commit QA fixtures.

Pipe-delimited values inside a field represent arrays of stable IDs. They are staging encodings only; do not make gameplay parse these CSV strings at runtime if the canonical structures are already available.
## RB / Soul authority

- SoulCore owns campaign state, movement legality, AP, ownership, fog/exploration and battle commitment.
- RB Weather owns dynamic weather.
- RB Optimization remains the first optimization authority for the strategic presentation.
- RB Save owns persisted campaign state.
- The Unreal presentation consumes these rows; it does not infer missing gameplay state.

The final battle-commit state rows carry the exact directed approach and battlefield recipe but leave weather/time dynamic.
## Acceptance

The validator checks:
- source hashes and generated CSV hashes;
- exact row counts and stable unique DataTable names;
- region, route and directed-approach referential integrity;
- route spline endpoint presence;
- planned route adjacency in every presentation state;
- target visibility before every planned move/commit;
- battle-only fields appear only on battle-commit rows.

This keeps the UE handoff flat and convenient without weakening the deterministic source contracts.
