"""Validate Soul founder import bundle v2 for complete, deterministic UE staging."""
from __future__ import annotations

import csv
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
UE = ROOT / "Data" / "UEImport"
MANIFEST = UE / "soul_founder_import_bundle_v2_20260922.json"
REGIONS = UE / "soul_founder_regions_import_v2_20260922.csv"
ROUTES = UE / "soul_founder_routes_import_v2_20260922.csv"
APPROACHES = UE / "soul_founder_approaches_import_v2_20260922.csv"
STATES = UE / "soul_founder_states_import_v2_20260922.csv"
STARTS = ROOT / "Data" / "soul_campaign_start_states_v1_20260922.json"
OUT = ROOT / "Evidence" / "WorldOvermap" / "founder_import_bundle_v2_validation.json"

def read_csv(path: Path) -> list[dict]:
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        return list(csv.DictReader(handle))

def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
starts = json.loads(STARTS.read_text(encoding="utf-8"))
scenario = starts["scenarios"]["founder_human_orc_micro"]
expected_regions = set(scenario["region_ids"])
regions = read_csv(REGIONS)
routes = read_csv(ROUTES)
approaches = read_csv(APPROACHES)
states = read_csv(STATES)

errors: list[str] = []
warnings: list[str] = []

if manifest.get("schema") != 2 or manifest.get("semantic_version") != "2.0.0":
    errors.append("manifest schema/version mismatch")
if manifest.get("status") != "FOUNDER_IMPORT_READY_NON_UE":
    errors.append("manifest status mismatch")

region_ids = {x["RegionId"] for x in regions}
if region_ids != expected_regions:
    errors.append(f"region coverage mismatch: {sorted(region_ids ^ expected_regions)}")
if len(regions) != 9:
    errors.append(f"expected 9 region rows, got {len(regions)}")
if len(routes) != 10:
    errors.append(f"expected 10 route rows, got {len(routes)}")
if len(approaches) != 20:
    errors.append(f"expected 20 approach rows, got {len(approaches)}")
if len(states) != 30:
    errors.append(f"expected 30 presentation-state rows, got {len(states)}")

required_region = [
    "RegionId","DisplayName","X","Y","Z","SelectionRadius","InitialFogState",
    "MacroRegion","Biome","Landform","Feature","ElevationBand","SurfaceIdentity",
    "SurfaceStatus","AnchorType","AnchorLabel","AnchorSource","AnchorScaleClass",
    "Interactive","InteractionScope","PersistentStateProjection","BattleRecipe",
    "BattleRecipeStatus","BattleRecipeDonor","BattleLandmark",
    "IncomingApproachIds","OutgoingApproachIds","IncomingHandoffIds","OutgoingHandoffIds",
]
for row in regions:
    rid = row["RegionId"]
    for field in required_region:
        if row.get(field, "") == "":
            errors.append(f"{rid}: missing required region field {field}")
    if row["Interactive"] not in ("true","false"):
        errors.append(f"{rid}: invalid Interactive boolean")
    if row["PersistentStateProjection"] not in ("true","false"):
        errors.append(f"{rid}: invalid PersistentStateProjection boolean")
    incoming = [x for x in row["IncomingApproachIds"].split("|") if x]
    outgoing = [x for x in row["OutgoingApproachIds"].split("|") if x]
    if not incoming or not outgoing:
        errors.append(f"{rid}: region must have incoming and outgoing founder approaches")

for rid in ("human_capital","crossroads","orc_camp"):
    row = next(x for x in regions if x["RegionId"] == rid)
    if not row["SettlementTier"] or not row["SettlementStatus"] or not row["SettlementVisitScope"]:
        errors.append(f"{rid}: expected settlement-slot metadata")
for rid in expected_regions - {"human_capital","crossroads","orc_camp"}:
    row = next(x for x in regions if x["RegionId"] == rid)
    if row["SettlementTier"]:
        warnings.append(f"{rid}: unexpected settlement metadata present")

