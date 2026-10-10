#!/usr/bin/env python3
"""
Soul SETTLEMENT_ENVIRONMENT_REGISTRY builder.
Read-only research lane. Emits settlement_environment_registry.json and .csv
into this directory only. Every /Game/... path carries an evidence citation
(repo file + key) and an evidence tier. No Soul Source/Config/Content/.uproject
or existing maps/assets are modified.
"""
import json, csv, os

HERE = os.path.dirname(os.path.abspath(__file__))

# ---------------------------------------------------------------------------
# Copperlight ownership cross-reference (verified in catalog/products.json and
# catalog/raw/epic_library.json). listing_id values appear in epic_library.json;
# the ProductName records with ownership/local/import/license fields are in
# products.json. LicenseStatus is UNKNOWN for ALL six donors -> licensing caveat.
# ---------------------------------------------------------------------------
COPPERLIGHT = {
    "826f1b55-01e2-4272-993d-ea142a264f9f": {
        "product_name": "Modular Castle (Medieval Castle, Medieval Town, Medieval Fortress, Castle)",
        "publisher": "Hivemind", "owned": True, "owned_confidence": "HIGH",
        "locally_available": None, "imported": False, "license_status": "UNKNOWN",
        "provenance": "OWNERSHIP_CONFIRMED",
        "copperlight_id": "fab_6d9352d40acf4a2c8f68113d924944b8",
        "evidence": "Copperlight-Asset-Catalog/catalog/products.json (ProductName 'Modular Castle...'); listing_id in catalog/raw/epic_library.json"},
    "b91e0942-7668-44cc-8b3a-6b977bd831ee": {
        "product_name": "Modular Legendary Forge",
        "publisher": "Macbry", "owned": True, "owned_confidence": "HIGH",
        "locally_available": None, "imported": False, "license_status": "UNKNOWN",
        "provenance": "OWNERSHIP_CONFIRMED",
        "copperlight_id": "fab_bf013eb27085437f9c071cf318019c26",
        "evidence": "Copperlight-Asset-Catalog/catalog/products.json (ProductName 'Modular Legendary Forge'); listing_id in catalog/raw/epic_library.json"},
    "fd908d45-b194-4a7d-97c9-942c0cd22095": {
        "product_name": "Modular Water City Environment (Water Village,Town,Seaside)",
        "publisher": "JustB Studios", "owned": True, "owned_confidence": "HIGH",
        "locally_available": None, "imported": False, "license_status": "UNKNOWN",
        "provenance": "OWNERSHIP_CONFIRMED",
        "copperlight_id": "fab_cff1623a9db641e1bc2c0b948835b41d",
        "evidence": "Copperlight-Asset-Catalog/catalog/products.json (ProductName 'Modular Water City Environment...'); listing_id in catalog/raw/epic_library.json"},
    "2b14fc54-691f-4a37-8045-bef78d8b1ebc": {
        "product_name": "Medieval Ruins",
        "publisher": "LAYA DESIGN", "owned": True, "owned_confidence": "HIGH",
        "locally_available": True, "imported": True, "license_status": "UNKNOWN",
        "provenance": "IMPORTED",
        "copperlight_id": "fab_41faeeed3014466d9861374190d4f438",
        "evidence": "Copperlight-Asset-Catalog/catalog/products.json (ProductName 'Medieval Ruins'); listing_id in catalog/raw/epic_library.json"},
    "454baa93-0c78-4a53-a57e-47943160d591": {
        "product_name": "Fantasy Alien Castle",
        "publisher": "Leartes Studios", "owned": True, "owned_confidence": "HIGH",
        "locally_available": None, "imported": False, "license_status": "UNKNOWN",
        "provenance": "OWNERSHIP_CONFIRMED",
        "copperlight_id": "fab_90c707355cf24c00aa79c6dfd77aba99",
        "evidence": "Copperlight-Asset-Catalog/catalog/products.json (ProductName 'Fantasy Alien Castle'); listing_id in catalog/raw/epic_library.json"},
    "d8d7b289-dfed-4236-97ab-941c2c6c3790": {
        "product_name": "Fantasy Forest Village Kit",
        "publisher": "LAYA DESIGN", "owned": True, "owned_confidence": "HIGH",
        "locally_available": True, "imported": True, "license_status": "UNKNOWN",
        "provenance": "IMPORTED",
        "copperlight_id": "fab_498c0617a1db4433920954a3b60ec011",
        "evidence": "Copperlight-Asset-Catalog/catalog/products.json (ProductName 'Fantasy Forest Village Kit'); listing_id in catalog/raw/epic_library.json"},
}

