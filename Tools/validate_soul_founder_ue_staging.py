"""Validate Unreal/DataTable-friendly founder overmap staging exports."""
import csv
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PRESENTATION = ROOT / "Data" / "soul_founder_slice_presentation_import_v1_20260922.json"
STATES = ROOT / "Data" / "soul_founder_presentation_state_vectors_v1_20260922.json"
OUT_DIR = ROOT / "Data" / "UEImport"
MANIFEST = OUT_DIR / "soul_founder_ue_import_manifest_v1_20260922.json"
EVIDENCE = ROOT / "Evidence" / "WorldOvermap" / "founder_ue_staging_validation.json"
REPORT = ROOT / "Evidence" / "WorldOvermap" / "founder_ue_staging_validation.md"

p = json.loads(PRESENTATION.read_text(encoding="utf-8"))
s = json.loads(STATES.read_text(encoding="utf-8"))
m = json.loads(MANIFEST.read_text(encoding="utf-8"))
errors = []

def read_csv(name):
    path = OUT_DIR / name
    with path.open(newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    if not rows or "Name" not in rows[0]:
        errors.append(f"{name}: first DataTable key column Name is missing")
    names = [r.get("Name", "") for r in rows]
    if len(names) != len(set(names)) or any(not n for n in names):
        errors.append(f"{name}: DataTable row names are missing or duplicated")
    return rows
region_name = "soul_founder_regions_v1_20260922.csv"
route_name = "soul_founder_routes_v1_20260922.csv"
approach_name = "soul_founder_approaches_v1_20260922.csv"
state_name = "soul_founder_presentation_states_v1_20260922.csv"

regions = read_csv(region_name)
routes = read_csv(route_name)
approaches = read_csv(approach_name)
states = read_csv(state_name)

expected_counts = {
    "regions": len(p["regions"]),
    "routes": len(p["routes"]),
    "approaches": len(p["directed_approaches"]),
    "presentation_states": sum(len(g["states"]) for g in s["corridors"] + s["detours"]),
}
actual_counts = {
    "regions": len(regions),
    "routes": len(routes),
    "approaches": len(approaches),
    "presentation_states": len(states),
}
if actual_counts != expected_counts:
    errors.append(f"row count mismatch: expected {expected_counts}, got {actual_counts}")
if m.get("counts") != expected_counts:
    errors.append("manifest count drift")
source_hashes = {
    PRESENTATION.name: hashlib.sha256(PRESENTATION.read_bytes()).hexdigest(),
    STATES.name: hashlib.sha256(STATES.read_bytes()).hexdigest(),
}
if m.get("sources") != source_hashes:
    errors.append("manifest source hash drift")

for file_name, expected_hash in m.get("files", {}).items():
    path = OUT_DIR / file_name
    if not path.exists():
        errors.append(f"manifest file missing: {file_name}")
        continue
    actual_hash = hashlib.sha256(path.read_bytes()).hexdigest()
    if actual_hash != expected_hash:
        errors.append(f"manifest output hash drift: {file_name}")

expected_region_ids = {r["region_id"] for r in p["regions"]}
if {r["RegionId"] for r in regions} != expected_region_ids:
    errors.append("region CSV ID set drift")
expected_route_ids = {r["route_id"] for r in p["routes"]}
if {r["RouteId"] for r in routes} != expected_route_ids:
    errors.append("route CSV ID set drift")
expected_approach_ids = {a["id"] for a in p["directed_approaches"]}
if {r["ApproachId"] for r in approaches} != expected_approach_ids:
    errors.append("approach CSV ID set drift")
route_pairs = {
    frozenset((r["A"], r["B"])): r["RouteId"]
    for r in routes
}
for row in routes:
    if row["A"] not in expected_region_ids or row["B"] not in expected_region_ids:
        errors.append(f"{row['RouteId']}: route endpoint missing from founder regions")
    if row["Road"] not in {"true", "false"} or row["Chokepoint"] not in {"true", "false"}:
        errors.append(f"{row['RouteId']}: boolean encoding drift")
    for axis in ("X", "Y", "Z"):
        if row[f"Spline0{axis}"] == "" or row[f"Spline2{axis}"] == "":
            errors.append(f"{row['RouteId']}: endpoint spline data missing")

approach_pairs = {}
for row in approaches:
    pair = (row["SourceRegion"], row["DestinationRegion"])
    approach_pairs[pair] = row["ApproachId"]
    route_id = route_pairs.get(frozenset(pair))
    if route_id is None:
        errors.append(f"{row['ApproachId']}: no matching founder route")
    if row["BattleRecipe"] == "":
        errors.append(f"{row['ApproachId']}: battle recipe missing")

state_keys = set()
battle_rows = 0
for row in states:
    key = (row["SourceKind"], row["ScenarioId"], int(row["Step"]))
    if key in state_keys:
        errors.append(f"duplicate state key: {key}")
    state_keys.add(key)
    active = row["ActiveRegion"]
    target = row["PlannedTargetRegion"]
    if active not in expected_region_ids or target not in expected_region_ids:
        errors.append(f"{row['Name']}: active/target region missing")
    route_id = route_pairs.get(frozenset((active, target)))
    if route_id != row["PlannedRouteId"]:
        errors.append(f"{row['Name']}: planned route does not connect active/target")
    visible = set(filter(None, row["VisibleRegions"].split("|")))
    if target not in visible:
        errors.append(f"{row['Name']}: planned target absent from visible regions")
    if row["PlannedAction"] == "battle_commit":
        battle_rows += 1
        pair = (active, target)
        if approach_pairs.get(pair) != row["BattleApproachId"]:
            errors.append(f"{row['Name']}: battle approach mismatch")
        if not row["BattleRecipeId"]:
            errors.append(f"{row['Name']}: battle recipe missing")
    elif row["BattleApproachId"] or row["BattleRecipeId"]:
        errors.append(f"{row['Name']}: move state leaks battle-only fields")

expected_battle_rows = len(s["corridors"]) + len(s["detours"])
if battle_rows != expected_battle_rows:
    errors.append(f"expected {expected_battle_rows} battle-commit state rows, got {battle_rows}")
result = {
    "schema": 1,
    "status": "pass" if not errors else "fail",
    "counts": actual_counts,
    "battle_commit_rows": battle_rows,
    "manifest_sha256": hashlib.sha256(MANIFEST.read_bytes()).hexdigest(),
    "csv_sha256": m.get("files", {}),
    "errors": errors,
}
EVIDENCE.parent.mkdir(parents=True, exist_ok=True)
EVIDENCE.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")

lines = [
    "# Soul Founder UE Staging Validation",
    "",
    f"Status: **{result['status'].upper()}**",
    "",
    f"- Regions: **{actual_counts['regions']}**.",
    f"- Routes: **{actual_counts['routes']}**.",
    f"- Directed approaches: **{actual_counts['approaches']}**.",
    f"- Presentation states: **{actual_counts['presentation_states']}**.",
    f"- Battle-commit state rows: **{battle_rows}**.",
    "",
    "All CSVs use a stable first-column Name key suitable for Unreal DataTable staging.",
    "The CSV layer is non-authoritative; source JSON and SoulCore remain the source of truth.",
]
if errors:
    lines += ["", "## Errors", ""] + [f"- {e}" for e in errors]
REPORT.write_text("\n".join(lines) + "\n", encoding="utf-8")
print(json.dumps(result, indent=2))
if errors:
    raise SystemExit(1)