route_ids = set()
approach_ids = {x["ApproachId"] for x in approaches}
handoff_ids = {x["BattleHandoffId"] for x in approaches}
for row in routes:
    route_id = row["RouteId"]
    if route_id in route_ids:
        errors.append(f"duplicate route {route_id}")
    route_ids.add(route_id)
    if row["A"] not in region_ids or row["B"] not in region_ids:
        errors.append(f"{route_id}: endpoint outside founder slice")
    for field in (
        "RouteClass","Road","Chokepoint","ActionCost","LogisticsCost","SplinePointsCmJson",
        "SplineWidth","RenderFamily","Surface","NavigationCue",
        "AtoBApproachId","AtoBEntryDirection","AtoBTransitionKind","AtoBBattleRecipe",
        "AtoBBattleRecipeStatus","AtoBBattleHandoffId",
        "BtoAApproachId","BtoAEntryDirection","BtoATransitionKind","BtoABattleRecipe",
        "BtoABattleRecipeStatus","BtoABattleHandoffId",
    ):
        if row.get(field, "") == "":
            errors.append(f"{route_id}: missing route field {field}")
    if row["AtoBApproachId"] not in approach_ids or row["BtoAApproachId"] not in approach_ids:
        errors.append(f"{route_id}: missing directed approach linkage")
    if row["AtoBBattleHandoffId"] not in handoff_ids or row["BtoABattleHandoffId"] not in handoff_ids:
        errors.append(f"{route_id}: missing battle handoff linkage")
    try:
        points = json.loads(row["SplinePointsCmJson"])
        if len(points) != 3 or any(len(p) != 3 for p in points):
            errors.append(f"{route_id}: expected three 3D spline points")
    except Exception as exc:
        errors.append(f"{route_id}: invalid spline JSON: {exc}")

pairs = set()
for row in approaches:
    pair = (row["SourceRegion"], row["DestinationRegion"])
    if pair in pairs:
        errors.append(f"duplicate directed approach {pair}")
    pairs.add(pair)
    if not set(pair) <= region_ids:
        errors.append(f"{row['ApproachId']}: endpoint outside founder slice")
    for field in (
        "ApproachId","RouteType","Road","Chokepoint","ActionCost","LogisticsCost",
        "EntryDirection","TransitionKind","BattleRecipe","BattleRecipeStatus","BattleHandoffId",
        "EncounterKind","WeatherAuthority","TimeAuthority","ApproachTags",
    ):
        if row.get(field, "") == "":
            errors.append(f"{row['ApproachId']}: missing approach field {field}")
    if row["WeatherAuthority"] != "RBWeather":
        errors.append(f"{row['ApproachId']}: weather authority must be RBWeather")

if len(pairs) != 20:
    errors.append("directed approach pair coverage incomplete")

for row in states:
    if row["ActiveRegion"] not in region_ids:
        errors.append(f"{row['Name']}: invalid active region")
    if not row.get("ActiveRegionBiome") or not row.get("ActiveRegionAnchorType"):
        errors.append(f"{row['Name']}: missing enriched active-region presentation metadata")
    if row.get("PlannedAction") == "battle_commit":
        if not row.get("BattleApproachId") or not row.get("BattleHandoffId"):
            errors.append(f"{row['Name']}: battle commit lacks approach/handoff")
        if row.get("BattleApproachId") not in approach_ids:
            errors.append(f"{row['Name']}: unknown battle approach")
        if row.get("BattleHandoffId") not in handoff_ids:
            errors.append(f"{row['Name']}: unknown battle handoff")

for rel, expected_hash in manifest["files"].items():
    path = ROOT / rel
    if not path.exists():
        errors.append(f"manifest output missing: {rel}")
    elif sha(path) != expected_hash:
        errors.append(f"manifest output hash mismatch: {rel}")
for rel, expected_hash in manifest["sources"].items():
    path = ROOT / rel
    if not path.exists():
        errors.append(f"manifest source missing: {rel}")
    elif sha(path) != expected_hash:
        errors.append(f"manifest source hash mismatch: {rel}")

counts = manifest["counts"]
for key, actual in {
    "regions":len(regions), "routes":len(routes), "directed_approaches":len(approaches),
    "presentation_states":len(states),
}.items():
    if counts.get(key) != actual:
        errors.append(f"manifest count mismatch for {key}")

result = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "pass" if not errors else "fail",
    "counts": counts,
    "complete_region_metadata": not any("region field" in x for x in errors),
    "complete_route_metadata": not any("route field" in x for x in errors),
    "complete_approach_metadata": not any("approach field" in x for x in errors),
    "state_battle_linkage_valid": not any("battle commit" in x or "battle approach" in x or "battle handoff" in x for x in errors),
    "warnings": warnings,
    "errors": errors,
}
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
print(json.dumps(result, indent=2))
if errors:
    raise SystemExit(1)
