"""Generate the non-runtime Soul roster/casting candidate data from the 2026-09-20 audit."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "Data" / "soul_faction_roster_candidates_20260920.json"

FIELDS = ["faction","slot","unit","asset","listing_id","package_path","building","combat_role","footprint_hexes","flying","qualification","risk"]
ROSTER = [
"Humans|fighter_1|Castle Militia|Knights Pack / Knight_02|6907d3d8-d0db-43c0-bf9c-8ad43c70e7b3|Content/Knights_Pack/Meshes/Knight_02/Mesh_UE5/Full/SKM_Knight_02_Full_01.uasset|Muster Yard|basic melee fighter|1|no|EVIDENCE_BACKED|Needs low-status material/loadout pass so it does not read as elite",
"Humans|ranged|Castle Archer (provisional)|Knight_03 body + Dark Fantasy Weapons bow|6907d3d8-d0db-43c0-bf9c-8ad43c70e7b3 + 244607ad-e819-4320-bbc7-687b844b5bed|Content/Knights_Pack/Meshes/Knight_03/Mesh_UE5/Full/SKM_Knight_03_Full_01.uasset|Archery Range|ranged physical|1|no|UE_VISUAL_CONFIRM|No owned regular archer body was verified; local Pro Longbow Pack supplies bow actions, but retarget and bow/body cohesion require UE",
"Humans|fighter_2|Spear Guard|Knights Pack / Knight_04|6907d3d8-d0db-43c0-bf9c-8ad43c70e7b3|Content/Knights_Pack/Meshes/Knight_04/Mesh_UE5/Full_Mesh/SKM_Knight_04_Full_01.uasset|Spear Guardhouse|reach melee / anti-large|1|no|EVIDENCE_BACKED|Verify spear animation coverage and formation clipping",
"Humans|fighter_3|Man-at-Arms|Knights Pack / Knight_05|6907d3d8-d0db-43c0-bf9c-8ad43c70e7b3|Content/Knights_Pack/Meshes/Knight_05/Mesh_UE5/Full_Mesh/SKM_Knight_05_Full_01.uasset|Man-at-Arms Barracks|armored melee fighter|1|no|EVIDENCE_BACKED|Avoid the spikiest optional armor pieces",
"Humans|support_magic|Witch Adventurer (fallback: Fantasy Witch)|Fantasy Warriors Pack / Fantasy Witch|df8194f9-ec63-49f6-a885-6feb7a94fcb1||Witch Collegium|support caster / control|1|no|UE_VISUAL_CONFIRM|Exact Witch Adventurer ownership/path was not found; Fantasy Witch is the evidence-backed owned substitute",
"Humans|fighter_4|Royal Guard|Knights Pack / Knight_01|6907d3d8-d0db-43c0-bf9c-8ad43c70e7b3|Content/Knights_Pack/Meshes/Knight_01/Mesh_UE5/Knight_01_Full/SKM_Knight_01_Full_01.uasset|Royal Chapterhouse|elite melee / guard|1|no|EVIDENCE_BACKED|Prefer sword/shield or halberd; suppress banner in normal combat",
"Humans|beast|Griffon|Quadruped Fantasy Creatures / Griffon|52d686b6-1180-4f26-901f-ce3c69a14767||Griffon Roost|flying shock beast|2|yes|UE_VISUAL_CONFIRM|Owned pack verified but current local payload path unresolved; confirm scale and tactical landing footprint",
"Dwarves|fighter_1|Stoneguard|Dwarfs Pack / Bedvar|80643e93-5d54-438c-ae9d-7717242e5a16||Stoneguard Hall|armored melee fighter|1|no|UE_VISUAL_CONFIRM|Pack ownership and named character verified; current payload path unresolved",
"Dwarves|ranged|Crossbow Guard (provisional)|Dwarfs Pack / Orme|80643e93-5d54-438c-ae9d-7717242e5a16||Crossbow Workshop|ranged physical|1|no|UE_VISUAL_CONFIRM|Source evidence does not prove a crossbow loadout; no crossbow animation set was verified locally",
"Dwarves|fighter_2|Hammer Guard|Dwarfs Pack / Broddi|80643e93-5d54-438c-ae9d-7717242e5a16||Hammer Hall|heavy melee / armor break|1|no|UE_VISUAL_CONFIRM|Named character and Epic-skeleton support verified; weapon silhouette still needs visual check",
"Dwarves|support_magic|Runesmith|Dwarfs Pack / Agvid|80643e93-5d54-438c-ae9d-7717242e5a16||Rune Forge|support / rune magic|1|no|UE_VISUAL_CONFIRM|Forge/runesmith read is strong; spell presentation is game-side and needs qualification",
"Dwarves|fighter_3|Runic Golem|Fantasy Characters Pack / Golem|f5816915-86d9-4bef-b8f3-921408ae240b||Construct Foundry|large blocker / construct melee|2|no|UE_VISUAL_CONFIRM|Confirm it reads stone/forge rather than generic fantasy and does not overlap dragon scale",
"Dwarves|fighter_4|King's Guard (provisional)|Fantasy Characters Pack / Dwarf|f5816915-86d9-4bef-b8f3-921408ae240b||King's Guard Hall|elite melee / morale anchor|1|no|UE_VISUAL_CONFIRM|Use a regular dwarf body, not the unique Dwarf King; exact elite guard silhouette requires UE pick",
"Dwarves|beast|Mountain Dragon|Quadruped Fantasy Creatures / Mountain Dragon|52d686b6-1180-4f26-901f-ce3c69a14767||Mountain Dragon Eyrie|apex flying large creature|3|yes|UE_VISUAL_CONFIRM|Owned pack verified but local payload path unresolved; confirm scale and aerial footprint",
"Vikings|fighter_1|Raider|Viking by Art.Hiraeth|ca4ba583-8d90-4069-b51f-50e694530b2f|Content/Viking/Mesh/SK_Viking.uasset|Raider Longhouse|fast melee fighter|1|no|EVIDENCE_BACKED|Custom skeleton/additional bones may require retarget work",
"Vikings|ranged|Hunter (provisional)|Viking Customized modular family + Dark Fantasy Weapons bow|e814c3a3-11ba-4316-a07a-b0980fd016ef + 244607ad-e819-4320-bbc7-687b844b5bed|Content/Vikings/Mesh_UE5/Full/|Hunter Range|ranged skirmisher|1|no|UE_VISUAL_CONFIRM|Viking pack has no bow; local Pro Longbow Pack supplies bow actions, but retarget and bow style/cohesion require UE",
"Vikings|fighter_2|Shieldmaiden|Norse Shield maiden|5ee25a21-08c6-4511-8cd4-eb84a9e6344d|Content/Shieldmaiden/Mesh/SK_ShieldMaiden.uasset|Shield Hall|defensive melee / shield wall|1|no|EVIDENCE_BACKED|Different art source; verify scale/material cohesion beside Bugrimov and Art.Hiraeth characters",
"Vikings|fighter_3|Berserker|Fantasy Characters Pack / Viking Ulf|f5816915-86d9-4bef-b8f3-921408ae240b||Berserker Mead Hall|aggressive melee / damage|1|no|UE_VISUAL_CONFIRM|Strong semantic fit; current payload package path not resolved",
"Vikings|support_magic|Shaman (provisional)|Viking Customized modular family|e814c3a3-11ba-4316-a07a-b0980fd016ef|Content/Vikings/Mesh_UE5/Full/|Shaman Lodge|support caster / debuff|1|no|UE_VISUAL_CONFIRM|No exact owned shaman character was verified; requires grounded staff/talisman/VFX treatment",
"Vikings|fighter_4|Huscarl|Viking Customized modular family|e814c3a3-11ba-4316-a07a-b0980fd016ef|Content/Vikings/Mesh_UE5/Full/|Huscarl Hall|heavy melee / line holder|1|no|UE_VISUAL_CONFIRM|Choose restrained armor configuration; pack has many modular variants and weapons",
"Vikings|beast|Wolf|ANIMAL VARIETY PACK / Wolf|2dd7964c-a601-4264-a53d-465dcae1644c|Content/AnimalVarietyPack/Wolf/Meshes/SK_Wolf.uasset|Wolf Kennels|fast beast / flank|1|no|EVIDENCE_BACKED|26-animation wolf set is cached; verify stack readability at hex scale",
"Orcs|fighter_1|Orc Grunt|14 Orcs Pack / Orc 01|3c224616-e998-433c-910b-32901d426103||Grunt Barracks|basic melee fighter|1|no|UE_VISUAL_CONFIRM|Pack and shared-skeleton family are verified; exact visual role assignment among 14 variants needs review",
"Orcs|ranged|Orc Hunter (provisional)|14 Orcs Pack / Orc 02 + Dark Fantasy Weapons bow|3c224616-e998-433c-910b-32901d426103 + 244607ad-e819-4320-bbc7-687b844b5bed||Hunter Range|ranged skirmisher|1|no|UE_VISUAL_CONFIRM|No bow/crossbow is evidenced in the Orc pack; local Pro Longbow Pack supplies bow actions, but retarget and faction styling require UE",
"Orcs|fighter_2|Shield Orc|14 Orcs Pack / Orc 04|3c224616-e998-433c-910b-32901d426103||Shield Pit|defensive melee / blocker|1|no|UE_VISUAL_CONFIRM|Variant selected only as casting candidate; confirm shield/armor silhouette before lock",
"Orcs|fighter_3|Berserker Orc|14 Orcs Pack / Gnur|3c224616-e998-433c-910b-32901d426103||Berserker Pit|aggressive melee / damage|1|no|UE_VISUAL_CONFIRM|Named variant exists; role mapping is visual hypothesis, not package metadata",
"Orcs|fighter_4|Orc Brute|Fantasy Characters Pack / Orc Hammer|f5816915-86d9-4bef-b8f3-921408ae240b||Brute Hall|heavy melee / armor break|2|no|UE_VISUAL_CONFIRM|Confirm size supports 2-hex treatment and weapon is not visually excessive",
"Orcs|support_magic|Orc Shaman (provisional)|14 Orcs Pack / Magur|3c224616-e998-433c-910b-32901d426103||Shaman Totem Court|support caster / morale|1|no|UE_VISUAL_CONFIRM|No shaman role is documented in the pack; Magur is a named casting hypothesis only",
"Orcs|beast|War Elephant (provisional)|Fantasy Animal Pack / Fantasy Elephant|4878a5f0-f82b-4530-b50d-a3f880ef4665||War Elephant Yard|large shock beast|3|no|UE_VISUAL_CONFIRM|Elephant is owned and rigged; confirm armor/rider treatment reads Orc rather than generic fantasy",
"Dark|fighter_1|Black Guard|Fantasy Warriors Pack / Dark Knight|df8194f9-ec63-49f6-a885-6feb7a94fcb1||Black Guard Bastion|elite melee / guard|1|no|UE_VISUAL_CONFIRM|Strip or replace excessive spikes/emissive materials if present",
"Dark|ranged|Devil|Fantasy Enemies Pack / Devil|79f79e3e-570b-4d96-bc3f-f62107178169||Dread Gallery|ranged spell attacker|1|no|UE_VISUAL_CONFIRM|Verify projectile/VFX remain readable without neon presentation",
"Dark|fighter_2|Executioner|Fantasy Enemies Pack / Executioner|79f79e3e-570b-4d96-bc3f-f62107178169||Execution Court|high-damage melee|1|no|UE_VISUAL_CONFIRM|Check weapon scale against grounded art rule",
"Dark|fighter_3|Demon|Fantasy Enemies Pack / Demon|79f79e3e-570b-4d96-bc3f-f62107178169||Demon Gate|durable melee / terror|2|no|UE_VISUAL_CONFIRM|Confirm silhouette is fantasy rather than stylized or overly emissive",
"Dark|fighter_4|Troll|Fantasy Enemies Pack / Troll|79f79e3e-570b-4d96-bc3f-f62107178169||Fallen Hall|large melee / regeneration|2|no|UE_VISUAL_CONFIRM|Confirm 2-hex footprint and avoid comic proportions",
"Dark|support_magic|Befouler|Fantasy Enemies Pack / Befouler|79f79e3e-570b-4d96-bc3f-f62107178169||Befouler Sanctum|debuff / occult support|1|no|UE_VISUAL_CONFIRM|Confirm readable support silhouette and tame emissive effects",
"Dark|beast|Fantasy Dragon|Fantasy Animal Pack / Fantasy Dragon|4878a5f0-f82b-4530-b50d-a3f880ef4665||Apex Dragon Roost|apex flying creature|3|yes|UE_VISUAL_CONFIRM|Owned pack component verified; current payload package path unresolved and scale must be qualified",
]

ALT_FIELDS = ["faction","slot","candidate","evidence","reason","qualification"]
ALTERNATES = [
"Humans|ranged|Knight_02 or NPC Male body + Dark Fantasy Weapons bow|owned body + owned weapon pack|More grounded body options; local longbow animations exist, but retarget/body cohesion still need UE|UE_VISUAL_CONFIRM",
"Humans|support_magic|Fantasy Witch|Fantasy Warriors Pack df8194f9-ec63-49f6-a885-6feb7a94fcb1|Best verified owned substitute for the intended Witch Adventurer|UE_VISUAL_CONFIRM",
"Humans|fighter_1|Knight_03 low-ornament configuration|cached Knights Pack|Can separate militia/man-at-arms silhouettes if Knight_02 reads too elite|UE_VISUAL_CONFIRM",
"Dwarves|ranged|Generic Dwarf from Fantasy Characters + future crossbow|owned Fantasy Characters Pack|Body is owned but no owned crossbow was verified|UE_VISUAL_CONFIRM",
"Dwarves|ranged|Orme as rune-thrower rather than crossbowman|owned Dwarfs Pack|Preserves owned cast but changes building/role fiction; only use if design approves|UE_VISUAL_CONFIRM",
"Dwarves|fighter_3|Generic Dwarf from Fantasy Characters|owned Fantasy Characters Pack|Fallback if Golem art clashes with dwarf city|UE_VISUAL_CONFIRM",
"Vikings|ranged|Customized Viking + Dark Fantasy Weapons bow|owned body + owned weapon pack|Strongest visual-cohesion route; local longbow actions exist, but exact body/retarget still needs UE|UE_VISUAL_CONFIRM",
"Vikings|ranged|Art.Hiraeth Viking with spear/javelin ranged treatment|cached Viking listing|Avoids bow mismatch but changes hunter fantasy|UE_VISUAL_CONFIRM",
"Vikings|support_magic|Primitive elder / grandfather configuration|Primitive Characters Pack 3ce14ea2-e944-46b1-afec-e10c50e7fa96|Older silhouette may sell shaman better than armored Viking but cohesion is weaker|UE_VISUAL_CONFIRM",
"Vikings|support_magic|Fantasy Witch heavily re-equipped|owned Fantasy Warriors Pack|Mechanically ready caster proxy but female gothic silhouette may not read Norse|UE_VISUAL_CONFIRM",
"Orcs|ranged|Any visually suitable 14 Orcs Pack variant + Dark Fantasy Weapons bow|owned Orc family + owned weapon pack|Keeps faction body language; exact variant must be chosen visually|UE_VISUAL_CONFIRM",
"Orcs|support_magic|Primitive elder re-skinned/re-equipped|owned Primitive Characters Pack|Grounded body fallback only; high faction-cohesion cost|UE_VISUAL_CONFIRM",
"Orcs|support_magic|Fantasy Witch with complete Orc-facing replacement kit|owned Fantasy Warriors Pack|Caster mechanics proxy, but too much visual replacement for preferred route|UE_VISUAL_CONFIRM",
"Dark|ranged|Befouler|owned Fantasy Enemies Pack|Could carry ranged/debuff role if Devil projectile presentation fails|UE_VISUAL_CONFIRM",
"Dark|fighter_3|Demonic Warrior|owned Fantasy Warriors Pack|Fallback if Demon silhouette is too creature-like or emissive|UE_VISUAL_CONFIRM",
"Dark|support_magic|Fantasy Witch|owned Fantasy Warriors Pack|Cleaner humanoid caster fallback if Befouler is visually noisy|UE_VISUAL_CONFIRM",
]

HERO_FIELDS = ["faction","candidate","source","role","grounded_fit","notes","qualification"]
HEROES = [
"Humans|Greystone|Paragon: Greystone|martial|high|Realistic knight/hero; strip supernatural FX and prefer least-luminous skin|UE_VISUAL_CONFIRM",
"Humans|Sparrow|Paragon: Sparrow|ranged|high|Realistic archer; suppress glowing arrows/ability FX|UE_VISUAL_CONFIRM",
"Humans|Fantasy Hero|Fantasy Warriors Pack|martial/hybrid|high|Owned pack candidate; inspect weapon scale and ornament|UE_VISUAL_CONFIRM",
"Humans|Fantasy Hero 02|Fantasy Warriors Pack|martial/hybrid|high|Second owned hero silhouette; useful redundancy only if visually distinct|UE_VISUAL_CONFIRM",
"Humans|Knight_01 Commander|Knights Pack|martial|high|Cached modular knight with sword/shield/halberd options|EVIDENCE_BACKED",
"Humans|NPC King|NPC King And Queen|martial/support|high|Strong court commander candidate; binary manifest blocks exact package path audit|UE_VISUAL_CONFIRM",
"Humans|Fantasy Witch|Fantasy Warriors Pack|caster|medium-high|Grounded caster candidate if clothing/VFX pass avoids JRPG read|UE_VISUAL_CONFIRM",
"Humans|Terra|Paragon: Terra|martial|high|Shield/armor tank silhouette fits Castle with minimal conversion|UE_VISUAL_CONFIRM",
"Humans|Aurora|Paragon: Aurora|caster/hybrid|high|Armored frost hero; keep ice FX ability-bound rather than constantly emissive|UE_VISUAL_CONFIRM",
"Humans|Kwang|Paragon: Kwang|martial/hybrid|medium-high|Grounded warrior body; cultural silhouette is foreign to Castle but not technological|UE_VISUAL_CONFIRM",
"Dwarves|Dwarf King|Dwarfs Pack|martial/support|high|Most natural commander silhouette in owned dwarf family|UE_VISUAL_CONFIRM",
"Dwarves|Agvid|Dwarfs Pack|caster/support|high|Runesmith/forge identity differentiates non-royal hero|UE_VISUAL_CONFIRM",
"Dwarves|Bedvar|Dwarfs Pack|martial|high|Armored veteran option|UE_VISUAL_CONFIRM",
"Dwarves|Broddi|Dwarfs Pack|martial|high|Hammer-warrior option|UE_VISUAL_CONFIRM",
"Dwarves|Orme|Dwarfs Pack|martial|medium-high|Named owned dwarf; visible sword-bearing read conflicts with ranged slot but works as hero|UE_VISUAL_CONFIRM",
"Dwarves|Generic Dwarf|Fantasy Characters Pack|martial|medium-high|Useful alternate body if main pack heroes look too similar|UE_VISUAL_CONFIRM",
"Vikings|Khaimera|Paragon: Khaimera|martial/hybrid|medium-high|Tribal hunter/beast visual fits raider culture better than Castle; strip ability FX|UE_VISUAL_CONFIRM",
"Vikings|Art.Hiraeth Viking|Viking|martial|high|Cached realistic Nordic fighter with axe|EVIDENCE_BACKED",
"Vikings|Customized Viking Commander|Viking Customized|martial|high|16 full-body combinations plus modular equipment; best controllable hero family|UE_VISUAL_CONFIRM",
"Vikings|Shieldmaiden|Norse Shield maiden|martial|high|Sword/shield heroine; art-source cohesion must be checked|EVIDENCE_BACKED",
"Vikings|Viking Ulf|Fantasy Characters Pack|martial|high|Strong berserker/hero silhouette candidate|UE_VISUAL_CONFIRM",
"Vikings|Barbarian|Fantasy Characters Pack|martial|medium-high|Useful alternate if Ulf overlaps regular unit too closely|UE_VISUAL_CONFIRM",
"Vikings|Barbarian Fantasy|Fantasy Characters Pack|hybrid|medium|Likely more fantastical; inspect for oversized weapon/ornament|UE_VISUAL_CONFIRM",
"Vikings|Feng Mao|Paragon: Feng Mao|martial|medium|Realistic spear master, but imperial/East-Asian styling may fight Norse cohesion|UE_VISUAL_CONFIRM",
"Orcs|Azrak|14 Orcs Pack|martial|high|Named owned-pack orc; shared pack skeleton/92-animation family|UE_VISUAL_CONFIRM",
"Orcs|Dardan|14 Orcs Pack|martial|high|Named owned-pack orc; candidate shield/brute commander after visual pass|UE_VISUAL_CONFIRM",
"Orcs|Grux|Paragon: Grux|martial|high|Heavy creature-warrior commander with strong faction read|UE_VISUAL_CONFIRM",
"Orcs|Gnur|14 Orcs Pack|martial|high|Named owned-pack orc; candidate berserker commander|UE_VISUAL_CONFIRM",
"Orcs|Iggy & Scorch|Paragon: Iggy & Scorch|hybrid|medium-high|Goblin beast-rider duo; strip scrapyard/industrial props|UE_VISUAL_CONFIRM",
"Orcs|Magur|14 Orcs Pack|martial/hybrid|medium-high|Named owned-pack orc; shaman role is a hypothesis, not source metadata|UE_VISUAL_CONFIRM",
"Orcs|Ursag|14 Orcs Pack|martial|high|Named owned-pack orc; alternate heavy commander|UE_VISUAL_CONFIRM",
"Orcs|Orc Hammer|Fantasy Characters Pack|martial|high|Distinct heavy-hammer commander; useful if 14-Orcs heroes feel too samey|UE_VISUAL_CONFIRM",
"Orcs|Narbash|Paragon: Narbash|support/martial|high|Excellent tribal drummer/morale commander; stronger support identity than inventing a shaman hero|UE_VISUAL_CONFIRM",
"Dark|Countess|Paragon: Countess|martial/hybrid|high|Gothic vampire/assassin presentation is one of the cleanest Dark hero fits; payload downloaded|UE_VISUAL_CONFIRM",
"Dark|Serath|Paragon: Serath|martial/hybrid|high|Gothic knight/angel-devil duality fits Dark or fallen-knight commander; remove bright holy FX if needed|UE_VISUAL_CONFIRM",
"Dark|Dark Knight|Fantasy Warriors Pack|martial|high|Direct grounded dark-fantasy commander candidate|UE_VISUAL_CONFIRM",
"Dark|Demonic Warrior|Fantasy Warriors Pack|martial/hybrid|medium-high|Strong if weapon/armor silhouette can be kept restrained|UE_VISUAL_CONFIRM",
"Dark|Befouler|Fantasy Enemies Pack|caster|medium-high|Purpose-built occult support silhouette; hero use avoids regular-unit duplication if roster changes|UE_VISUAL_CONFIRM",
"Dark|Executioner|Fantasy Enemies Pack|martial|medium-high|Grounded brutal commander if weapon scale passes|UE_VISUAL_CONFIRM",
"Dark|Morigesh|Paragon: Morigesh|caster|high|Swamp-witch/curse commander is a direct Dark fit|UE_VISUAL_CONFIRM",
"Dark|Sevarog|Paragon: Sevarog|martial/hybrid|high|Spectral reaper gives Dark a distinct non-human commander silhouette|UE_VISUAL_CONFIRM",
"Dark|Fantasy Witch|Fantasy Warriors Pack|caster|high|Cleaner humanoid occult commander than many monster silhouettes|UE_VISUAL_CONFIRM",
"Dark|Aurora|Paragon: Aurora|caster/hybrid|high|Armored frost queen/knight; ability-bound ice FX only|UE_VISUAL_CONFIRM",
]

def parse(rows, fields):
    out = []
    for row in rows:
        values = row.split("|")
        if len(values) != len(fields):
            raise ValueError(f"Bad row ({len(values)} vs {len(fields)}): {row}")
        item = dict(zip(fields, values))
        if "footprint_hexes" in item:
            item["footprint_hexes"] = int(item["footprint_hexes"])
        if "flying" in item:
            item["flying"] = item["flying"].lower() == "yes"
        out.append(item)
    return out

def main():
    payload = {
        "schema": 1,
        "generated": "2026-09-20",
        "status": "NON_UE_CANDIDATE_BOARD",
        "rules": {
            "heroes_separate": True,
            "do_not_lock_ue_visual_confirm": True,
            "owned_asset_evidence_only": True,
            "paragon_regular_units": False,
        },
        "preferred_roster": parse(ROSTER, FIELDS),
        "alternates": parse(ALTERNATES, ALT_FIELDS),
        "hero_candidates": parse(HEROES, HERO_FIELDS),
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    print("wrote", OUT)
    for faction in ["Humans","Dwarves","Vikings","Orcs","Dark"]:
        roster_count = sum(1 for x in payload["preferred_roster"] if x["faction"] == faction)
        hero_count = sum(1 for x in payload["hero_candidates"] if x["faction"] == faction)
        print(f"{faction}: roster={roster_count}, heroes={hero_count}")

if __name__ == "__main__":
    main()
