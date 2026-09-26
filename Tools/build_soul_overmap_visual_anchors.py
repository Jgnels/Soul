"""Build visible strategic-map anchors for every Soul overmap region."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORLD = json.loads((ROOT/"Data"/"soul_world_overmap_v1_20260922.json").read_text(encoding="utf-8"))
RECIPES = json.loads((ROOT/"Data"/"battlefield_recipes.json").read_text(encoding="utf-8"))
SLOTS = json.loads((ROOT/"Data"/"soul_overmap_settlement_slots_v1_20260922.json").read_text(encoding="utf-8"))
CITIES = json.loads((ROOT/"Data"/"city_siege_blueprints.json").read_text(encoding="utf-8"))
OUT = ROOT/"Data"/"soul_overmap_visual_anchors_v1_20260922.json"

recipe_by_id = {r["id"]:r for r in RECIPES["recipes"]}
slot_by_region = {s["region_id"]:s for s in SLOTS["slots"]}
city_by_id = {c["id"]:c for c in CITIES["cities"]}

FEATURE_OVERRIDES = {
    "resource": ("resource_infrastructure","quarry cut, ore carts and exposed stone"),
    "river_crossing": ("crossing","visible ford / crossing geometry"),
    "grove": ("resource_landscape","distinct treeline / woodland resource edge"),
    "narrow_pass": ("chokepoint","rock needle, cairn or pass gate"),
    "broken_bridge": ("chokepoint","collapsed bridge silhouette"),
    "road_crossing": ("crossing","road-and-river crossing marker"),
    "crossroads": ("travel_landmark","road junction marker / roadside shrine"),
    "ancient_shrine": ("landmark","ancient shrine / altar"),
    "shrine": ("landmark","mountain shrine / sanctuary"),
    "quarry": ("resource_infrastructure","terraced quarry face and ore handling"),
    "dry_gully": ("terrain_landmark","mesa pillar / gully edge"),
    "dead_ground": ("terrain_landmark","black monolith / ruined altar"),
}
anchors = []
for node in WORLD["nodes"]:
    rid = node["id"]
    slot = slot_by_region.get(rid)
    recipe = recipe_by_id[node["battle_recipe_hint"]]
    anchor = {
        "region_id": rid,
        "display_name": node["name"],
        "macro_region": node["macro_region"],
        "battle_recipe": recipe["id"],
        "battle_recipe_landmark": recipe.get("landmark"),
        "battle_recipe_donor": recipe.get("donor"),
        "founder_slice": rid in WORLD["founder_slice"]["region_ids"],
    }

    if slot and slot["tier"] == "major":
        city = city_by_id[slot["settlement_id"]]
        anchor.update({
            "anchor_type": "major_settlement_silhouette",
            "anchor_label": node["name"],
            "source": city["town_view"]["base"],
            "scale_class": "major",
            "interactive": True,
            "interaction_scope": "enter_settlement",
            "persistent_state_projection": True,
        })
    elif slot and slot["tier"] == "minor":
        anchor.update({
            "anchor_type": "minor_settlement_proxy",
            "anchor_label": slot.get("minor_kind", node["name"]),
            "source": slot["donor"],
            "scale_class": "medium",
            "interactive": True,
            "interaction_scope": "bounded_minor_site",
            "persistent_state_projection": False,
        })
    else:
        override = FEATURE_OVERRIDES.get(node["feature"])
        if override:
            anchor_type,label = override
        elif node["kind"] == "landmark":
            anchor_type,label = "landmark", recipe.get("landmark") or node["name"]
        elif node.get("resource"):
            anchor_type,label = "resource_infrastructure", f"{node['resource']} resource site"
        elif node["landform"] in {"pass","mountain_pass","ravine","causeway"}:
            anchor_type,label = "chokepoint", recipe.get("landmark") or node["feature"]
        else:
            anchor_type,label = "terrain_landmark", recipe.get("landmark") or node["feature"]
        anchor.update({
            "anchor_type": anchor_type,
            "anchor_label": label,
            "source": recipe.get("donor"),
            "scale_class": "small",
            "interactive": node["kind"] == "landmark" or bool(node.get("resource")),
            "interaction_scope": "site_or_encounter" if node["kind"] == "landmark" or node.get("resource") else "visual_navigation",
            "persistent_state_projection": False,
        })

    anchors.append(anchor)

payload = {
    "schema": 1,
    "generated": "2026-09-22",
    "rules": [
        "Every strategic region receives one primary visible navigation anchor.",
        "Major settlements project their actual visitable-city identity rather than generic castle icons.",
        "Minor settlements use bounded/proxy donors, not capital-scale scenes.",
        "Non-settlement anchors inherit the same landmark promise used by their battlefield recipe.",
        "Visual anchors communicate geography; they do not own movement, battle or economy rules.",
    ],
    "anchors": anchors,
}
OUT.write_text(json.dumps(payload,indent=2)+"\n",encoding="utf-8")
print(f"WROTE {OUT} anchors={len(anchors)}")
