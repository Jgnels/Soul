"""Validate Soul overmap settlement-density candidate layer."""
import json
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORLD = json.loads((ROOT/"Data"/"soul_world_overmap_v1_20260922.json").read_text(encoding="utf-8"))
SLOTS = json.loads((ROOT/"Data"/"soul_overmap_settlement_slots_v1_20260922.json").read_text(encoding="utf-8"))
CITIES = json.loads((ROOT/"Data"/"city_siege_blueprints.json").read_text(encoding="utf-8"))
OUT = ROOT/"Evidence"/"WorldOvermap"/"settlement_slot_validation.json"

nodes = {n["id"]:n for n in WORLD["nodes"]}
city_ids = {c["id"] for c in CITIES["cities"]}
slots = SLOTS["slots"]
errors = []
seen = set()

for s in slots:
    rid = s["region_id"]
    if rid in seen:
        errors.append(f"duplicate settlement slot: {rid}")
    seen.add(rid)
    if rid not in nodes:
        errors.append(f"slot region missing from world: {rid}")
    if s["tier"] == "major":
        if s.get("settlement_id") not in city_ids:
            errors.append(f"major slot missing city blueprint: {rid}")
        if s.get("visit_scope") != "full_visit_scene":
            errors.append(f"major slot visit scope drift: {rid}")
    elif s["tier"] == "minor":
        if s.get("settlement_id") is not None:
            errors.append(f"minor slot must not claim canonical city id: {rid}")
        if s.get("status") != "CANDIDATE_NONCANONICAL":
            errors.append(f"minor slot lost candidate status: {rid}")
    else:
        errors.append(f"unknown slot tier: {rid}")
counts = Counter(s["tier"] for s in slots)
if counts["major"] != 6:
    errors.append(f"expected 6 major seats, got {counts['major']}")
if counts["minor"] != 8:
    errors.append(f"expected 8 minor candidates, got {counts['minor']}")

minor_factions = Counter((s.get("faction_affinity") or "neutral") for s in slots if s["tier"]=="minor")
for faction in ["humans","vikings","dwarves","orcs","nature","dark"]:
    if minor_factions[faction] < 1:
        errors.append(f"missing minor settlement candidate for {faction}")

if not set(seen).issubset(nodes):
    errors.append("settlement layer introduced non-world region ids")

result = {
    "schema": 1,
    "status": "pass" if not errors else "fail",
    "major_slots": counts["major"],
    "minor_candidate_slots": counts["minor"],
    "minor_by_affinity": dict(sorted(minor_factions.items())),
    "world_regions_unchanged": len(nodes),
    "errors": errors,
}
OUT.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
print(json.dumps(result,indent=2))
if errors:
    raise SystemExit(1)
