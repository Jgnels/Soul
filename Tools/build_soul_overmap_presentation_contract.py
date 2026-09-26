"""Build the non-UE presentation contract for Soul's campaign overmap."""
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/"Data"/"soul_overmap_presentation_contract_v1_20260922.json"
DOC=ROOT/"Docs"/"WORLD_OVERMAP_PRESENTATION_CONTRACT_20260922.md"

CONTRACT={
 "schema":1,
 "generated":"2026-09-22",
 "presentation_model":"continuous_compressed_3d_world_over_hidden_region_graph",
 "reference_mix":{
   "heroes_3":["clear roads/resources/landmarks","fog/exploration","hero movement choices"],
   "total_war_warhammer":["terrain-constrained fronts","exaggerated readable settlement landmarks","strategic chokepoints"],
   "bannerlord":["continuous physical geography","moving world parties","settlements visually embedded in terrain"],
 },
 "authority":{
   "world_state":"SoulCore FSoulWorldState / FSoulWorldRules",
   "weather":"RB Weather",
   "optimization":"RB Optimization",
   "save":"RB Save",
   "presentation":"Unreal overmap actors/materials only; presentation does not invent campaign outcomes",
 },
 "camera":{
   "default":"elevated three-quarter strategic view",
   "zoom_levels":["world","regional","settlement_approach"],
   "requirements":["terrain readable without labels","settlement silhouette readable at regional zoom",
                   "route/chokepoint readability survives fog/weather"],
 },
 "terrain":{
   "rule":"purpose-built continuous terrain; donor landscapes are source vocabulary, not stitched levels",
   "scale":"compressed strategic geography; absolute kilometres deferred",
   "hard_barriers":["coast","major river unless crossing","mountain belt unless pass"],
   "soft_costs":["forest","rough slope","badlands","snow"],
   "final_region_nodes_visible":False,
   "final_province_borders_visible":False,
 },
 "roads":{
   "presentation":"terrain-following roads/trails with destination highlight only when selected",
   "canonical_semantics":"one adjacent-region action in founder slice; independent logistics movement cost",
   "selected_route_overlay":"temporary readable highlight, never permanent board lines",
 },
 "settlements":{
   "rule":"overmap landmark proxy must resemble the actual visitable settlement donor",
   "states":["intact","damaged","ruined","repairing"],
   "state_cues":["silhouette damage","smoke/fire when appropriate","scaffolding/patching","faction banner"],
   "interaction":"select for details; enter/visit transitions to the physical town presentation",
 },
 "heroes_and_armies":{
   "presentation":"world party actor with readable hero/banner identity; modest scale exaggeration allowed",
   "movement":"animate along validated route after canonical move succeeds",
   "enemy_visibility":"respect Soul exploration/current-visibility state",
   "regiment_detail":"do not render every soldier on the campaign map",
 },
 "fog":{
   "unexplored":"obscure geography and objects",
   "explored_not_visible":"terrain memory remains; current armies/changes are hidden",
   "visible":"show current armies, settlement condition and relevant interactables",
 },
 "territory":{
   "default":"terrain and faction occupation communicate ownership without permanent hard province borders",
   "selection":"subtle temporary ownership wash/outline is allowed on demand",
 },
 "battle_handshake":{
   "inputs":["defender biome","landform","feature","attacker approach","road presence",
             "settlement proximity","weather","time of day","siege flag"],
   "promise":"campaign geography visible to the player must be honored by tactical battlefield selection",
 },
 "performance":{
   "first":["RB Optimization","HLOD/instancing/static representation","bounded foliage density"],
   "settlements":"use simplified overmap proxies; load full visitable town only when entering it",
   "armies":"party-level representation only",
   "micro_props":"do not carry town-showcase clutter onto the overmap",
 },
 "founder_slice_gate":[
   "nine founder regions use current exact IDs and UE blockout positions",
   "three distinct short approaches to Orc Stronghold remain legible",
   "Old Quarry and Forest Edge read as resource opportunities",
   "River Ford and North Pass visibly read as crossing/chokepoint geography",
   "Human Capital and Orc Stronghold are recognizable from their visitable-city silhouettes",
   "fog, AP spending, region selection and AI movement remain driven by existing canonical state",
 ],
}

def main():
    OUT.write_text(json.dumps(CONTRACT,indent=2)+"\n",encoding="utf-8")
    lines=[
      "# Soul Overmap Presentation Contract — 2026-09-22",
      "",
      "## Target",
      "",
      "A continuous compressed 3D campaign world over Soul's deterministic hidden region graph.",
      "The final player should see terrain, roads, rivers, passes, forests, settlements and moving parties — not a province-node editor.",
      "",
      "## Reference synthesis",
      "",
      "- Heroes III: readable roads, resources, landmarks, fog/exploration and hero movement choices.",
      "- Total War: Warhammer: terrain-constrained fronts, readable settlement landmarks and strategic chokepoints.",
      "- Bannerlord: continuous physical geography, moving parties, and settlements embedded in terrain.",
      "",
      "Soul keeps its own deterministic region/state rules underneath that presentation.",
      "",
      "## Locked presentation rules",
      "",
      "- Region nodes and province borders are hidden by default.",
      "- Roads/trails are physical geography; route highlight appears only during selection/planning.",
      "- Settlement proxies resemble their actual visitable scenes and surface canonical damage/repair state.",
      "- Hero/army representation is party-level, not hundreds of campaign-map soldiers.",
      "- Explored terrain memory and current visibility remain distinct.",
      "- RB Weather / RB Optimization / RB Save retain authority for their domains.",
      "",
      "## Founder-slice visual gate",
      "",
    ]
    lines += ["- "+x+"." for x in CONTRACT["founder_slice_gate"]]
    lines += [
      "",
      "## Battle handshake",
      "",
      "The overmap must visibly promise the same biome, landform, feature, approach, road, settlement, weather and siege context used to choose the tactical battlefield.",
      "If the campaign map shows a bridge, pass, river or forest approach, the battle may vary but may not contradict it.",
      "",
      "## Performance",
      "",
      "Use simplified settlement proxies, party-level armies, HLOD/instancing and RB Optimization before bespoke representation work.",
      "The full visitable settlement and its micro-detail are not kept live on the strategic map.",
    ]
    DOC.write_text("\n".join(lines)+"\n",encoding="utf-8")
    print("WROTE",OUT)
    print("WROTE",DOC)

if __name__=="__main__":
    main()
