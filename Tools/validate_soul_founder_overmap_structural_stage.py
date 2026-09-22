"""Static validation for the future UE founder-overmap structural staging script."""
from __future__ import annotations

import ast
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "Tools" / "stage_soul_founder_overmap_structural.py"
BUNDLE = ROOT / "Data" / "UEImport" / "soul_founder_import_bundle_v2_20260922.json"
FIXTURES = ROOT / "Data" / "soul_overmap_ue_acceptance_fixtures_v1_20260922.json"
OUT = ROOT / "Evidence" / "WorldOvermap" / "founder_overmap_structural_stage_validation.json"
REPORT = ROOT / "Evidence" / "WorldOvermap" / "founder_overmap_structural_stage_validation.md"
LAUNCHER = ROOT / "Tools" / "Run_Soul_Founder_Overmap_Structural.bat"

source = SCRIPT.read_text(encoding="utf-8")
launcher_source = LAUNCHER.read_text(encoding="utf-8")
bundle = json.loads(BUNDLE.read_text(encoding="utf-8"))
fixtures = json.loads(FIXTURES.read_text(encoding="utf-8"))
errors: list[str] = []
warnings: list[str] = []

try:
    ast.parse(source, filename=str(SCRIPT))
except SyntaxError as exc:
    errors.append(f"staging script syntax error: {exc}")

if bundle.get("schema") != 2 or bundle.get("status") != "FOUNDER_IMPORT_READY_NON_UE":
    errors.append("founder v2 import bundle is not accepted")
if bundle.get("counts", {}).get("regions") != 9:
    errors.append("expected 9 founder regions")
if bundle.get("counts", {}).get("routes") != 10:
    errors.append("expected 10 founder routes")
if bundle.get("counts", {}).get("directed_approaches") != 20:
    errors.append("expected 20 founder directed approaches")
if len(fixtures.get("fixtures", [])) != 6:
    errors.append("expected 6 prepared UE acceptance fixtures")

region_ids = {x["RegionId"] for x in bundle["regions"]}
route_ids = set()
route_segments = 0
for route in bundle["routes"]:
    rid = route["RouteId"]
    if rid in route_ids:
        errors.append(f"duplicate route {rid}")
    route_ids.add(rid)
    if route["A"] not in region_ids or route["B"] not in region_ids:
        errors.append(f"{rid}: route endpoint outside founder slice")
    try:
        points = json.loads(route["SplinePointsCmJson"])
    except Exception as exc:
        errors.append(f"{rid}: invalid SplinePointsCmJson: {exc}")
        continue
    if len(points) != 3:
        errors.append(f"{rid}: structural staging requires exactly 3 route spline points")
    for point in points:
        if not isinstance(point, list) or len(point) != 3:
            errors.append(f"{rid}: invalid 3D route point {point!r}")
    route_segments += max(0, len(points) - 1)

required_literals = [
    'MAP_DEST = "/Game/Soul/Maps/Overmap/LV_Soul_FounderOvermap_Structural"',
    'soul_founder_import_bundle_v2_20260922.json',
    'soul_overmap_ue_acceptance_fixtures_v1_20260922.json',
    'SoulCore remains campaign authority',
    'RB Weather remains weather authority',
]
for literal in required_literals:
    if literal not in source:
        errors.append(f"staging script missing required contract literal: {literal}")

for forbidden in [
    r"D:\RefinedBadger\Games\Soul",
    "EditorAssetLibrary.delete_asset",
    "delete_directory",
    "SystemLibrary.quit_editor",
]:
    if forbidden in source:
        errors.append(f"staging script contains forbidden operation/path: {forbidden}")

basic_shapes = [
    "/Engine/BasicShapes/Cube.Cube",
    "/Engine/BasicShapes/Cylinder.Cylinder",
    "/Engine/BasicShapes/Sphere.Sphere",
]
for asset in basic_shapes:
    if asset not in source:
        errors.append(f"missing proof-only Engine asset: {asset}")

if route_segments != 20:
    errors.append(f"expected 20 structural route segments, got {route_segments}")

launcher_required = [
    "Get-CimInstance Win32_Process",
    "UnrealBuildTool",
    "BUSY: another UE/build lane is active",
    "-ExecutePythonScript=",
    "-nullrhi",
]
for literal in launcher_required:
    if literal not in launcher_source:
        errors.append(f"guarded launcher missing required literal: {literal}")
if "exit /b 42" not in launcher_source:
    errors.append("guarded launcher does not fail closed on an occupied UE/build lane")

fixture_ids = {x["id"] for x in fixtures["fixtures"]}
required_fixtures = {
    "ue.fixture.founder_forest_contact",
    "ue.fixture.human_capital_east_assault",
    "ue.fixture.broken_bridge_chokepoint",
    "ue.fixture.dragon_graveyard_special_site",
    "ue.fixture.stronghold_reinforcement_retreat",
    "ue.fixture.river_ford_context",
}
if fixture_ids != required_fixtures:
    errors.append("structural proof fixture set drift")

expected_actor_minimum = 1 + 9 + 20 + 1 + 1
result = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "pass" if not errors else "fail",
    "execution_status": "NOT_RUN_UE_LANE_BUSY",
    "map_destination": "/Game/Soul/Maps/Overmap/LV_Soul_FounderOvermap_Structural",
    "regions_planned": len(region_ids),
    "routes_planned": len(route_ids),
    "route_segments_planned": route_segments,
    "core_actor_minimum_excluding_optional_labels": expected_actor_minimum,
    "acceptance_fixtures_carried": len(fixture_ids),
    "proof_asset_policy": "Engine BasicShapes only; no donor or shipping-art claim",
    "guarded_launcher": "Tools/Run_Soul_Founder_Overmap_Structural.bat",
    "warnings": warnings,
    "errors": errors,
}
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")

lines = [
    "# Soul Founder Structural UE Staging — Static Validation",
    "",
    f"Status: **{result['status'].upper()}**",
    "",
    f"- UE execution: **{result['execution_status']}**.",
    f"- Regions planned: **{result['regions_planned']}**.",
    f"- Routes planned: **{result['routes_planned']}**.",
    f"- Route segments planned: **{result['route_segments_planned']}**.",
    f"- Core proof actors (excluding optional labels): **{expected_actor_minimum}**.",
    f"- Acceptance fixtures carried forward: **{result['acceptance_fixtures_carried']}**.",
    "",
    "The staging script is intentionally unexecuted while another UE/build lane owns the machine.",
    "It creates only proof geometry using Engine BasicShapes and does not establish campaign gameplay authority.",
]
if errors:
    lines += ["", "## Errors", ""] + [f"- {x}" for x in errors]
REPORT.write_text("\n".join(lines) + "\n", encoding="utf-8")
print(json.dumps(result, indent=2))
if errors:
    raise SystemExit(1)
