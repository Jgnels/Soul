"""Build owned-asset surface/donor bindings for the Soul overmap."""
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/"Data"/"soul_overmap_surface_bindings_v1_20260922.json"
DOC=ROOT/"Docs"/"WORLD_OVERMAP_SURFACE_PLAN_20260922.md"

REGIONS={
"northern_fjords":{
 "terrain":["LandscapePack413 SnowyMountain_01/02","Water City cliffs/water"],
 "settlement":["Water City LV_WaterVillage","Modular Viking Village LV_MainVillage"],
 "identity":"cold coast, fjords, boreal routes, snow pass",
 "status":"LOCAL_VERIFIED"},
"crownspine":{
 "terrain":["LandscapePack413 Mountain_01/04","LandscapePack413 SnowyMountain_03"],
 "settlement":["Modular Legendary Forge"],
 "identity":"continuous mountain spine, quarry terraces, engineered passes",
 "status":"TERRAIN_LOCAL_FORGE_PAYLOAD_PENDING"},
"heartland":{
 "terrain":["LandscapePack413 Grassland_01/02","Elite Coastal Ruins river terrain"],
 "settlement":["Medieval_Megapack PL_Fortress_Day"],
 "identity":"temperate roads, farms/grassland, river approaches, fortified capital",
 "status":"LOCAL_VERIFIED"},
"eastern_badlands":{
 "terrain":["LandscapePack413 Mesa_01/02","Elite Coastal Ruins","Ravenhold ruined bridge pieces"],
 "settlement":["Medieval Ruins","YI_BanditCamp support dressing"],
 "identity":"mesa shelves, gullies, occupied ruins, broad War Elephant lanes",
 "status":"TERRAIN_LOCAL_CITY_PAYLOAD_PENDING"},
"greenwood":{
 "terrain":["LandscapePack413 Grassland_02","Fantasy Forest Village","Ancient Mountain/Shrine","Elite Coastal Ruins water"],
 "settlement":["Fantasy Forest Village","Treefort/Nature donor to locate"],
 "identity":"deep forest, river woodland, meadow edges, shrine terraces",
 "status":"MIXED_PAYLOAD_PENDING"},
"ashen_south":{
 "terrain":["LandscapePack413 Desert_01/02","LandscapePack413 Mountain_05","Elite Coastal Ruins/Ravenhold causeway pieces"],
 "settlement":["Fantasy Alien Castle"],
 "identity":"ash plain, corrupted valley, broken causeways, oppressive fortress horizon",
 "status":"TERRAIN_LOCAL_CASTLE_PAYLOAD_PENDING"},
}

RULES=[
 "Author one purpose-built campaign terrain; do not stitch donor showcase levels together literally.",
 "Use donor landscapes for height/material/landform vocabulary and source data, not for arbitrary world borders.",
 "Settlement silhouettes must correspond to the actual visitable settlement donor used by that faction.",
 "Roads, rivers, passes and forest belts are gameplay communication first and decoration second.",
 "Use RB Weather for weather presentation and RB Optimization for repeated/static representation before bespoke systems.",
 "Overmap geography must preserve the same biome/landform/feature promise used to select tactical battlefield recipes.",
]

def main():
    payload={
        "schema":1,
        "generated":"2026-09-22",
        "status":"NON_UE_OWNED_ASSET_BINDING_PLAN",
        "rules":RULES,
        "macro_regions":REGIONS,
        "overmap_scale":"RELATIVE_TOPOLOGY_LOCKED_ABSOLUTE_KM_DEFERRED",
        "ue_gate":"Purpose-built terrain + settlement proxies only after the active realtime-battle lane releases UE.",
    }
    OUT.write_text(json.dumps(payload,indent=2)+"\n",encoding="utf-8")

    lines=[
        "# Soul World Overmap — Surface / Donor Plan — 2026-09-22",
        "",
        "The structural overmap is not a collage of donor showcase maps.",
        "It should be a purpose-built continuous campaign terrain that borrows terrain vocabulary and source data from owned assets.",
        "",
        "## Global rules",
        "",
    ]
    lines += ["- "+x for x in RULES]
    lines += ["", "## Macro-region bindings", ""]
    for key,value in REGIONS.items():
        lines += [
            f"### {key}",
            f"- Identity: {value['identity']}.",
            f"- Status: {value['status']}.",
            "- Terrain donors: " + "; ".join(value["terrain"]) + ".",
            "- Settlement / landmark donors: " + "; ".join(value["settlement"]) + ".",
            "",
        ]
    DOC.write_text("\n".join(lines)+"\n",encoding="utf-8")
    print("WROTE",OUT)
    print("WROTE",DOC)

if __name__=="__main__":
    main()