# Secondary / support owned environment packs (Copperlight products.json, verified)
SECONDARY = {
    "ravenhold": {"name": "Ravenhold: Modular Medieval Castle, Fortress & Dungeon Megapack",
        "publisher": "Hivemind", "owned": True, "locally_available": None, "imported": False,
        "license_status": "UNKNOWN", "provenance": "OWNERSHIP_CONFIRMED", "id": "fab_313593e1b9c74b928b13d97ac6b4bb30",
        "note": "Siege-damage donor: clean+ruined CurtainWall_10m / Lrg_Wall_10m / ruined bridges. Scene HM-FortCastle_Kit_Demo.umap per Docs audit (LOCAL VERIFIED in prior non-UE audit; Copperlight shows LocallyAvailable=null -> on-disk payload needs RD confirm)."},
    "viking_village": {"name": "Modular Viking Village (Hivemind)",
        "publisher": "Hivemind", "owned": True, "locally_available": True, "imported": True,
        "license_status": "UNKNOWN", "provenance": "IMPORTED", "id": "fab_9a2467f766654e4fb5f3da9875105bab",
        "note": "Viking cultural kit; LV_MainVillage.umap, BP_HouseBuilding_001/002/003."},
    "coastal_ruins": {"name": "Elite Landscapes: Coastal Ruins",
        "publisher": "Velarion", "owned": True, "locally_available": True, "imported": False,
        "license_status": "UNKNOWN", "provenance": "OWNERSHIP_CONFIRMED", "id": "fab_eaa63e51c32548f2b65ede6be3cc5e3f",
        "note": "CoastalRuins_01..04.umap; neutral/coastal/ruined battlefields and Orc/bridge donor."},
    "ancient_mountain": {"name": "Mountain (Ancient Mountain, Shrine, Mountain Environment)",
        "publisher": "Hivemind", "owned": True, "locally_available": True, "imported": False,
        "license_status": "UNKNOWN", "provenance": "OWNERSHIP_CONFIRMED", "id": "fab_155f959c44724060b362df0ad8e295c2",
        "note": "Shrine/landmark donor for Nature ancient_shrine and neutral mountain_shrine."},
    "kingdom_capital": {"name": "Fantasy Kingdom Capital Kit",
        "publisher": "LAYA DESIGN", "owned": True, "locally_available": None, "imported": False,
        "license_status": "UNKNOWN", "provenance": "OWNERSHIP_CONFIRMED", "id": "fab_8c14fe4c4753409892a69eeb17efc0f5",
        "note": "Human alternate / landmark capital donor; do NOT replace mechanically-modular Hivemind fortress."},
    "townsmith": {"name": "Townsmith: Modular Medieval Town",
        "publisher": "Hivemind", "owned": True, "locally_available": True, "imported": False,
        "license_status": "UNKNOWN", "provenance": "OWNERSHIP_CONFIRMED", "id": "fab_0eba8defe69f4d4589c3f12022c34fcf",
        "note": "Secondary human/medieval town donor for houses/halls/town dressing and secondary towns."},
}

TIER_NOTE = {
    "LOCAL_EVIDENCE_DEFAULT": "Path/prefab asserted by Soul local evidence; on-disk read in prior non-UE audit. Final UE-cache confirm is a Remote-Desktop task.",
    "UE_VISUAL_CONFIRM": "Real candidate exists; exact assignment needs in-editor visual pass (Gate 1).",
    "UE_VISUAL_PICK": "Multiple real candidates; only an editor view picks the correct one.",
    "COMPOSITE": "Assemble from owned pieces; no new modeled asset implied.",
    "PAYLOAD_PENDING": "Fab ownership confirmed but UE payload/showcase map name not expanded locally; needs Remote-Desktop qualification.",
    "LOCAL_VERIFIED": "Files/maps/prefabs inspected on disk in prior non-UE audit; UE-cache re-confirm is a Remote-Desktop task.",
    "READY_FOR_UE": "Donor map local-verified and ready for in-editor battlefield crop.",
    "UE_CROP_REQUIRED": "Donor map exists; needs an in-editor bounded combat-zone crop.",
    "UE_COMPOSITE": "Battlefield assembled from more than one owned donor; needs editor composition.",
}

def cl(listing_id):
    return COPPERLIGHT[listing_id]

