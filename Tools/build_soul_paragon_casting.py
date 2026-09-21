"""Build the non-UE Paragon casting audit for Soul."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "Data" / "soul_paragon_casting_candidates_20260920.json"

COMMON_SKELETON = (
    "Official Fab pack includes character model plus animations/AnimBP or equivalent animation content. "
    "Exact package and skeleton paths remain PAYLOAD_PENDING until downloaded."
)

def row(name, listing, factions, archetype, fit, hide_replace, redundancy, hero_role,
        local=False, catalog_id=None, hero_eligible=True, notes=None):
    return {
        "name": name,
        "ownership": "ACQUIRED_AND_LOCAL" if local else "SCREENSHOT_CONFIRMED_OWNED_NOT_LOCAL",
        "catalog_id": catalog_id,
        "fab_listing_id": listing,
        "local_cache": "C:/Users/Jeff/Desktop/VaultCache/ParagonCountess" if local else None,
        "likely_factions": factions,
        "archetype": archetype,
        "grounded_fantasy": fit,
        "hide_replace": hide_replace,
        "skeleton_animation": (
            "Local payload present; binary manifest prevented exact package-path/skeleton inventory. "
            "Official pack includes animations and AnimBP."
            if local else COMMON_SKELETON
        ),
        "redundancy": redundancy,
        "hero_role": hero_role,
        "hero_eligible": hero_eligible,
        "qualification": "UE_VISUAL_CONFIRM" if local or fit in {"LOW_TO_MEDIUM","MEDIUM"} else "PAYLOAD_PENDING",
        "notes": notes,
    }

CANDIDATES = [
    row("Countess","0bf014eb-f2ed-4029-adda-81a855eb5220",["Dark"],"martial/hybrid","HIGH",
        "Use a restrained skin and suppress bright ability FX; keep blades only if proportions read as normal weapons.",
        "Overlaps Serath/Morigesh occult space but has a distinct gothic-court silhouette.",
        "vampire noble / assassin commander",local=True),
    row("Greystone","122fd7bf-6f12-4304-a930-cccbbacdaebc",["Humans"],"martial","HIGH",
        "Prefer least-luminous knight skin; remove resurrection/ability glow.",
        "Overlaps Knight_01/Fantasy Hero; strongest as a named castle commander.",
        "knight-lord / frontline commander"),
    row("Sparrow","7d76ddf0-d9ce-4d00-939e-d72793534d01",["Humans"],"ranged","HIGH",
        "Suppress magical-arrow FX and keep the most conventional bow/armor skin.",
        "Unique clean ranged-commander niche among the owned Paragons.",
        "ranger / archer commander"),
    row("Terra","5ea6bcb6-e43e-4bbe-813f-c19d8c907565",["Humans","Vikings"],"martial","HIGH",
        "Keep shield/armor; suppress ability glow and any non-medieval VFX.",
        "Competes with Greystone/Shieldmaiden but offers a strong tank-command silhouette.",
        "shield captain / defensive commander"),
    row("Feng Mao","af344d79-eca6-4b6c-aae9-87c617a27ba1",["Humans"],"martial","MEDIUM_HIGH",
        "Keep spear and practical armor; suppress supernatural FX. Cultural silhouette is the main mismatch.",
        "Competes with Greystone/Kwang/Yin as a foreign human martial hero.",
        "veteran spear-master / guardian"),
    row("Kwang","f4c67e92-b976-4b5b-ab9f-4c25b010f6f3",["Humans"],"martial/hybrid","MEDIUM_HIGH",
        "Keep the grounded warrior body; tame electrical FX and avoid any overly ornate sword treatment.",
        "Overlaps Feng Mao/Yin in an East-Asian martial niche.",
        "sword-sage / storm champion"),
    row("Yin","dc21702b-7f1e-4aa5-a747-78d519f5fb51",["Humans"],"martial","MEDIUM_HIGH",
        "Whip can stay if scale reads practical; suppress energy trails except for discrete abilities.",
        "Distinct weapon silhouette, but culturally overlaps Kwang/Feng Mao.",
        "whip fighter / mobile duelist"),
    row("Aurora","918456eb-4c36-4346-9be2-8986e25c9a0b",["Humans","Dark"],"caster/hybrid","HIGH_WITH_FX_TRIM",
        "Keep medieval armor and ice-blade silhouette; reduce constant ice glow to ability moments.",
        "Overlaps Fantasy Witch/Morigesh as caster but offers a clean armored frost identity.",
        "frost knight / control caster"),
    row("Serath","522b6160-15ab-492b-a2b0-c09f9bb5f6e6",["Dark","Humans"],"martial/hybrid","HIGH_WITH_FX_TRIM",
        "Keep grounded knight body; suppress wings/bright transformation FX except for rare abilities.",
        "Overlaps Greystone as ritual knight and Countess as Dark female hero.",
        "fallen knight / cursed champion"),
    row("Morigesh","29e67175-fa08-448f-822b-37f411530749",["Dark"],"caster","HIGH",
        "Keep swamp-witch body language; trim permanent glow and any oversized magical props.",
        "Overlaps Fantasy Witch/Befouler but is a much stronger named Dark hero silhouette.",
        "swamp witch / curse commander"),
    row("Sevarog","a4882b5e-cfad-4830-a3dd-46a6c31a79b2",["Dark"],"martial/hybrid","HIGH",
        "Keep reaper/spectre silhouette; remove any sci-fi surface treatment and restrain emissives.",
        "Distinct from humanoid Dark heroes; strongest spectral/undead commander option.",
        "reaper lord / terror commander"),
    row("The Fey","9afbcde6-4a14-4018-95c3-2f3a2e1da858",["Nature"],"caster","HIGH",
        "No major technology removal needed; keep nature/spirit styling and restrained magic.",
        "Direct Nature-faction commander by founder direction.",
        "nature spirit / druidic commander",
        notes="Nature hero; do not force into the current five."),
    row("Narbash","d8904a0e-9169-4763-b82b-5fcf864235a4",["Orcs"],"support/martial","HIGH",
        "Keep tribal armor/drum; suppress MOBA-scale ability FX.",
        "Very strong Orc support identity and materially better than inventing an Orc shaman hero.",
        "war drummer / morale-support commander"),
    row("Grux","8c4bac2c-f7f7-4632-a644-47f4e104f5d8",["Orcs"],"martial","HIGH",
        "Keep creature-warrior silhouette; replace or hide any implausibly oversized weapon pieces if needed.",
        "Distinct heavy beastman commander; overlaps brute role but not shaman/support.",
        "beast warlord / shock commander"),
    row("Khaimera","e7c665c1-8c13-42f0-9152-0753008853d7",["Vikings","Orcs"],"martial/hybrid","MEDIUM_HIGH",
        "Keep tribal armor/skull motif; remove glowing ability FX.",
        "Strong tribal identity overlaps Viking Ulf/Orc berserker space; use in one faction only.",
        "cursed hunter / berserker commander"),
    row("Iggy & Scorch","67570f6d-3290-4482-819d-b18853bd8307",["Orcs"],"hybrid","MEDIUM_HIGH",
        "Keep goblin+rider concept; remove scrapyard/industrial props and any comedic-modern equipment.",
        "Unique mounted/duo silhouette; less grounded than Narbash/Grux but highly faction-readable.",
        "goblin beast-rider / disruptive commander"),
    row("Rampage","0807cf74-08fd-4a33-8c8d-f33c9439fb1f",["Orcs","Dark"],"martial/tank","MEDIUM",
        "Strip sci-fi laboratory cues, tech accessories and bright FX; retain the monster body.",
        "Overlaps Grux as a huge creature commander with substantially more conversion burden.",
        "escaped brute / monster commander"),
    row("Wukong","27054d0c-c26e-4fe3-b6f9-fa778dfcb8b6",["Nature"],"martial","HIGH",
        "Keep staff/warrior silhouette; suppress magical clone/ability FX. Nature-faction fit is intentional; retain staff-warrior/trickster identity.",
        "Direct Nature hero by founder direction; complements animal/hybrid regular army.",
        "nature trickster / mobile martial commander"),
    row("Phase","b2c95d5c-a805-460b-a01b-db6da3a778f0",["Dark"],"caster/support","LOW_TO_MEDIUM",
        "Remove cyberpunk clothing/details, modern accessories and sci-fi reads; replace with period-fantasy garments.",
        "Caster/support niche overlaps Morigesh/Fantasy Witch with much higher conversion cost.",
        "psychic/occult support after full fantasy redress"),
    row("Minions","039ea035-9360-4e76-ad06-5d3a92da6f65",[],"troop_pack","LOW_TO_MEDIUM",
        "Not a hero candidate; do not use as a Paragon commander. Technology/style varies by minion type.",
        "Separate troop/donor pack rather than a hero archetype.",
        "not hero-eligible",hero_eligible=False,
        notes="Ownership confirmed by screenshot, but excluded from hero casting."),
]

def main():
    payload = {
        "schema": 2,
        "generated": "2026-09-20",
        "ownership_evidence": {
            "source": "user-supplied Fab My Library screenshots",
            "search": "Paragon",
            "visible_result_count": 21,
            "hero_character_packs": 19,
            "nonhero_character_pack": "Minions",
            "environment_pack": "Paragon: Agora and Monolith Environment",
        },
        "rules": {
            "paragons_are_heroes_not_regular_units": True,
            "undownloaded_owned_candidates_are_valid": True,
            "exact_package_paths_require_local_payload": True,
            "screenshot_ownership_outweighs_older_incomplete_catalog_capture": True,
        },
        "candidates": CANDIDATES,
        "excluded_from_hero_casting": [
            {"name":"Minions","reason":"troop/donor pack, not a named hero"},
            {"name":"Paragon: Agora and Monolith Environment","reason":"environment pack"},
        ],
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    print("wrote", OUT, "candidates", len(CANDIDATES))

if __name__ == "__main__":
    main()
