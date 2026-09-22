"""Read-only spatial convergence analysis: dirty founder runtime layout -> accepted v2 layout."""
from __future__ import annotations

import hashlib
import json
import math
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANDIDATE = Path(r"D:\RefinedBadger\Games\Soul")
CANDIDATE_CPP = CANDIDATE / "Source/Soul/Private/SoulFounderPlaytestCampaignActor.cpp"
BUNDLE = ROOT / "Data" / "UEImport" / "soul_founder_import_bundle_v2_20260922.json"
OUT = ROOT / "Evidence" / "WorldOvermap" / "founder_runtime_spatial_convergence.json"
DOC = ROOT / "Docs" / "SOUL_FOUNDER_RUNTIME_SPATIAL_CONVERGENCE_20260922.md"

def solve_linear(matrix: list[list[float]], vector: list[float]) -> list[float]:
    n = len(vector)
    a = [row[:] + [vector[i]] for i, row in enumerate(matrix)]
    for col in range(n):
        pivot = max(range(col, n), key=lambda r: abs(a[r][col]))
        if abs(a[pivot][col]) < 1e-12:
            raise RuntimeError("singular normal equations")
        a[col], a[pivot] = a[pivot], a[col]
        denom = a[col][col]
        a[col] = [x / denom for x in a[col]]
        for row in range(n):
            if row == col:
                continue
            factor = a[row][col]
            if factor:
                a[row] = [a[row][j] - factor * a[col][j] for j in range(n + 1)]
    return [a[i][-1] for i in range(n)]

def least_squares(rows: list[list[float]], values: list[float]) -> list[float]:
    cols = len(rows[0])
    normal = [[0.0] * cols for _ in range(cols)]
    rhs = [0.0] * cols
    for row, value in zip(rows, values):
        for i in range(cols):
            rhs[i] += row[i] * value
            for j in range(cols):
                normal[i][j] += row[i] * row[j]
    return solve_linear(normal, rhs)

candidate_text = CANDIDATE_CPP.read_text(encoding="utf-8")
bundle = json.loads(BUNDLE.read_text(encoding="utf-8"))
pattern = re.compile(
    r'\{TEXT\("([^"]+)"\),\s*FVector\(([-\d.]+),\s*([-\d.]+),\s*([-\d.]+)\)\}'
)
candidate = {
    match.group(1): (float(match.group(2)), float(match.group(3)), float(match.group(4)))
    for match in pattern.finditer(candidate_text)
}
accepted = {
    row["RegionId"]: (float(row["X"]), float(row["Y"]), float(row["Z"]))
    for row in bundle["regions"]
}
region_ids = sorted(set(candidate) & set(accepted))
if len(region_ids) != 9:
    raise RuntimeError(f"expected 9 shared founder regions, got {len(region_ids)}")

# Reflected 2-D similarity:
# X = a*x + b*y + tx
# Y = b*x - a*y + ty
design = []
values = []
for rid in region_ids:
    x, y, _ = candidate[rid]
    X, Y, _ = accepted[rid]
    design.append([x, y, 1.0, 0.0])
    values.append(X)
    design.append([-y, x, 0.0, 1.0])
    values.append(Y)
a, b, tx, ty = least_squares(design, values)
scale = math.hypot(a, b)
angle_deg = math.degrees(math.atan2(-b, a))

def transform(point):
    x, y, z = point
    return (a * x + b * y + tx, b * x - a * y + ty, z)

region_rows = []
sum_sq = 0.0
max_error = 0.0
for rid in region_ids:
    px, py, _ = transform(candidate[rid])
    ax, ay, az = accepted[rid]
    error = math.hypot(px - ax, py - ay)
    sum_sq += error * error
    max_error = max(max_error, error)
    region_rows.append({
        "region_id": rid,
        "candidate_xy": list(candidate[rid][:2]),
        "transformed_candidate_xy": [round(px, 3), round(py, 3)],
        "accepted_xy": [ax, ay],
        "error_cm": round(error, 3),
    })

rms = math.sqrt(sum_sq / len(region_rows))
xs = [p[0] for p in accepted.values()]
ys = [p[1] for p in accepted.values()]
span = math.hypot(max(xs) - min(xs), max(ys) - min(ys))

edge_rows = []
for route in bundle["routes"]:
    ra, rb = route["A"], route["B"]
    ca = transform(candidate[ra])
    cb = transform(candidate[rb])
    aa = accepted[ra]
    ab = accepted[rb]
    transformed_len = math.hypot(cb[0] - ca[0], cb[1] - ca[1])
    accepted_len = math.hypot(ab[0] - aa[0], ab[1] - aa[1])
    ratio = transformed_len / accepted_len if accepted_len else 0.0
    edge_rows.append({
        "route_id": route["RouteId"],
        "transformed_candidate_length_cm": round(transformed_len, 3),
        "accepted_length_cm": round(accepted_len, 3),
        "delta_cm": round(transformed_len - accepted_len, 3),
        "ratio": round(ratio, 6),
        "relative_error_pct": round(abs(ratio - 1.0) * 100.0, 3),
    })

