import json
from pathlib import Path
from collections import Counter

ROOT = Path(r"D:\RefinedBadger\Games\Soul")
settlements = json.loads((ROOT / "Data" / "settlement_blueprints.json").read_text())
battlefields = json.loads((ROOT / "Data" / "battlefield_recipes.json").read_text())
cities = json.loads((ROOT / "Data" / "city_siege_blueprints.json").read_text())

errors = []
warnings = []
active = [f for f in settlements["factions"] if f.get("enabled")]
active_ids = {f["id"] for f in active}

expected_roles = Counter({"fighter": 4, "ranged": 1, "support_magic": 1, "beast": 1})
seen_structure_ids = set()

for faction in active:
    fid = faction["id"]
    recruits = faction.get("recruit_structures", [])
    if len(recruits) != 7:
        errors.append(f"{fid}: expected 7 recruit structures, got {len(recruits)}")
    roles = Counter(x["role"] for x in recruits)
    if roles != expected_roles:
        errors.append(f"{fid}: role shape {dict(roles)} != {dict(expected_roles)}")
    for structure in recruits + faction.get("civic_structures", []):
        sid = structure["id"]
        if sid in seen_structure_ids:
            errors.append(f"duplicate structure id {sid}")
        seen_structure_ids.add(sid)
    status = faction.get("donor", {}).get("status", "")
    if "PENDING" in status:
        warnings.append(f"{fid}: primary environment payload still needs UE/cache qualification")

city_by_faction = {c["faction"]: c for c in cities["cities"]}
for fid in active_ids:
    if fid not in city_by_faction:
        errors.append(f"{fid}: missing city/siege blueprint")
        continue
    city = city_by_faction[fid]
    layers = city.get("siege", {}).get("layers", [])
    objectives = city.get("siege", {}).get("objectives", [])
    if len(layers) < 4:
        errors.append(f"{fid}: siege needs at least 4 layers")
    if len(objectives) < 4:
        errors.append(f"{fid}: siege needs at least 4 physical objectives")
recipes_by_faction = Counter(r["faction"] for r in battlefields["recipes"])
for fid in active_ids:
    if recipes_by_faction[fid] < 4:
        errors.append(f"{fid}: needs >=4 homeland battlefield recipes, has {recipes_by_faction[fid]}")

seen_recipe_ids = set()
for recipe in battlefields["recipes"]:
    rid = recipe["id"]
    if rid in seen_recipe_ids:
        errors.append(f"duplicate battlefield recipe id {rid}")
    seen_recipe_ids.add(rid)
    if not recipe.get("landmark"):
        errors.append(f"{rid}: landmark missing")
    if len(recipe.get("tactics", [])) < 2:
        errors.append(f"{rid}: fewer than two tactical questions")
    if recipe.get("status") == "PAYLOAD_PENDING":
        warnings.append(f"{rid}: source payload pending local UE qualification")

candidate = next((f for f in settlements["factions"] if f["id"] == "nature_candidate"), None)
if candidate and candidate.get("enabled"):
    warnings.append("nature_candidate is enabled; this should be an explicit founder roster decision")
print("SOUL ENVIRONMENT PLAN VALIDATION")
print(f"active_factions={len(active)}")
print(f"recruit_structures={sum(len(f['recruit_structures']) for f in active)}")
print(f"battlefield_recipes={len(battlefields['recipes'])}")
print(f"city_siege_blueprints={len(cities['cities'])}")
print(f"errors={len(errors)} warnings={len(warnings)}")
for e in errors:
    print("ERROR:", e)
for w in warnings:
    print("WARN:", w)

evidence = ROOT / "Evidence" / "environment_plan_validation_20260920.txt"
evidence.write_text(
    "\n".join([
        "SOUL ENVIRONMENT PLAN VALIDATION",
        f"active_factions={len(active)}",
        f"recruit_structures={sum(len(f['recruit_structures']) for f in active)}",
        f"battlefield_recipes={len(battlefields['recipes'])}",
        f"city_siege_blueprints={len(cities['cities'])}",
        f"errors={len(errors)} warnings={len(warnings)}",
        *[f"ERROR: {x}" for x in errors],
        *[f"WARN: {x}" for x in warnings],
        "STATUS=" + ("PASS" if not errors else "FAIL"),
    ]) + "\n",
    encoding="utf-8",
)
raise SystemExit(1 if errors else 0)
