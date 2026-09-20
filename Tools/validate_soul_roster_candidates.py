"""Validate the non-runtime Soul roster/casting outputs."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
roster = json.loads((ROOT/"Data"/"soul_faction_roster_candidates_20260920.json").read_text())
paragon = json.loads((ROOT/"Data"/"soul_paragon_casting_candidates_20260920.json").read_text())
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
if errors:
    print("\n".join("ERROR: "+x for x in errors))
    raise SystemExit(1)
print("PASS: 5 factions x 7 units, 5-10 heroes/faction, 20 Paragon character packs/19 hero-eligible, buildings preserved, no Paragon regular troops")
