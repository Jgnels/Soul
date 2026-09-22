"""Validate Soul's battlefield-environment expansion registry and local first-playtest evidence."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORLD = json.loads((ROOT / "Data" / "soul_world_overmap_v1_20260922.json").read_text(encoding="utf-8"))
PLAN = json.loads((ROOT / "Data" / "soul_battlefield_environment_expansion_v1_20260922.json").read_text(encoding="utf-8"))
OUT = ROOT / "Evidence" / "WorldOvermap" / "battlefield_environment_expansion_validation.json"
REPORT = ROOT / "Evidence" / "WorldOvermap" / "battlefield_environment_expansion_validation.md"

nodes = {n["id"]: n for n in WORLD["nodes"]}
founder = set(WORLD["founder_slice"]["region_ids"])
assets = PLAN["active_assets"]
errors = []

ids = [a["id"] for a in assets]
if len(ids) != len(set(ids)):
    errors.append("duplicate active asset id")
listing_ids = [a["fab_listing_id"] for a in assets if a.get("fab_listing_id")]
if len(listing_ids) != len(set(listing_ids)):
    errors.append("duplicate active Fab listing id")
if len(assets) != 12:
    errors.append(f"expected 12 active assets, got {len(assets)}")
for asset in assets:
    binding = asset.get("strategic_binding", {})
    region_id = binding.get("existing_region_candidate")
    if region_id and region_id not in nodes:
        errors.append(f"{asset['id']}: missing strategic region {region_id}")
    if binding.get("topology_change") is True:
        errors.append(f"{asset['id']}: this pass must not alter topology")
    if asset["integration_class"] == "atmosphere_palette":
        presentation = asset.get("presentation", {})
        if presentation.get("authority") != "RB Weather":
            errors.append(f"{asset['id']}: sky palette escaped RB Weather authority")
        if presentation.get("hard_bind_to_recipe") is not False:
            errors.append(f"{asset['id']}: sky palette must remain dynamically selected")

arena = PLAN["arena_city_state_candidate"]
if arena["region_id"] != "southern_crossing":
    errors.append("arena city-state candidate moved from Southern Crossing")
southern = nodes.get("southern_crossing", {})
if southern.get("owner") is not None:
    errors.append("Southern Crossing is no longer neutral enough for arena city-state candidate")
if "southern_crossing" in founder:
    errors.append("arena city-state must not disturb the founder slice")
if arena.get("topology_change") is not False:
    errors.append("arena city-state candidate must reuse existing topology")

lost = next((a for a in assets if a["id"] == "lost_shrine"), None)
if not lost or lost["strategic_binding"].get("existing_region_candidate") != "ancient_shrine":
    errors.append("Lost Shrine must remain targeted at Ancient Shrine fidelity risk")
deferred = {a["fab_listing_id"]: a for a in PLAN["deferred_assets"]}
if "87483ef2-4f8b-4dd5-94e3-0d654baff737" not in deferred:
    errors.append("Jungle Ruins must remain explicitly deferred by founder decision")

bridge = PLAN["first_playtest_bridge"]
if bridge["target"] != "dragon_graveyard_humans_vs_dwarves":
    errors.append("first environment playtest target drifted")
if bridge["battle_runtime"]["accepted_active_combatants"] != 48:
    errors.append("first playtest must preserve accepted 24v24 baseline")

asset_project = Path(bridge["environment"]["asset_project"])
dragon_maps = [
    asset_project / "Content" / "Dragon_graveyard" / "Level" / "L_showcase_level.umap",
    asset_project / "Content" / "Dragon_graveyard" / "Level" / "L_assets_showcase.umap",
]
missing_dragon = [str(p) for p in dragon_maps if not p.exists()]
if missing_dragon:
    errors.append(f"Dragon Graveyard local map evidence missing: {missing_dragon}")

human_root = asset_project / "Content" / "Knights_Pack"
dwarf_root = asset_project / "Content" / "Dwarf_Pack"
if not human_root.exists():
    errors.append("Knights_Pack missing from asset project")
if not dwarf_root.exists():
    errors.append("Dwarf_Pack missing from asset project")

human_candidates = list(human_root.rglob("SKM_Knight_*Full*.uasset")) if human_root.exists() else []
if not human_candidates and human_root.exists():
    human_candidates = list(human_root.rglob("SKM_Knight*.uasset"))
dwarf_candidates = list(dwarf_root.rglob("SK_Dwarf_*_Full.uasset")) if dwarf_root.exists() else []
if not human_candidates:
    errors.append("no Human Knight skeletal-mesh candidate found locally")
if not dwarf_candidates:
    errors.append("no Dwarf full skeletal-mesh candidate found locally")
classes = {}
for asset in assets:
    classes[asset["integration_class"]] = classes.get(asset["integration_class"], 0) + 1

result = {
    "schema": 1,
    "status": "pass" if not errors else "fail",
    "active_assets": len(assets),
    "integration_classes": dict(sorted(classes.items())),
    "arena_region": arena["region_id"],
    "arena_region_is_neutral": southern.get("owner") is None,
    "founder_slice_unchanged": arena["region_id"] not in founder,
    "dragon_graveyard_maps_present": len(dragon_maps) - len(missing_dragon),
    "human_mesh_candidates_found": len(human_candidates),
    "dwarf_mesh_candidates_found": len(dwarf_candidates),
    "first_playtest_target": bridge["target"],
    "errors": errors,
}
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")

lines = [
    "# Soul Battlefield Environment Expansion Validation",
    "",
    f"Status: **{result['status'].upper()}**",
    "",
    f"- Active owned assets admitted this round: **{result['active_assets']}**.",
    f"- Dragon Graveyard maps present locally: **{result['dragon_graveyard_maps_present']}/2**.",
    f"- Human skeletal-mesh candidates found: **{result['human_mesh_candidates_found']}**.",
    f"- Dwarf skeletal-mesh candidates found: **{result['dwarf_mesh_candidates_found']}**.",
    f"- Arena city-state candidate: **{result['arena_region']}**, without topology change.",
    "",
    "The first visually representative battle target is Dragon Graveyard with Human and Dwarf unit assets.",
]
if errors:
    lines += ["", "## Errors", ""] + [f"- {e}" for e in errors]
REPORT.write_text("\n".join(lines) + "\n", encoding="utf-8")
print(json.dumps(result, indent=2))
if errors:
    raise SystemExit(1)
