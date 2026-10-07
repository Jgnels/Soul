# Soul 3.5 km composition study

Authority and acceptance are in `Evidence/ProductionWorldComposition-20261007/HANDOFF.md`.
This is an isolated editor candidate. It does not change campaign runtime bindings.

The analytical sequence is `compose_plan.py`, `analyze_lakes.py`, `fit_geography.py`,
`fit_corridors.py`. Crossing selection was reviewed and frozen in
`selected-crossing-sites-r3.json`; do not regenerate that selection silently.
`measure_bank_spans.py` is analysis, not an automatic authority to move bridges.
`fit_bridge_approaches.py` and `repair_local_route_grades.py` operate on presentation
paths only. The latter uses the final channel terrain and measured bridge deck
planes; it does not qualify the curved mesh surface or gameplay movement.

Use the bundled Python runtime with NumPy and Pillow. Analytical tools write local
licensed-derived data under `Data/CampaignCompositionLocal` and the evidence
directory. They must not be run over a later accepted revision without changing
output revisions. The final display uses `route-local-repair-study-r7.json` and
`Composition_Control_r5.png`. The expanded-window r8 experiment made no further
repair and was not applied.

UE scripts run in the guarded live editor, one at a time. They assert candidate
map ownership and never save donors. They are recorded authoring steps, not an
idempotent one-command production pipeline. `save_map` creates a copy without
switching editor worlds: explicitly load the saved candidate before subsequent
mutation. Do not run placement scripts twice; duplicate placement is rejected.

Material setup had two rejected iterations. The local historical
`prepare_review_material.py` remains uncommitted and should not be replayed.
The successful instance was created fresh with Nwiro's dedicated material-instance
tool, renamed into the candidate namespace, then given the original Mountain05
scalar/vector/texture parameters. Reusing the cached duplicated instance produced
a checkerboard. Current parent is `M_Composition_Review_r3`, instance
`MI_Composition_Review_r4`, control texture `Controls/T_Composition_Control_r5`.
New texture import uses Unreal Interchange. Shared samplers keep the parent at five
samplers. Final paths and changes are in the evidence receipts.

Water meshes come from measured downhill centerlines and clipped native basin
triangle planes. They are deterministic geometry, not generated art. The final
lake/river mesh assets have `_r2` suffixes. `refine_lake_edges.py` skips already
created r2 assets on resume. Do not interpret it as a general asset replacement tool.

Validation: `validate_composition.py` checks canonical IDs/pairs, water profiles,
post-channel road/deck grades, syntax and preservation hashes. It reports the
remaining River Ford approach failure rather than treating it as a pass. Native
collision and screenshots have separate receipts. Existing gameplay automation
runs against the pre-existing editor binary, not this candidate's 36-site layout.
