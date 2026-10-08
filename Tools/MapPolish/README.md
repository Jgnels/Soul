# Frozen-geography transport polish

Offline scripts operate on the existing imported uint16 Landscape, never write its
heights. Live scripts assert the candidate map and save only Soul-owned presentation
assets. Evidence authority: `Evidence/MapPolish-20261007/HANDOFF.md`.

The final on-disk candidate is already saved. **Do not blindly rerun all scripts**:
some are iteration receipts and rejected bounded experiments. Donor payloads,
native data and intermediate coordinates are intentionally local-only.

Final checks can be repeated with the bundled Python runtime:

```
python Tools/MapPolish/route_atlas.py --input routes-presentation-r4.json
python Tools/MapPolish/prepare_native_routes.py --input routes-presentation-r4.json
python Tools/MapPolish/summarize_native.py
```

Between prepare/summarize, execute `qualify_native_routes.py` in the live candidate
with `QUALIFY_REVISION='final'`; wait for its callback to finish. Native inputs are
1 m road stations with 0.25 m crossing stations. Independent analytical validation
includes each polyline vertex at <=0.25 m. Sampled centerline grades do not prove
turning radii, road-width clearance or gameplay integration.

Final route chain:
ford repair → fine grade repairs → equivalent-corridor consolidation → bounded
corner rounding → shore landing fit → Human arrivals → Southern/Broken bridge axis
alignment → verified exception repairs → micro-curves r5/r6 → control r4.
The final assets use `M_Polish_r1`, `MI_Polish_r1`, `T_Polish_Control_r4` and owned
collision/ruined-bridge derivatives under the candidate's `Polish` folder.

`apply_final_control.py` applies the final road channel. `capture_review.py` captures
fixed before/after views under the existing 85°C guard and 12 FPS review cap.
`build_review.py` consolidates screenshots and completed receipts. The three extra
crossing views are documented in the final capture receipt. Avoid simultaneous
capture and automation callbacks: tests can redirect the viewport.

Read the final handoff before any future run. Preserve all prior snapshots and
donors. Do not use these presentation scripts as campaign/save/topology authority.