# ---------------------------------------------------------------------------
# Settlement records: 15 required fields + alternates + evidence + tiers.
# ---------------------------------------------------------------------------
SETTLEMENTS = [
 {
  "settlement_id": "city.human_capital",
  "name": "Human Capital (Hivemind Fortress)",
  "faction": "humans",
  "enabled": True,
  "rank": 1,
  "donor_listing_id": "826f1b55-01e2-4272-993d-ea142a264f9f",
  "best_visit_environment": {
    "path": "/Game/Medieval_Megapack/Levels/PL_Fortress_Day",
    "evidence_tier": "LOCAL_VERIFIED",
    "evidence": "Soul/Data/environment_asset_bindings.json humans.base_map; Soul/Data/city_siege_blueprints.json city.human_capital.town_view.base; Soul/Evidence/EnvironmentCaptures/safe_capture_manifest.json (load_ok=true, span 50400, anchor LandscapeBounds)"},
  "best_siege_environment": {
    "path": "/Game/Medieval_Megapack/Levels/PL_Fortress_Day",
    "evidence_tier": "LOCAL_VERIFIED",
    "evidence": "Soul/Data/city_siege_blueprints.json city.human_capital.siege (layers outer_fields->gate_and_walls->lower_courtyard->inner_keep; objectives gatehouse/forge/witch_collegium/keep); damage via Ravenhold ruined walls where material-compatible"},
  "best_field_battle_environment": {
    "path": "/Game/Medieval_Megapack/Levels/PL_Fortress_Day (exterior zone) ; LandscapePackTwo/Maps/Grassland_01.umap",
    "evidence_tier": "READY_FOR_UE",
    "evidence": "Soul/Data/battlefield_recipes.json human.fortress_outskirts (status READY_FOR_UE) and human.grassland_crossroads (READY_FOR_UE)"},
  "alternates": [
    {"path": "/Game/Medieval_Megapack/Levels/PL_Fortress_Day1", "status": "DO_NOT_USE",
     "evidence_tier": "LOCAL_VERIFIED",
     "evidence": "Soul/Docs/ENVIRONMENT_ASSET_AUDIT_20260920.md 'Hivemind UE qualification update': PL_Fortress_Day1 is locally corrupt/unloadable (failed package name-table seek). Must not be used."},
    {"path": "Fantasy Kingdom Capital Kit (fab_8c14fe4c4753409892a69eeb17efc0f5)", "status": "PAYLOAD_PENDING",
     "evidence_tier": "PAYLOAD_PENDING",
     "evidence": "Soul/Docs/ENVIRONMENT_ASSET_AUDIT_20260920.md 'Human alternates'; Copperlight products.json 'Fantasy Kingdom Capital Kit' (LAYA DESIGN, Owned HIGH, Local null). Landmark alternate only; do NOT replace modular Hivemind fortress."},
    {"path": "Townsmith: Modular Medieval Town (fab_0eba8defe69f4d4589c3f12022c34fcf)", "status": "SECONDARY_DONOR",
     "evidence_tier": "OWNERSHIP_CONFIRMED",
     "evidence": "Copperlight products.json 'Townsmith: Modular Medieval Town' (Hivemind, Owned HIGH, Local true). Secondary human town donor."}],
  "full_demo_or_kit": "Full authored demo map (PL_Fortress_Day) backed by a modular prefab/level kit (Building_A..D, Forge, Tavern prefabs).",
  "visual_style": "Ordered medieval stone castle/capital; realistic PBR, scanned foliage, daytime lighting.",
  "walkability": "High - full authored environment loads (span 50400, LandscapeBounds anchor); courtyards/streets/wall-walks support town-view and siege movement.",
  "interiors": "Yes - prefab buildings have interiors (Forge/Tavern/Building_A..D); interiors are tactical spaces per locked interior-interaction scope.",
  "streets": "Yes - courtyards, market quarter and gate approach provide street-level space.",
  "walls_gates": "Yes - explicit curtain walls, gatehouse, towers, portcullis and wall-walk modular pieces (architecture_keep list).",
  "nav_pathing_evidence": "safe_capture_manifest.json confirms map load_ok with LandscapeBounds; two PNG captures (human_hivemind_overview/perspective). No baked NavMesh dump in repo -> nav is Remote-Desktop confirm.",
  "technical_burden": "MEDIUM once remediated / HIGH if shipped raw. Pathological Diner_Colection micro-props dominate memory+build (SM_Cheese_Var1 5193MB; SM_WoodCup 4455MB/185.74s build; SM_WineBottles_Var3 3403MB/157.19s; SM_SilverCandle 3256MB; SM_SilverCup 2159MB/84.59s; SM_Cheese_Board 1534MB/54.58s). Remediation = author removal of pathological props + RB Optimization/HLOD on retained architecture.",
  "ranking_reason": "Best current production base: real local-verified authored map + modular prefab/level kit means town growth = actual level instances, not icons. Only faction with hard UE load evidence. Rank 1.",
  "notes": "Human Capital 'limited slice' is a performance-driven reduced exposure, NOT a missing environment. See HUMAN_CAPITAL_DIAGNOSIS.md. Do not modify the environment."
 },
 {
  "settlement_id": "city.viking_harbour",
  "name": "Viking Harbour / Capital",
  "faction": "vikings",
  "enabled": True,
  "rank": 2,
  "donor_listing_id": "fd908d45-b194-4a7d-97c9-942c0cd22095",
  "best_visit_environment": {
    "path": "/Game/JustBStudios/Water_City/Levels/LV_WaterVillage",
    "evidence_tier": "LOCAL_VERIFIED",
    "evidence": "Soul/Data/environment_asset_bindings.json vikings.base_maps[0]; Soul/Data/settlement_blueprints.json vikings.donor (status LOCAL_VERIFIED); Soul/Docs/ENVIRONMENT_ASSET_AUDIT_20260920.md Water City LOCAL VERIFIED (LV_WaterVillage/LV_Overview/LV_DayLighting/LV_NightLighting)"},
  "best_siege_environment": {
    "path": "/Game/JustBStudios/Water_City/Levels/LV_WaterVillage + /Game/Viking_Village/Levels/MainVillage/LV_MainVillage",
    "evidence_tier": "LOCAL_VERIFIED",
    "evidence": "Soul/Data/city_siege_blueprints.json city.viking_harbour.town_view.base & siege (shore_and_docks->bridge_or_palisade->lower_village->upper_great_hall; objectives bridge_watch/shipyard/shaman_lodge/great_hall)"},
  "best_field_battle_environment": {
    "path": "/Game/JustBStudios/Water_City/Levels/LV_WaterVillage.umap (outer harbour crop)",
    "evidence_tier": "READY_FOR_UE",
    "evidence": "Soul/Data/battlefield_recipes.json viking.harbour_edge (READY_FOR_UE); also viking.snow_pass SnowyMountain_01 and viking.forest_track LV_MainVillage outskirts"},
  "alternates": [
    {"path": "/Game/Viking_Village/Levels/MainVillage/LV_MainVillage", "status": "SECONDARY_DONOR_LOCAL",
     "evidence_tier": "LOCAL_VERIFIED",
     "evidence": "Soul/Data/environment_asset_bindings.json vikings.base_maps[1]; Copperlight products.json 'Modular Viking Village' (Hivemind, Owned HIGH, Local true, Imported true). Cultural overwrite kit."},
    {"path": "/Game/JustBStudios/Water_City/Levels/LV_Overview", "status": "ALT_LIGHTING_MAP",
     "evidence_tier": "LOCAL_VERIFIED",
     "evidence": "Soul/Docs/ENVIRONMENT_ASSET_AUDIT_20260920.md Water City authored maps list."}],
  "full_demo_or_kit": "Full authored demo map (LV_WaterVillage) + second authored village map (LV_MainVillage) + ready-made house blueprints/piers/bridges.",
  "visual_style": "Cold-coast wooden harbour village climbing cliffs; piers/bridges; Viking cultural redress on Water City shells.",
  "walkability": "High - docks/piers/bridges/houses authored for traversal; silhouette docks->bridge->great hall.",
  "interiors": "Yes - Water City large/medium/small house blueprints and Viking houses have interiors.",
  "streets": "Yes - harbour lanes, bridges and village paths.",
  "walls_gates": "Partial - palisade/watchtower/bridge-choke rather than stone curtain walls (BP_Watchtower, BP_WoodTower, palisade watch).",
  "nav_pathing_evidence": "viking_house_blueprint_inventory.json documents Viking house BP inventory. No baked NavMesh in repo -> Remote-Desktop confirm.",
  "technical_burden": "MEDIUM - two packs composited (Water City geography + Viking Village culture); timber modules, water rendering. No pathological micro-prop audit flagged.",
  "ranking_reason": "Two LOCAL_VERIFIED authored maps plus ready-made house/pier/bridge blueprints; strongest non-human environment with real local evidence. Rank 2.",
  "notes": "Copperlight ownership confirmed; LicenseStatus UNKNOWN. Water City Copperlight LocallyAvailable=null but Soul audit marks LOCAL_VERIFIED -> reconcile on Remote Desktop."
 },
 {
  "settlement_id": "city.orc_ruinhold",
  "name": "Orc Capital / Ruinhold",
  "faction": "orcs",
  "enabled": True,
  "rank": 3,
  "donor_listing_id": "2b14fc54-691f-4a37-8045-bef78d8b1ebc",
  "best_visit_environment": {
    "path": "SHOWCASE_TO_QUALIFY (Medieval Ruins showcase map; exact /Game/... name not in repo)",
    "evidence_tier": "PAYLOAD_PENDING",
    "evidence": "Soul/Data/settlement_blueprints.json orcs.donor.primary_map='SHOWCASE_TO_QUALIFY' status ACQUIRED_PAYLOAD_PENDING_UE_CACHE; Copperlight products.json 'Medieval Ruins' (LAYA DESIGN, Owned HIGH, Local TRUE, Imported TRUE, IMPORTED) -> payload is actually imported/local, so showcase map name is recoverable on Remote Desktop"},
  "best_siege_environment": {
    "path": "Medieval Ruins showcase + Ravenhold ruined pieces (CurtainWall_10m_Ruined / Lrg_Wall_10m_Ruined) + YI_BanditCamp tents",
    "evidence_tier": "PAYLOAD_PENDING",
    "evidence": "Soul/Data/city_siege_blueprints.json city.orc_ruinhold.siege (broken_outer_city->patched_palisade->war_camp->ritual_quarter->warchief_citadel; objectives elephant_gate/war_drum_tower/shaman_court/warchief_hall); Soul/Data/environment_asset_bindings.json ravenhold ruined families"},
  "best_field_battle_environment": {
    "path": "LandscapePackTwo/Maps/Mesa_01.umap (badlands, READY_FOR_UE); orc.ruined_field = Medieval Ruins showcase (PAYLOAD_PENDING)",
    "evidence_tier": "READY_FOR_UE",
    "evidence": "Soul/Data/battlefield_recipes.json orc.badlands (READY_FOR_UE, Mesa_01); orc.ruined_field (PAYLOAD_PENDING); orc.war_camp (UE_COMPOSITE CoastalRuins_02 + BanditCamp); orc.broken_bridge (UE_COMPOSITE Ravenhold + CoastalRuins_03)"},
  "alternates": [
    {"path": "Elite_CoastalRuins/Maps/CoastalRuins_01..04.umap", "status": "SUPPORT_DONOR_LOCAL",
     "evidence_tier": "LOCAL_VERIFIED",
     "evidence": "Soul/Data/battlefield_recipes.json neutral.coastal_ruins (LOCAL_VERIFIED); Copperlight products.json 'Elite Landscapes: Coastal Ruins' (Velarion, Owned HIGH, Local true)."},
    {"path": "Ravenhold HM-FortCastle ruined geometry", "status": "SUPPORT_DONOR",
     "evidence_tier": "LOCAL_VERIFIED",
     "evidence": "Soul/Docs/ENVIRONMENT_ASSET_AUDIT_20260920.md Ravenhold ruined variants; Copperlight 'Ravenhold ... Megapack' (Hivemind, Owned HIGH, Local null)."}],
  "full_demo_or_kit": "Showcase map (to qualify) + composited support kits (Ravenhold ruins, BanditCamp tents, Coastal Ruins terrain).",
  "visual_style": "Occupation grammar: ancient medieval/gothic masonry ruins + crude Orc timber patches, tents, totems, beast pens, fires. Never a clean rebuilt castle.",
  "walkability": "Medium-High expected - ruined open courtyards favour large beast lanes (War Elephant); exact walkable extents need UE.",
  "interiors": "Partial - surviving ruin halls/towers reused as structures; mostly broken/open.",
  "streets": "Yes - broken outer-city streets / collapsed side streets (siege approach 'collapsed_side_street').",
  "walls_gates": "Yes but ruined/patched - persistent breach grammar; elephant_gate objective; Ravenhold ruined walls for breaches.",
  "nav_pathing_evidence": "None in repo (payload pending). Remote-Desktop confirm of showcase map + nav.",
  "technical_burden": "MEDIUM - Medieval Ruins is IMPORTED/local per Copperlight so payload exists; composite dressing is the main cost. No pathological-prop audit yet.",
  "ranking_reason": "Copperlight shows Medieval Ruins IMPORTED+LocallyAvailable (matches Soul's local evidence), so the exact showcase map is recoverable quickly on Remote Desktop; strong thematic fit. Rank 3.",
  "notes": "Soul status flag says ACQUIRED_PAYLOAD_PENDING_UE_CACHE but Copperlight says IMPORTED/local -> the single most actionable reconciliation for Codex: open the imported Medieval Ruins content, record the exact showcase /Game/... map."
 },
 {
  "settlement_id": "city.dwarf_hold",
  "name": "Dwarf Hold",
  "faction": "dwarves",
  "enabled": True,
  "rank": 4,
  "donor_listing_id": "b91e0942-7668-44cc-8b3a-6b977bd831ee",
  "best_visit_environment": {
    "path": "PACK_SHOWCASE_TO_QUALIFY (Modular Legendary Forge showcase; exact /Game/... name not in repo)",
    "evidence_tier": "PAYLOAD_PENDING",
    "evidence": "Soul/Data/settlement_blueprints.json dwarves.donor.primary_map='PACK_SHOWCASE_TO_QUALIFY' status ACQUIRED_PAYLOAD_PENDING_UE_CACHE; Soul/Evidence/environment_plan_validation_20260920.txt WARN 'dwarves: primary environment payload still needs UE/cache qualification'; Copperlight 'Modular Legendary Forge' (Macbry, Owned HIGH, Local null, OWNERSHIP_CONFIRMED)"},
  "best_siege_environment": {
    "path": "Legendary Forge showcase (to qualify) + LandscapePackOne Mountain/SnowyMountain terrain",
    "evidence_tier": "PAYLOAD_PENDING",
    "evidence": "Soul/Data/city_siege_blueprints.json city.dwarf_hold.siege (mountain_approach->outer_works->gate_tunnel->forge_district->inner_hold; objectives mountain_gate/great_forge/rune_forge/kings_hall)"},
  "best_field_battle_environment": {
    "path": "LandscapePackOne/Maps/Mountain_01.umap (READY_FOR_UE); SnowyMountain_03.umap; Mountain_04.umap",
    "evidence_tier": "READY_FOR_UE",
    "evidence": "Soul/Data/battlefield_recipes.json dwarf.mountain_pass (READY_FOR_UE Mountain_01), dwarf.snow_basin (READY_FOR_UE SnowyMountain_03), dwarf.high_quarry (READY_FOR_UE Mountain_04); dwarf.forge_approach (PAYLOAD_PENDING)"},
  "alternates": [
    {"path": "Ancient Mountain / Shrine (fab_155f959c44724060b362df0ad8e295c2)", "status": "TERRAIN/LANDMARK_SUPPORT",
     "evidence_tier": "LOCAL_VERIFIED",
     "evidence": "Soul/Data/settlement_blueprints.json dwarves.donor.terrain_support; Copperlight 'Mountain (Ancient Mountain, Shrine...)' (Hivemind, Owned HIGH, Local true)."}],
  "full_demo_or_kit": "Modular kit + showcase map (106 meshes, modular walls/floors/roofs, indoor+outdoor forge, lava/molten, smoke/snow, damage decals per audit). Showcase map name pending.",
  "visual_style": "Vertical mountain forge-city; molten Great Forge centerpiece; lava/smoke/snow; engineered-into-terrain.",
  "walkability": "Medium expected - vertical approach->workshops->forge->hold->eyrie; verticality strong but exact walk extents need UE.",
  "interiors": "Yes - indoor and outdoor forge spaces, modular halls (per audit).",
  "streets": "Partial - artisan quarter/workshops rather than open streets; gate tunnel.",
  "walls_gates": "Yes - mountain gate / gate tunnel; forge architecture.",
  "nav_pathing_evidence": "None in repo (payload pending). Remote-Desktop confirm.",
  "technical_burden": "MEDIUM-HIGH - payload not expanded locally (OWNERSHIP_CONFIRMED, Local null); needs download/import before any UE qualification. Forge FX (lava/smoke) adds GPU cost.",
  "ranking_reason": "Very strong thematic fit and rich modular kit, but payload is OWNERSHIP_CONFIRMED-only (not local) so it needs import before qualification; terrain family is READY_FOR_UE. Rank 4.",
  "notes": "LicenseStatus UNKNOWN. Primary blocker is local payload availability, not asset existence."
 },
 {
  "settlement_id": "city.dark_fortress",
  "name": "Dark Fortress",
  "faction": "dark",
  "enabled": True,
  "rank": 5,
  "donor_listing_id": "454baa93-0c78-4a53-a57e-47943160d591",
  "best_visit_environment": {
    "path": "PREASSEMBLED_SCENE_TO_QUALIFY (Fantasy Alien Castle preassembled scene; exact /Game/... name not in repo)",
    "evidence_tier": "PAYLOAD_PENDING",
    "evidence": "Soul/Data/settlement_blueprints.json dark.donor.primary_map='PREASSEMBLED_SCENE_TO_QUALIFY' status ACQUIRED_PAYLOAD_PENDING_UE_CACHE; Soul/Evidence/environment_plan_validation_20260920.txt WARN 'dark: primary environment payload still needs UE/cache qualification'; Copperlight 'Fantasy Alien Castle' (Leartes Studios, Owned HIGH, Local null, OWNERSHIP_CONFIRMED)"},
  "best_siege_environment": {
    "path": "Fantasy Alien Castle preassembled scene (to qualify) + Ravenhold ruined geometry if material-compatible",
    "evidence_tier": "PAYLOAD_PENDING",
    "evidence": "Soul/Data/city_siege_blueprints.json city.dark_fortress.siege (dead_approach->outer_buttresses->guard_gallery->sacrificial_court->inner_sanctum; objectives ward_spire/demon_gate/dragon_roost/throne_sanctum; art_filter removes technological reads)"},
  "best_field_battle_environment": {
    "path": "LandscapePackTwo/Maps/Desert_01.umap (ash plain, READY_FOR_UE); LandscapePackOne/Maps/Mountain_05.umap (corrupted valley, READY_FOR_UE)",
    "evidence_tier": "READY_FOR_UE",
    "evidence": "Soul/Data/battlefield_recipes.json dark.ash_plain (READY_FOR_UE Desert_01), dark.corrupted_valley (READY_FOR_UE Mountain_05); dark.castle_approach (PAYLOAD_PENDING), dark.ruined_causeway (UE_COMPOSITE)"},
  "alternates": [
    {"path": "Ravenhold ruined/damaged geometry", "status": "DAMAGE_SUPPORT_DONOR_CONDITIONAL",
     "evidence_tier": "LOCAL_VERIFIED",
     "evidence": "Soul/Data/settlement_blueprints.json dark.donor.support_donor 'Ravenhold ruined/damaged geometry if material-compatible'; use only after material/style compatibility test."}],
  "full_demo_or_kit": "Preassembled scene + modular kit (117 meshes + preassembled scene per audit); straddles dark-fantasy / alien-sci-fi.",
  "visual_style": "Otherworldly oppressive fortress; MUST remove technological/sci-fi emissive reads via UE art filter; restrained occult emissive, dark stone, fog, corruption.",
  "walkability": "Medium expected - vertical approach to inner sanctum/dragon roost; exact extents need UE.",
  "interiors": "Yes - lower castle interiors (armory/court) candidates per blueprint.",
  "streets": "Partial - buttressed approach/causeway rather than civic streets.",
  "walls_gates": "Yes - outer buttresses/walls, demon gate, causeway.",
  "nav_pathing_evidence": "None in repo (payload pending). Remote-Desktop confirm.",
  "technical_burden": "HIGH - payload not local (OWNERSHIP_CONFIRMED, Local null) AND mandatory art-direction pass to strip technological reads before it is faction-coherent.",
  "ranking_reason": "Strong silhouette fit but gated twice: needs import AND an art filter pass; terrain family READY_FOR_UE keeps battles unblocked. Rank 5.",
  "notes": "LicenseStatus UNKNOWN. Art filter is a hard prerequisite recorded in city_siege_blueprints.json art_filter and the audit."
 },
 {
  "settlement_id": "city.nature_treehold",
  "name": "Nature Treehold (candidate 6th faction)",
  "faction": "nature_candidate",
  "enabled": False,
  "rank": 6,
  "donor_listing_id": "d8d7b289-dfed-4236-97ab-941c2c6c3790",
  "best_visit_environment": {
    "path": "SHOWCASE_TO_QUALIFY (Fantasy Forest Village showcase; exact /Game/... name not in repo) + user Treefort/Nature map TO LOCATE",
    "evidence_tier": "PAYLOAD_PENDING",
    "evidence": "Soul/Data/settlement_blueprints.json nature_candidate (enabled:false) donor.primary_map='SHOWCASE_TO_QUALIFY'; Copperlight 'Fantasy Forest Village Kit' (LAYA DESIGN, Owned HIGH, Local TRUE, Imported TRUE, IMPORTED) -> showcase map recoverable on Remote Desktop; Treefort map NOT found in either repo"},
  "best_siege_environment": {
    "path": "Fantasy Forest Village + Treefort/Nature map (to locate)",
    "evidence_tier": "PAYLOAD_PENDING",
    "evidence": "Soul/Data/city_siege_blueprints.json city.nature_treehold.siege (forest_approach->root_barrier->ground_village->canopy_bridges->great_tree_core; objectives root_barrier/druid_circle/canopy_bridge/great_tree)"},
  "best_field_battle_environment": {
    "path": "LandscapePackTwo/Maps/Grassland_02.umap (grassland edge, READY_FOR_UE)",
    "evidence_tier": "READY_FOR_UE",
    "evidence": "Soul/Data/battlefield_recipes.json nature.grassland_edge (READY_FOR_UE Grassland_02); nature.forest_clearing/river_woodland/ancient_shrine all PAYLOAD_PENDING"},
  "alternates": [
    {"path": "Ancient Mountain/Shrine (fab_155f959c44724060b362df0ad8e295c2)", "status": "SHRINE_SUPPORT_LOCAL",
     "evidence_tier": "LOCAL_VERIFIED",
     "evidence": "Soul/Data/settlement_blueprints.json nature.ancient_shrine donor; Copperlight 'Mountain (Ancient Mountain, Shrine...)' (Hivemind, Owned HIGH, Local true)."},
    {"path": "user-owned Treefort/Nature map", "status": "NOT_FOUND_IN_REPOS",
     "evidence_tier": "PAYLOAD_PENDING",
     "evidence": "Referenced in settlement_blueprints.json/city_siege_blueprints.json as 'user Treefort/Nature map to locate'; no matching record in Copperlight products.json or Soul repo -> Remote-Desktop local-library search required."}],
  "full_demo_or_kit": "Showcase level + asset showcase + 68 static meshes + procedural foliage/falling leaves/editable prefab actors (per audit). Treefort verticality donor unlocated.",
  "visual_style": "Forest/tree settlement; clearings, platforms, bridges, Great Tree, groves - deliberately NOT a walled castle.",
  "walkability": "Unknown - verticality is the open question; Forest Village may be mostly ground-level, Treefort map needed for canopy verticality.",
  "interiors": "Partial - lodges/prefabs; several 'dwellings' are clearings/groves/platforms rather than buildings.",
  "streets": "No conventional streets - forest trails and river edge.",
  "walls_gates": "No stone walls - root/timber barriers.",
  "nav_pathing_evidence": "None in repo. Remote-Desktop confirm + Treefort map search.",
  "technical_burden": "MEDIUM - Forest Village is IMPORTED/local, but faction is disabled and verticality unproven; foliage/leaves FX cost.",
  "ranking_reason": "Disabled candidate (enabled:false); not part of the current five-faction requirement and gated on locating a Treefort verticality donor. Rank 6.",
  "notes": "Do NOT lock Nature as faction six until Treefort/Forest Village passes the UE verticality test (UE_ENVIRONMENT_EXECUTION_QUEUE Gate 1 / stop conditions)."
 },
]

