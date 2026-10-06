# Settlement development controls audit

**CONTROLS_PASS** — development-controls-r1 exited cleanly with code 0. All 24 paced steps 0–23, 116 successful runtime checks, 20 input actions, two F5 saves and two F9 loads are present. No Error/Fatal/assertion/controls-failure markers were found.

This is functional UI/state/persistence evidence only: **authored_environment=0; miniature=0; battle_environment=0**. It is not art acceptance or a performance benchmark.

## State and persistence

| Saved checkpoint | Day | Gold | Hired | Tavern level | Days remaining | Condition |
|---|---:|---:|---|---:|---:|---|
| start | 1 | 3000 | False | 0 | 0 | Unbuilt |
| mid | 2 | 3250 | False | 0 | 1 | Building |
| completed_hired | 3 | 2500 | True | 1 | 0 | Intact |

The 200-gold build and 1200-gold hire occurred once each. Gold reconciles as 3000−200+2×450−1200=2500. Hiring was blocked before construction and after mid-construction rollback. Duplicate rendered BuildTavern/Hire clicks produced the actual rejection messages and left both domains unchanged. All four other starting buildings remained unchanged.

Both saves logged Soul.Campaign and Soul.Settlements scope and successful completion; the qualifier required actual checkpoint-byte changes. Both loads logged success and presentation revisions 1/2, then passed exact two-domain equality and exact load/development revision assertions. The six persisted JSON files are expected checkpoint pairs; separate post-load JSON files were not written, so restoration equality is supported by the frozen source predicates and successful runtime checks.

## Input and capture evidence

The frozen qualifier routes actions through PC InputKey and the two duplicate actions through actual HUD hitboxes. Exact input counts are listed below.

`{"F5": 2, "F9": 2, "H": 3, "LeftMouseButton": 2, "SpaceBar": 4, "T": 6, "U": 1}`

All six PNGs have valid 1920×1080 headers and matching completion log entries: start, mid, completed, loaded, hired, loaded_completed. Completed capture finished before the first F9. Root performs actual image inspection.

## Resource observations

| Metric | Peak |
|---|---:|
| peak_gpu_temperature_c | 58.0 |
| peak_gpu_utilization_percent | 48.0 |
| peak_device_vram_used_mib | 5111.0 |
| peak_sampled_process_working_set_mib | 3801.66 |
| peak_process_reported_working_set_mib | 3812.36 |
| peak_process_private_commit_mib | 4919.09 |

105 telemetry samples span full startup through shutdown. VRAM is whole-device usage; process RAM reports sampled working set, OS peak working set, and private commit separately. The run was capped at 20 FPS with a 20-second functional warmup. No performance acceptance is claimed.

## Integrity and limits

All 29 frozen source/payload hashes match, including qualifier revision2, integration sources, proof DA, retained map/presentation/height, and loaded DLL. Exact hashes, every check/input, all screenshots, snapshots, save files and telemetry are in [controls-runtime-audit.json](controls-runtime-audit.json).

There were 25 Warning lines, preserved by category in JSON; no error markers. The MaxFPS priority warning retained the required 20 value. The final async busy flag was not sampled true, but current callback success/revision/byte-equality guards passed. The proof DA has no bound authored environment; complete capital recovery and any scene/miniature/battle-environment acceptance remain separate.
