"""Validate the non-runtime Soul roster/casting outputs."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
roster = json.loads((ROOT/"Data"/"soul_faction_roster_candidates_20260920.json").read_text())
paragon = json.loads((ROOT/"Data"/"soul_paragon_casting_candidates_20260920.json").read_text())
nature = json.loads((ROOT/"Data"/"soul_nature_faction_candidate_20260921.json").read_text())
factions = ["Humans","Dwarves","Vikings","Orcs","Dark"]
expected_buildings = {
    "Humans":{"Muster Yard","Archery Range","Spear Guardhouse","Man-at-Arms Barracks","Witch Collegium","Royal Chapterhouse","Griffon Roost"},
    "Dwarves":{"Stoneguard Hall","Crossbow Workshop","Hammer Hall","Rune Forge","Construct Foundry","King's Guard Hall","Mountain Dragon Eyrie"},
    "Vikings":{"Raider Longhouse","Hunter Range","Shield Hall","Berserker Mead Hall","Shaman Lodge","Huscarl Hall","Wolf Kennels"},
    "Orcs":{"Grunt Barracks","Hunter Range","Shield Pit","Berserker Pit","Brute Hall","Shaman Totem Court","War Elephant Yard"},
    "Dark":{"Black Guard Bastion","Dread Gallery","Execution Court","Demon Gate","Fallen Hall","Befouler Sanctum","Apex Dragon Roost"},
}
errors=[]
for f in factions:
    units=[x for x in roster["preferred_roster"] if x["faction"]==f]
    heroes=[x for x in roster["hero_candidates"] if x["faction"]==f]
    if len(units)!=7: errors.append(f"{f}: expected 7 units, got {len(units)}")
    if not (5 <= len(heroes) <= 10): errors.append(f"{f}: expected 5-10 heroes, got {len(heroes)}")
    if {x["building"] for x in units} != expected_buildings[f]: errors.append(f"{f}: building set mismatch")
    if any("Paragon:" in x["asset"] for x in units): errors.append(f"{f}: Paragon used as regular unit")
    if any(not x["listing_id"] for x in units): errors.append(f"{f}: missing listing/catalog id")

if len(paragon["candidates"]) != 20:
    errors.append(f"Paragon audit expected 20 character packs, got {len(paragon['candidates'])}")
if sum(1 for x in paragon["candidates"] if x.get("hero_eligible", True)) != 19:
    errors.append("Paragon audit expected 19 hero-eligible character packs")
if not any(x["name"]=="Minions" and not x.get("hero_eligible", True) for x in paragon["candidates"]):
    errors.append("Minions non-hero classification missing")
if not any(x["name"]=="Countess" and x["ownership"]=="ACQUIRED_AND_LOCAL" for x in paragon["candidates"]):
    errors.append("Countess local acquisition evidence missing")

if len(nature["roster"]) != 7:
    errors.append(f"Nature: expected 7 units, got {len(nature['roster'])}")
centaur = next((x for x in nature["roster"] if x["unit"]=="Centaur Archer"), None)
if not centaur or centaur["slot"] != "ranged":
    errors.append("Nature: Centaur Archer must be the ranged family")
apex = next((x for x in nature["roster"] if x["slot"]=="beast"), None)
if not apex or apex["unit"] != "Apex Beast (species TBD)" or apex["footprint_hexes"] != 3:
    errors.append("Nature: apex beast must remain species-open at 3-hex elephant/dragon tier")
support = next((x for x in nature["roster"] if x["slot"]=="fighter_5_support_magic"), None)
if not support or support["unit"]!="Cheetah Warrior" or "magic/support" not in support["combat_role"] or support["recruitment_site"]!="Druid Circle":
    errors.append("Nature: hooded Cheetah Warrior must be the Druid Circle magic/support fighter")
bear = next((x for x in nature["roster"] if x["unit"]=="Bear Warrior"), None)
bull = next((x for x in nature["roster"] if x["unit"]=="Bull Warrior"), None)
elephant = next((x for x in nature["roster"] if x["unit"]=="Elephant Warrior (anthropomorphic)"), None)
if not bear or "sword" not in bear["combat_role"]:
    errors.append("Nature: Bear Warrior must carry the sword-family role")
if not bull or "spear" not in bull["combat_role"]:
    errors.append("Nature: Bull Warrior must carry the spear-family role")
if not elephant or "heavy-weapon" not in elephant["combat_role"]:
    errors.append("Nature: Elephant Warrior must carry the heavy-weapon role")
nature_heroes = {x["candidate"] for x in nature["hero_candidates"]}
if not {"The Fey","Wukong"}.issubset(nature_heroes):
    errors.append("Nature: The Fey and Wukong hero assignments missing")
for name in ["The Fey","Wukong"]:
    px = next((x for x in paragon["candidates"] if x["name"]==name), None)
    if not px or "Nature" not in px["likely_factions"]:
        errors.append(f"Paragon: {name} must map to Nature")

if errors:
    print("\n".join("ERROR: "+x for x in errors))
    raise SystemExit(1)
print("PASS: current five validated; Nature has Sword/Spear/Heavy/Cheetah-Magic-Support/Flex + Centaur Archer + species-open Apex Beast; Fey/Wukong mapped to Nature; no Paragon regular troops")
