"""Build a compact compatibility projection of the authoritative Soul start-state set."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT/"Data"/"soul_campaign_start_states_v1_20260922.json"
OUT = ROOT/"Data"/"soul_campaign_start_state_v1_20260922.json"

src = json.loads(SOURCE.read_text(encoding="utf-8"))
sandbox = src["scenarios"]["six_faction_sandbox_candidate"]

factions = {}
for faction,state in sandbox["factions"].items():
    factions[faction] = {
        "seat_region": state["capital_region"],
        "starting_regions": state["starting_possessions"],
        "safe_expansion_targets": state["safe_expansion_targets"],
        "army_spawn_anchors": state["army_spawn_anchors"],
    }

projection = {
    "schema": 1,
    "generated": src["generated"],
    "status": "DERIVED_COMPATIBILITY_PROJECTION",
    "source": SOURCE.name,
    "authority": {
        "authoritative_start_state_set": SOURCE.name,
        "scenario": "six_faction_sandbox_candidate",
        "founder_micro_remains_separate": True,
        "diplomacy": "OUT_OF_SCOPE_UNSET",
    },
    "rules": [
        "This file is derived; do not edit it independently.",
        "Full-world owner/homeland coloring is not campaign-start ownership.",
        "The six-faction sandbox uses one major seat plus one aligned minor settlement per faction.",
        "Human-Orc war is locked only in the founder micro scenario, not in this sandbox projection.",
        "Army composition and garrison numbers remain balance-owned.",
    ],
    "factions": factions,
    "region_control": sandbox["region_control"],
    "neutral_minor_settlements": sandbox["neutral_minor_sites"],
    "counts": {
        "factions": len(factions),
        "starting_possessions": len(sandbox["region_owners"]),
        "neutral_regions": len(sandbox["neutral_regions"]),
        "neutral_minor_settlements": len(sandbox["neutral_minor_sites"]),
    },
}
OUT.write_text(json.dumps(projection,indent=2)+"\n",encoding="utf-8")
print(f"WROTE {OUT}")
print(json.dumps(projection["counts"],indent=2))
