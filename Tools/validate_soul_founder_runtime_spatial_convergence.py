"""Validate the founder runtime spatial convergence migration analysis snapshot."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANDIDATE = Path(r"D:\RefinedBadger\Games\Soul\Source\Soul\Private\SoulFounderPlaytestCampaignActor.cpp")
ANALYSIS = ROOT / "Evidence" / "WorldOvermap" / "founder_runtime_spatial_convergence.json"
OUT = ROOT / "Evidence" / "WorldOvermap" / "founder_runtime_spatial_convergence_validation.json"

analysis = json.loads(ANALYSIS.read_text(encoding="utf-8"))
errors: list[str] = []
warnings: list[str] = []

if analysis.get("status") != "READ_ONLY_SPATIAL_CONVERGENCE_ANALYSIS":
    errors.append("spatial analysis status mismatch")
if analysis.get("authority", {}).get("candidate_authority") != "UNTRACKED_DIRTY_PRIMARY_PROTOTYPE_NOT_CANONICAL":
    errors.append("candidate authority boundary missing")
if analysis.get("fit", {}).get("model") != "reflected_2d_similarity":
    errors.append("unexpected spatial fit model")
if analysis.get("fit", {}).get("reflection") is not True:
    errors.append("expected reflected founder-layout relationship")

if len(analysis.get("regions", [])) != 9:
    errors.append("expected 9 spatially compared founder regions")
if len(analysis.get("routes", [])) != 10:
    errors.append("expected 10 spatially compared founder routes")

fit = analysis.get("fit", {})
rms_pct = float(fit.get("rms_error_pct_world_span", 999))
max_pct = float(fit.get("max_error_pct_world_span", 999))
if rms_pct >= 4.0:
    errors.append(f"candidate layout no longer close enough for low-cost migration: RMS {rms_pct:.3f}%")
if max_pct >= 6.0:
    errors.append(f"candidate layout max region error exceeds migration envelope: {max_pct:.3f}%")

route_error = analysis.get("route_error", {})
max_route_error = float(route_error.get("max_relative_length_error_pct", 999))
if max_route_error >= 20.0:
    errors.append(f"route-length distortion exceeds migration envelope: {max_route_error:.3f}%")
elif max_route_error >= 10.0:
    warnings.append(
        f"route lengths are not exact ({max_route_error:.3f}% worst); accepted v2 route geometry must replace candidate lines"
    )

recommendation = analysis.get("migration_recommendation", {})
replace = set(recommendation.get("replace_from_import_bundle", []))
for required in (
    "hardcoded RegionPositions coordinates",
    "hardcoded connection presentation geometry",
    "region biome/landform/feature metadata",
):
    if required not in replace:
        errors.append(f"migration recommendation lost required replacement: {required}")
if recommendation.get("do_not_make_canonical") != "best-fit transform; it is migration evidence only":
    errors.append("best-fit transform is not clearly marked noncanonical")

if not CANDIDATE.exists():
    errors.append("candidate source disappeared")
else:
    current_hash = hashlib.sha256(CANDIDATE.read_bytes()).hexdigest()
    if current_hash != analysis.get("candidate_sha256"):
        warnings.append("candidate layout source changed since spatial analysis; rerun before using migration numbers")

result = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "pass" if not errors else "fail",
    "migration_shape_confirmed": not errors,
    "rms_error_pct_world_span": rms_pct,
    "max_error_pct_world_span": max_pct,
    "max_route_relative_error_pct": max_route_error,
    "candidate_snapshot_unchanged": not any("changed since" in x for x in warnings),
    "master_gate_eligible": False,
    "warnings": warnings,
    "errors": errors,
}
OUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
print(json.dumps(result, indent=2))
if errors:
    raise SystemExit(1)
