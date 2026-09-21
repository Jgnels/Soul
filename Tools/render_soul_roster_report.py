"""Render the Soul faction roster + hero casting audit as Markdown."""
from pathlib import Path
import json
from collections import defaultdict

ROOT = Path(__file__).resolve().parents[1]
ROSTER = json.loads((ROOT/"Data"/"soul_faction_roster_candidates_20260920.json").read_text())
PARAGON = json.loads((ROOT/"Data"/"soul_paragon_casting_candidates_20260920.json").read_text())
NATURE = json.loads((ROOT/"Data"/"soul_nature_faction_candidate_20260921.json").read_text())
OUT = ROOT/"Docs"/"SOUL_FACTION_ROSTER_AND_HERO_CASTING_20260920.md"
FACTIONS = ["Humans","Dwarves","Vikings","Orcs","Dark"]

def esc(v):
    return str(v).replace("|","/").replace("\n"," ")

def table(headers, rows):
    lines = ["| "+" | ".join(headers)+" |", "|"+"|".join(["---"]*len(headers))+"|"]
    lines += ["| "+" | ".join(esc(x) for x in row)+" |" for row in rows]
    return "\n".join(lines)

def main():
    lines = []
    lines += ["# Soul Faction Roster + Hero Casting Audit — 2026-09-20",""]
    lines += ["## Scope and evidence rules",""]
    lines += [
        "- NON-UE audit only. No editor, cook, UBT/UAT or shader work was used.",
        "- Ownership comes from the local asset catalog/current Fab database; public Fab pages are used only to resolve current listing UUIDs and pack contents.",
        "- `EVIDENCE_BACKED` means the local payload/manifest proves the named mesh or animal path.",
        "- `UE_VISUAL_CONFIRM` means ownership/content evidence is good but the final silhouette, scale, retarget, equipment or faction cohesion still needs UE.",
        "- `PAYLOAD_PENDING` means ownership is established but exact local package paths cannot be audited until the payload is downloaded.",
        "- Paragon characters remain heroes/Paragons, never regular unit-family substitutes.",
        "",
    ]
    lines += ["## Preferred seven-unit rosters",""]
    for faction in FACTIONS:
        lines += [f"### {faction}",""]
        rows = []
        for x in ROSTER["preferred_roster"]:
            if x["faction"] != faction: continue
            rows.append([
                x["slot"], x["unit"], x["asset"], x["listing_id"], x["package_path"] or "payload/path pending",
                x["building"], x["combat_role"], f'{x["footprint_hexes"]} hex', "yes" if x["flying"] else "no",
                x["qualification"], x["risk"],
            ])
        lines += [table(["Slot","Unit","Owned asset","Listing ID","Known package path","Recruit building","Combat role","Footprint","Flying","Status","Risk"], rows),""]
        alts = [x for x in ROSTER["alternates"] if x["faction"] == faction]
        if alts:
            lines += ["**Alternates for weak slots**",""]
            for x in alts:
                lines.append(f'- **{x["slot"]}: {x["candidate"]}** — {x["reason"]} `{x["qualification"]}`')
            lines.append("")

    lines += ["## Hero casting board",""]
    for faction in FACTIONS:
        lines += [f"### {faction} heroes",""]
        rows=[]
        for x in ROSTER["hero_candidates"]:
            if x["faction"] != faction: continue
            rows.append([x["candidate"],x["source"],x["role"],x["grounded_fit"],x["qualification"],x["notes"]])
        lines += [table(["Candidate","Owned source","Role","Grounded fit","Status","Casting note"],rows),""]
    lines += ["## Future faction candidate — Nature",""]
    lines += [
        "**Identity:** asset-driven roster from the owned animal-warrior/hybrid packs: four martial animal warriors + hooded Cheetah magic/support + Centaur Archer + one elephant/dragon-tier Apex Beast.",
        "**Hero direction:** The Fey and Wukong are Nature heroes; human/animal hybrids expand the hero pool.",
        "**Support structure:** the hooded Cheetah Warrior is the magic/support fighter; its existing silhouette is the reason for the role assignment.",
        "",
    ]
    rows=[]
    for x in NATURE["roster"]:
        fly = "TBD" if x["flying"] is None else ("yes" if x["flying"] else "no")
        rows.append([x["slot"],x["unit"],x["source"],x["recruitment_site"],x["combat_role"],f'{x["footprint_hexes"]} hex',fly,x["qualification"],x["risk"]])
    lines += [table(["Slot","Unit","Owned/source evidence","Recruitment site","Combat role","Footprint","Flying","Status","Risk"],rows),""]
    lines += ["### Nature hero candidates",""]
    rows=[]
    for x in NATURE["hero_candidates"]:
        rows.append([x["candidate"],x["source"],x["role"],x["fit"],x["qualification"],x["note"]])
    lines += [table(["Candidate","Source","Role","Fit","Status","Note"],rows),""]
    lines += [
        "**Locked design decisions:** Centaur is the ranged family; Elephant Warrior is the heavy-weapon fighter; hooded Cheetah is magic/support; Apex Beast is elephant/dragon power tier but species remains open.",
        "**Owned Centaur evidence:** Quadruped Fantasy Creatures already contains the PROTOFACTOR Centaur with 85 animations; the matching Centaur model supports both archery and close combat.",
        "**Magic-animation evidence:** the local Mixamo catalog reports 61 magic/casting candidates; Pro Magic Pack covers one- and two-handed casts, attacks, area attacks, blocks and locomotion.",
        "**Capstone caution:** reusing the same Fantasy Elephant or Dragon already earmarked for Orc/Dark/Dwarf would weaken faction silhouette separation. Prefer a distinct owned apex creature if the library supports one.",
        "",
    ]

    lines += ["## Paragon-specific casting audit",""]
    rows=[]
    for x in PARAGON["candidates"]:
        rows.append([
            x["name"], x["ownership"], x["fab_listing_id"], ", ".join(x["likely_factions"]),
            x["archetype"], x["grounded_fantasy"], x["hide_replace"], x["skeleton_animation"],
            x["redundancy"], x["hero_role"], "yes" if x.get("hero_eligible", True) else "no", x["qualification"],
        ])
    lines += [table(["Paragon","Ownership/local state","Fab listing","Faction fit","Class","Grounded fit","Hide/replace","Skeleton/animation evidence","Redundancy","Best hero role","Hero eligible","Status"],rows),""]
    lines += [
        "### Paragon ownership resolved from screenshots",
        "",
        "The supplied Fab My Library screenshots resolve 21 Paragon search results: 19 named hero-character packs, the Minions troop pack, and the Agora/Monolith environment pack.",
        "Download is not required for casting. Countess is the only payload currently machine-visible locally; every other named hero remains PAYLOAD_PENDING for exact package/skeleton paths.",
        "Minions and Agora/Monolith are owned but excluded from the hero pool because they are not named hero characters.",
        "",
    ]

    lines += ["## Animation/equipment evidence that changes roster feasibility",""]
    lines += [
        "- Canonical local animation repository reports 45 Bow/archery candidates; Pro Longbow Pack includes equip/draw/aim/recoil/walk/block/death/dodge actions.",
        "- Crossbow-specific animation coverage was not found. Dwarf Orme/crossbow remains the cleanest unresolved ranged-slot problem.",
        "- Hivemind Dark Fantasy Weapons is owned and publicly resolves to Fab listing `244607ad-e819-4320-bbc7-687b844b5bed`; the pack is tagged for Bow/Crossbow and includes ranged weapon assets.",
        "- `92 Animations For Warrior` is locally cached and provides a broad Epic-skeleton combat donor for compatible humanoid packs.",
        "",
    ]
    lines += ["## Strongest unresolved roster gaps",""]
    lines += [
        "1. **Dwarf ranged identity:** Orme is owned and named, but the source evidence does not prove a crossbow. No crossbow animation set was found locally. Keep the Crossbow Workshop concept provisional until UE visual qualification or a better owned crossbow character is surfaced.",
        "2. **Exact Witch Adventurer identity:** no exact owned/local product named `Witch Adventurer` was found. `Fantasy Witch` from the owned Fantasy Warriors pack is the strongest evidence-backed substitute and should not be silently renamed into a proven asset.",
        "3. **Viking Shaman identity:** no exact owned Shaman character was verified. A grounded Customized Viking or Primitive elder redress is viable, but remains casting rather than asset proof.",
        "4. **Orc Shaman identity:** 14 Orcs provides a deep shared-skeleton family, but no source metadata labels a shaman. Magur is only a visual-casting hypothesis.",
        "",
    ]
    lines += ["## UE visual qualification queue",""]
    lines += [
        "- Side-by-side Human Knight_02/03/05 variants: ensure militia, man-at-arms and elite guard read as three tiers rather than recolors.",
        "- Verify Griffon at 2-hex and Mountain Dragon/Fantasy Dragon/War Elephant at 3-hex scale; adjust footprint only after battle-camera read.",
        "- Retarget Pro Longbow animations onto Human/Viking/Orc candidate bodies; test shoulder/hand/bow alignment and silhouette.",
        "- Test Dwarf Orme with an owned crossbow/bow solution; if it reads forced, change the ranged family rather than forcing the building fiction.",
        "- Check Shieldmaiden material/scale cohesion beside Bugrimov/Art.Hiraeth Vikings.",
        "- Cast exact Viking and Orc shaman visuals from owned modular pieces; reject anything too neon, sci-fi, anime/JRPG or over-spiked.",
        "- Run Dark pack side-by-side: Dark Knight, Devil, Executioner, Demon, Troll, Befouler, Fantasy Dragon. Suppress permanent emissive/neon styling.",
        "- For Paragons, inspect removable mesh components/skins and record exact skeleton paths as payloads arrive. Countess first because it is already local.",
        "",
    ]
    lines += ["## Evidence files and source notes",""]
    lines += [
        "- `Evidence/soul_roster_asset_inventory_20260920.json` — ownership rows, live Fab listing DB matches, downloaded manifest summaries and exact cached package paths where readable.",
        "- `Evidence/paragon_library_screenshot_inventory_20260920.json` — the 21-product My Library screenshot inventory resolving the previously unknown Paragon identities.",
        "- `Data/soul_faction_roster_candidates_20260920.json` — machine-readable 35-unit roster, alternates and hero board.",
        "- `Data/soul_paragon_casting_candidates_20260920.json` — Paragon ownership/local-state and casting classifications.",
        "- `D:/Animations/_Catalog/CAPABILITY_GAPS.md` + `SEARCH-ANIMATIONS.cmd bow` — local animation capability evidence.",
        "- Fab content confirmation used for pack membership/current listing UUIDs: Quadruped Fantasy Creatures, Fantasy Animal, Fantasy Characters, Fantasy Enemies, Fantasy Warriors, Dwarfs Pack, 14 Orcs Pack, Dark Fantasy Weapons and verified Paragon listings.",
        "",
        "## Non-lock rule",
        "",
        "Nothing marked `UE_VISUAL_CONFIRM` or `PAYLOAD_PENDING` is a final art lock. The roster is the strongest evidence-backed casting board available without consuming the UE lane.",
        "",
    ]
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text("\n".join(lines), encoding="utf-8")
    print("wrote", OUT)

if __name__ == "__main__":
    main()