mean_edge_error = sum(abs(x["delta_cm"]) for x in edge_rows) / len(edge_rows)
max_edge_relative = max(x["relative_error_pct"] for x in edge_rows)
worst_regions = sorted(region_rows, key=lambda x: x["error_cm"], reverse=True)
worst_edges = sorted(edge_rows, key=lambda x: x["relative_error_pct"], reverse=True)

payload = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "READ_ONLY_SPATIAL_CONVERGENCE_ANALYSIS",
    "authority": {
        "accepted_layout": "Data/UEImport/soul_founder_import_bundle_v2_20260922.json",
        "candidate_layout": str(CANDIDATE_CPP),
        "candidate_authority": "UNTRACKED_DIRTY_PRIMARY_PROTOTYPE_NOT_CANONICAL",
    },
    "candidate_sha256": hashlib.sha256(CANDIDATE_CPP.read_bytes()).hexdigest(),
    "fit": {
        "model": "reflected_2d_similarity",
        "equations": {
            "accepted_x": "a*candidate_x + b*candidate_y + tx",
            "accepted_y": "b*candidate_x - a*candidate_y + ty",
        },
        "a": a,
        "b": b,
        "translation_x_cm": tx,
        "translation_y_cm": ty,
        "uniform_scale": scale,
        "orientation_angle_deg": angle_deg,
        "reflection": True,
        "rms_error_cm": rms,
        "max_region_error_cm": max_error,
        "accepted_span_cm": span,
        "rms_error_pct_world_span": 100.0 * rms / span,
        "max_error_pct_world_span": 100.0 * max_error / span,
    },
    "regions": region_rows,
    "routes": edge_rows,
    "route_error": {
        "mean_absolute_length_delta_cm": mean_edge_error,
        "max_relative_length_error_pct": max_edge_relative,
        "worst_routes": worst_edges[:3],
    },
    "interpretation": [
        "The candidate and accepted layouts share the same topology and are strongly related by one reflected uniform-scale transform.",
        "The fit is close enough to reuse the prototype's interaction/camera concepts rather than rebuild them from scratch.",
        "The remaining local deviations are large enough that accepted v2 region coordinates and route splines should replace candidate hardcoded positions rather than applying the best-fit transform at runtime.",
        "No candidate source file was modified by this analysis.",
    ],
    "migration_recommendation": {
        "reuse": [
            "region actor interaction model",
            "campaign camera/input concepts after qualification",
            "existing nine-node runtime identifiers",
        ],
        "replace_from_import_bundle": [
            "hardcoded RegionPositions coordinates",
            "hardcoded connection presentation geometry",
            "region biome/landform/feature metadata",
        ],
        "do_not_make_canonical": "best-fit transform; it is migration evidence only",
    },
}
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

lines = [
    "# Soul Founder Runtime Spatial Convergence — 2026-09-22", "",
    "Status: **read-only migration analysis; accepted v2 coordinates remain authoritative.**", "",
    f"- Best fit: reflected 2-D similarity, uniform scale **{scale:.3f}x**.",
    f"- RMS position error: **{rms/100:.1f} m** ({100*rms/span:.2f}% of founder-map span).",
    f"- Worst region error: **{max_error/100:.1f} m** ({100*max_error/span:.2f}% of span).",
    f"- Mean route-length delta: **{mean_edge_error/100:.1f} m**.",
    f"- Worst route relative length error: **{max_edge_relative:.1f}%**.", "",
    "## Migration conclusion", "",
    "The existing founder runtime layout is structurally reusable, not disposable: it is essentially the accepted layout mirrored/scaled. "
    "However, the local deviations are too large to promote its hardcoded coordinates. Preserve the runtime interaction/camera concepts, "
    "but replace RegionPositions and route presentation geometry directly from the v2 import bundle.", "",
    "## Largest position deviations", "",
]
for row in worst_regions[:4]:
    lines.append(f"- {row['region_id']}: {row['error_cm']/100:.1f} m after best-fit transform.")
lines += ["", "## Largest route-length deviations", ""]
for row in worst_edges[:4]:
    lines.append(
        f"- {row['route_id']}: {row['relative_error_pct']:.1f}% "
        f"({row['delta_cm']/100:+.1f} m)."
    )
DOC.write_text("\n".join(lines) + "\n", encoding="utf-8")
print(json.dumps({
    "status": payload["status"],
    "scale": round(scale, 4),
    "rms_error_pct_world_span": round(100*rms/span, 3),
    "max_route_relative_error_pct": round(max_edge_relative, 3),
}, indent=2))