# Secondary towns / small settlements and neutral environments (practical where owned)
SECONDARY_SETTLEMENTS = [
 {
  "settlement_id": "secondary.human_town",
  "name": "Secondary human town / small settlement",
  "faction": "humans",
  "enabled": True, "rank": 7, "donor_listing_id": None,
  "best_visit_environment": {"path": "Townsmith: Modular Medieval Town (fab_0eba8defe69f4d4589c3f12022c34fcf)",
    "evidence_tier": "OWNERSHIP_CONFIRMED",
    "evidence": "Copperlight products.json 'Townsmith: Modular Medieval Town' (Hivemind, Owned HIGH, Local TRUE); Soul/Docs/ENVIRONMENT_ASSET_AUDIT_20260920.md 'CastleTown/Townsmith remain useful secondary medieval donors'"},
  "best_siege_environment": {"path": "Townsmith houses + Hivemind walls/gate pieces (composite)", "evidence_tier": "COMPOSITE",
    "evidence": "Soul/Docs/ENVIRONMENT_ASSET_AUDIT_20260920.md secondary medieval donors"},
  "best_field_battle_environment": {"path": "LandscapePackTwo/Maps/Grassland_01..02.umap", "evidence_tier": "LOCAL_VERIFIED",
    "evidence": "Soul/Data/battlefield_recipes.json neutral.open_grassland (LOCAL_VERIFIED)"},
  "alternates": [{"path": "Fantasy Kingdom Capital Kit (fab_8c14fe4c4753409892a69eeb17efc0f5)", "status": "LANDMARK_ALT_PAYLOAD_PENDING",
    "evidence_tier": "PAYLOAD_PENDING", "evidence": "Copperlight products.json (LAYA DESIGN, Owned HIGH, Local null)"}],
  "full_demo_or_kit": "Modular medieval town kit (Townsmith) - LocallyAvailable per Copperlight.",
  "visual_style": "Medieval town/village; non-fortress human settlements.",
  "walkability": "Expected high (town kit).", "interiors": "Kit-dependent (needs UE).", "streets": "Yes (town kit).",
  "walls_gates": "Partial - compose from Hivemind wall pieces.",
  "nav_pathing_evidence": "None in repo.", "technical_burden": "LOW-MEDIUM (Townsmith local).",
  "ranking_reason": "Fills the secondary-town gap with an owned, locally-available town kit; no new purchase needed. Rank 7.",
  "notes": "LicenseStatus UNKNOWN."
 },
 {
  "settlement_id": "neutral.fortress_bridge",
  "name": "Neutral legendary fortress / strongholds / bridges",
  "faction": "neutral",
  "enabled": True, "rank": 8, "donor_listing_id": None,
  "best_visit_environment": {"path": "Ravenhold Scenes/HM-FortCastle_Kit_Demo.umap", "evidence_tier": "LOCAL_VERIFIED",
    "evidence": "Soul/Docs/ENVIRONMENT_ASSET_AUDIT_20260920.md Ravenhold primary authored scene; Copperlight 'Ravenhold ... Megapack' (Hivemind, Owned HIGH, Local null)"},
  "best_siege_environment": {"path": "Ravenhold gatehouse/curtain-wall/bridge kit incl ruined variants", "evidence_tier": "LOCAL_VERIFIED",
    "evidence": "Soul/Data/environment_asset_bindings.json ravenhold (CurtainWall_10m / _Ruined, Lrg_Wall_10m / _Ruined, ruined bridges)"},
  "best_field_battle_environment": {"path": "Elite_CoastalRuins/Maps/CoastalRuins_01..04.umap", "evidence_tier": "LOCAL_VERIFIED",
    "evidence": "Soul/Data/battlefield_recipes.json neutral.coastal_ruins (LOCAL_VERIFIED)"},
  "alternates": [{"path": "Ravenhold Scenes/HM-FortCastle_Kit_Asset_Gym.umap", "status": "ASSET_GYM",
    "evidence_tier": "LOCAL_VERIFIED", "evidence": "Soul/Docs/ENVIRONMENT_ASSET_AUDIT_20260920.md Asset gym"}],
  "full_demo_or_kit": "Full authored scene + deep modular fortress/dungeon kit with clean+ruined variants.",
  "visual_style": "Legendary neutral medieval fortress; the canonical persistent-siege DAMAGE donor.",
  "walkability": "High (authored demo scene).", "interiors": "Yes - deep gatehouse/tower/dungeon coverage.", "streets": "Yes.",
  "walls_gates": "Yes - best-in-library walls/gates with explicit ruined variants.",
  "nav_pathing_evidence": "None in repo.", "technical_burden": "MEDIUM (Copperlight Local null -> import needed).",
  "ranking_reason": "The siege-damage backbone for ALL factions (clean->ruined breach grammar) + neutral stronghold/bridge donor. Rank 8.",
  "notes": "Do NOT spend as a routine faction town; reserve as damage/stronghold donor. LicenseStatus UNKNOWN."
 },
]

