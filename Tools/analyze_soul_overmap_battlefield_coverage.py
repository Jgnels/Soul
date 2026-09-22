"""Audit campaign-to-battle coverage and context fidelity for Soul's overmap."""
import json
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORLD = json.loads((ROOT / "Data" / "soul_world_overmap_v1_20260922.json").read_text(encoding="utf-8"))
BATTLES = json.loads((ROOT / "Data" / "battlefield_recipes.json").read_text(encoding="utf-8"))
PROFILES = json.loads((ROOT / "Data" / "soul_overmap_approach_profiles_v1_20260922.json").read_text(encoding="utf-8"))
OUT_JSON = ROOT / "Evidence" / "WorldOvermap" / "battlefield_coverage_analysis.json"
OUT_MD = ROOT / "Evidence" / "WorldOvermap" / "battlefield_coverage_analysis.md"

nodes = {n["id"]: n for n in WORLD["nodes"]}
recipes = {r["id"]: r for r in BATTLES["recipes"]}
approaches = PROFILES["approaches"]
founder = set(WORLD["founder_slice"]["region_ids"])

TRANSITION_SETTLEMENT_FEATURES = {"capital", "stronghold", "great_forge", "great_tree", "throne_sanctum"}
IDENTITY_SENSITIVE_FEATURES = {"trade_road", "river_crossing", "grove", "ancient_shrine", "watch", "narrow_pass", "road_crossing"}
approaches_by_destination = defaultdict(list)
for profile in approaches:
    approaches_by_destination[profile["destination_region"]].append(profile)

missing_recipe_regions = []
rows = []
for region_id in sorted(nodes):
    node = nodes[region_id]
    recipe_id = node.get("battle_recipe_hint")
    recipe = recipes.get(recipe_id)
    if recipe is None:
        missing_recipe_regions.append(region_id)
        continue
    differences = {}
    exact_fields = []
    for field in ("biome", "landform", "feature"):
        if node.get(field) == recipe.get(field):
            exact_fields.append(field)
        else:
            differences[field] = {
                "campaign": node.get(field),
                "battlefield": recipe.get(field),
            }
    if not differences:
        fidelity_class = "exact_context"
    elif node.get("feature") in TRANSITION_SETTLEMENT_FEATURES and node.get("settlement_id"):
        fidelity_class = "settlement_approach_transform"
    else:
        fidelity_class = "context_transform_review"
    identity_sensitive = node.get("feature") in IDENTITY_SENSITIVE_FEATURES
    rows.append({
        "region_id": region_id,
        "display_name": node["name"],
        "founder_slice": region_id in founder,
        "recipe_id": recipe_id,
        "recipe_status": recipe["status"],
        "fidelity_class": fidelity_class,
        "identity_sensitive_feature": identity_sensitive,
        "exact_context_fields": exact_fields,
        "context_differences": differences,
        "incoming_approach_count": len(approaches_by_destination[region_id]),
    })

missing_profile_recipes = [
    p["id"] for p in approaches
    if p["battlefield"]["recipe_id"] not in recipes
]
profile_recipe_drift = [
    p["id"] for p in approaches
    if p["battlefield"]["recipe_id"] != nodes[p["destination_region"]]["battle_recipe_hint"]
]

status_counts = Counter(r["recipe_status"] for r in rows)
fidelity_counts = Counter(r["fidelity_class"] for r in rows)
review_rows = [r for r in rows if r["fidelity_class"] == "context_transform_review"]
priority_review = [r for r in review_rows if r["identity_sensitive_feature"] or r["founder_slice"]]
hard_errors = []
if missing_recipe_regions:
    hard_errors.append(f"regions missing recipe ids: {missing_recipe_regions}")
if missing_profile_recipes:
    hard_errors.append(f"approaches missing recipes: {missing_profile_recipes}")
if profile_recipe_drift:
    hard_errors.append(f"approach recipe drift: {profile_recipe_drift}")
if len(rows) != len(nodes):
    hard_errors.append(f"expected {len(nodes)} covered regions, got {len(rows)}")

result = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "pass" if not hard_errors else "fail",
    "hard_integrity": {
        "regions": len(nodes),
        "covered_regions": len(rows),
        "directed_approaches": len(approaches),
        "missing_recipe_regions": missing_recipe_regions,
        "missing_profile_recipes": missing_profile_recipes,
        "profile_recipe_drift": profile_recipe_drift,
    },
    "recipe_status_counts_by_region": dict(sorted(status_counts.items())),
    "fidelity_counts_by_region": dict(sorted(fidelity_counts.items())),
    "context_transform_review_count": len(review_rows),
    "priority_review_count": len(priority_review),
    "priority_review_regions": [r["region_id"] for r in priority_review],
    "regions": rows,
    "errors": hard_errors,
}
OUT_JSON.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")

lines = [
    "# Soul Overmap - Battlefield Coverage Audit",
    "",
    f"Hard integrity: **{result['status'].upper()}**.",
    f"Regions covered: **{len(rows)}/{len(nodes)}**.",
    f"Directed approaches checked: **{len(approaches)}**.",
    "",
    "## Recipe readiness by destination region",
    "",
    "| Status | Regions |",
    "|---|---:|",
]
for status, count in sorted(status_counts.items()):
    lines.append(f"| {status} | {count} |")

lines += [
    "",
    "## Context fidelity",
    "",
    "Exact metadata equality is diagnostic only; settlement approaches may intentionally transform a capital/hold into its outskirts.",
    "",
    "| Class | Regions |",
    "|---|---:|",
]
for klass, count in sorted(fidelity_counts.items()):
    lines.append(f"| {klass} | {count} |")
lines += [
    "",
    "## Priority context-transform review",
    "",
    "These are not hard failures. They are founder-slice or identity-sensitive destinations where the selected recipe changes campaign context metadata.",
    "",
    "| Region | Recipe | Status | Differing context |",
    "|---|---|---|---|",
]
for row in priority_review:
    diffs = "; ".join(
        f"{field}: {values['campaign']} -> {values['battlefield']}"
        for field, values in row["context_differences"].items()
    )
    lines.append(
        f"| {row['display_name']} | {row['recipe_id']} | {row['recipe_status']} | {diffs} |"
    )

lines += [
    "",
    "## Interpretation",
    "",
    "- Missing recipe IDs or profile drift are hard failures; none should be accepted.",
    "- PAYLOAD_PENDING / UE_COMPOSITE / UE_CROP_REQUIRED are production-readiness gates, not topology failures.",
    "- Context transforms require design/UE review only where the chosen battlefield stops communicating a campaign-map feature that mattered to the route decision.",
    "- Weather and time remain runtime-owned and are intentionally outside this static audit.",
]
OUT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")

print(json.dumps({k: v for k, v in result.items() if k != "regions"}, indent=2))
print(f"WROTE {OUT_JSON}")
print(f"WROTE {OUT_MD}")
if hard_errors:
    raise SystemExit(1)