def flatten_rows():
    rows=[]
    for s in SETTLEMENTS + SECONDARY_SETTLEMENTS:
        clrec = cl(s["donor_listing_id"]) if s.get("donor_listing_id") else {}
        base = {
            "rank": s["rank"],
            "settlement_id": s["settlement_id"],
            "name": s["name"],
            "faction": s["faction"],
            "enabled": s["enabled"],
            "donor_listing_id": s.get("donor_listing_id") or "",
            "donor_product_name": clrec.get("product_name",""),
            "donor_publisher": clrec.get("publisher",""),
            "donor_owned": clrec.get("owned",""),
            "donor_owned_confidence": clrec.get("owned_confidence",""),
            "donor_locally_available": clrec.get("locally_available",""),
            "donor_imported": clrec.get("imported",""),
            "donor_license_status": clrec.get("license_status","UNKNOWN" if clrec else ""),
            "donor_provenance": clrec.get("provenance",""),
            "best_visit_environment_path": s["best_visit_environment"]["path"],
            "best_visit_evidence_tier": s["best_visit_environment"]["evidence_tier"],
            "best_siege_environment_path": s["best_siege_environment"]["path"],
            "best_siege_evidence_tier": s["best_siege_environment"]["evidence_tier"],
            "best_field_battle_path": s["best_field_battle_environment"]["path"],
            "best_field_battle_tier": s["best_field_battle_environment"]["evidence_tier"],
            "alternates": " || ".join(f"{a['path']} [{a.get('status','')}/{a.get('evidence_tier','')}]" for a in s["alternates"]),
            "full_demo_or_kit": s["full_demo_or_kit"],
            "visual_style": s["visual_style"],
            "walkability": s["walkability"],
            "interiors": s["interiors"],
            "streets": s["streets"],
            "walls_gates": s["walls_gates"],
            "nav_pathing_evidence": s["nav_pathing_evidence"],
            "technical_burden": s["technical_burden"],
            "ranking_reason": s["ranking_reason"],
            "notes": s["notes"],
            "visit_evidence": s["best_visit_environment"]["evidence"],
        }
        rows.append(base)
    return rows

def main():
    out_json = {
        "schema": 1,
        "lane": "SETTLEMENT_ENVIRONMENT_REGISTRY",
        "generated_by": "Kiro parallel research lane (read-only)",
        "generated_for": "RefinedBadger Soul / Codex integration lane",
        "source_authorities": {
            "gameplay_source": "github Jgnels/Soul (Data/, Docs/, Evidence/)",
            "asset_intelligence": "github Jgnels/Copperlight-Asset-Catalog (catalog/products.json, catalog/raw/epic_library.json)"
        },
        "evidence_tier_legend": TIER_NOTE,
        "global_licensing_caveat": "All six primary faction donors show LicenseStatus=UNKNOWN in Copperlight products.json. Treat every emitted /Game/... path as license-UNKNOWN pending a Remote-Desktop license review.",
        "ownership_local_split_caveat": "Medieval Ruins + Fantasy Forest Village are IMPORTED/LocallyAvailable (match Soul LOCAL evidence). Modular Castle, Modular Legendary Forge, Modular Water City, Fantasy Alien Castle are OWNERSHIP_CONFIRMED but LocallyAvailable=null (match Soul ACQUIRED_PAYLOAD_PENDING_UE_CACHE / PAYLOAD_PENDING). This split drives the confidence column and the Remote-Desktop confirm flags.",
        "copperlight_donor_crossref": COPPERLIGHT,
        "copperlight_secondary_packs": SECONDARY,
        "settlements": SETTLEMENTS,
        "secondary_and_neutral_settlements": SECONDARY_SETTLEMENTS,
    }
    with open(os.path.join(HERE,"settlement_environment_registry.json"),"w") as f:
        json.dump(out_json,f,indent=2)

    rows = flatten_rows()
    cols = list(rows[0].keys())
    with open(os.path.join(HERE,"settlement_environment_registry.csv"),"w",newline="") as f:
        w=csv.DictWriter(f,fieldnames=cols)
        w.writeheader()
        for r in rows: w.writerow(r)
    print("Wrote JSON and CSV with",len(rows),"flat rows and",len(SETTLEMENTS)+len(SECONDARY_SETTLEMENTS),"settlements.")

if __name__=="__main__":
    main()
