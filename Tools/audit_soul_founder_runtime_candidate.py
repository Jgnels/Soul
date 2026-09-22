"""Read-only convergence audit: accepted overmap contracts vs dirty founder runtime candidate."""
from __future__ import annotations

import hashlib
import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANDIDATE = Path(r"D:\RefinedBadger\Games\Soul")
DATA = ROOT / "Data"
OUT = ROOT / "Evidence" / "WorldOvermap" / "founder_runtime_candidate_drift.json"
DOC = ROOT / "Docs" / "SOUL_FOUNDER_RUNTIME_CANDIDATE_DRIFT_20260922.md"

STATE_CPP = CANDIDATE / "Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp"
CAMPAIGN_CPP = CANDIDATE / "Source/Soul/Private/SoulFounderPlaytestCampaignActor.cpp"
BATTLE_CPP = CANDIDATE / "Source/Soul/Private/SoulFounderPlaytestBattleActor.cpp"

world = json.loads((DATA / "soul_world_overmap_v1_20260922.json").read_text(encoding="utf-8"))
starts = json.loads((DATA / "soul_campaign_start_states_v1_20260922.json").read_text(encoding="utf-8"))
handoffs = json.loads((DATA / "soul_overmap_battle_handoff_v1_20260922.json").read_text(encoding="utf-8"))
scenario = starts["scenarios"]["founder_human_orc_micro"]
founder_ids = set(scenario["region_ids"])
accepted_nodes = {n["id"]: n for n in world["nodes"] if n["id"] in founder_ids}
accepted_edges = {}
for edge in world["edges"]:
    if edge["a"] in founder_ids and edge["b"] in founder_ids:
        accepted_edges[frozenset((edge["a"], edge["b"]))] = edge

for path in (STATE_CPP, CAMPAIGN_CPP, BATTLE_CPP):
    if not path.exists():
        raise SystemExit(f"candidate file missing: {path}")

state_text = STATE_CPP.read_text(encoding="utf-8")
campaign_text = CAMPAIGN_CPP.read_text(encoding="utf-8")
battle_text = BATTLE_CPP.read_text(encoding="utf-8")

def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

def git_status(path: Path) -> str:
    rel = path.relative_to(CANDIDATE).as_posix()
    proc = subprocess.run(
        ["git", "-C", str(CANDIDATE), "status", "--short", "--", rel],
        capture_output=True, text=True, encoding="utf-8", errors="replace",
    )
    return proc.stdout.strip()

region_pattern = re.compile(
    r'World\.Regions\.Add\(TEXT\("([^"]+)"\),\s*'
    r'MakeRegion\(TEXT\("([^"]+)"\),\s*'
    r'(TEXT\("([^"]+)"\)|NAME_None),\s*'
    r'TEXT\("([^"]+)"\),\s*TEXT\("([^"]+)"\),\s*TEXT\("([^"]+)"\)'
)
candidate_regions = {}
for match in region_pattern.finditer(state_text):
    key, inner_id, owner_expr, owner, biome, landform, feature = match.groups()
    candidate_regions[key] = {
        "id": inner_id,
        "owner": owner or None,
        "biome": biome,
        "landform": landform,
        "feature": feature,
    }

link_pattern = re.compile(
    r'Link\(World,\s*TEXT\("([^"]+)"\),\s*TEXT\("([^"]+)"\)(?:,\s*(true|false))?\);'
)
candidate_edges = {}
for a, b, road in link_pattern.findall(state_text):
    candidate_edges[frozenset((a, b))] = {
        "a": a, "b": b, "road": road == "true",
    }

metadata_drift = []
for rid in sorted(founder_ids):
    accepted = accepted_nodes[rid]
    candidate = candidate_regions.get(rid)
    if not candidate:
        metadata_drift.append({"region_id": rid, "kind": "missing_candidate_region"})
        continue
    for field in ("owner", "biome", "landform", "feature"):
        expected = accepted["owner"] if field == "owner" else accepted[field]
        actual = candidate[field]
        if actual != expected:
            metadata_drift.append({
                "region_id": rid,
                "field": field,
                "accepted": expected,
                "candidate": actual,
            })

edge_drift = []
for pair, accepted in accepted_edges.items():
    candidate = candidate_edges.get(pair)
    if not candidate:
        edge_drift.append({"pair": sorted(pair), "kind": "missing_candidate_edge"})
        continue
    if bool(candidate["road"]) != bool(accepted["road"]):
        edge_drift.append({
            "pair": sorted(pair),
            "field": "road",
            "accepted": bool(accepted["road"]),
            "candidate": bool(candidate["road"]),
        })
for pair in candidate_edges:
    if pair not in accepted_edges:
        edge_drift.append({"pair": sorted(pair), "kind": "extra_candidate_edge"})

checks = [
    {
        "id": "candidate_is_uncommitted",
        "severity": "boundary",
        "pass": all(git_status(p).startswith("??") for p in (STATE_CPP, CAMPAIGN_CPP, BATTLE_CPP)),
        "evidence": {p.name: git_status(p) for p in (STATE_CPP, CAMPAIGN_CPP, BATTLE_CPP)},
        "finding": "Founder playtest runtime is untracked in the dirty primary and has no committed Git authority.",
    },
    {
        "id": "topology_matches_current_founder_ids",
        "severity": "pass" if set(candidate_regions) == founder_ids else "high",
        "pass": set(candidate_regions) == founder_ids,
        "evidence": {
            "accepted_regions": sorted(founder_ids),
            "candidate_regions": sorted(candidate_regions),
        },
        "finding": "The candidate currently uses the same nine region IDs.",
    },
    {
        "id": "edge_graph_matches",
        "severity": "pass" if not edge_drift else "high",
        "pass": not edge_drift,
        "evidence": edge_drift,
        "finding": "Adjacency/road flags should converge on the accepted graph rather than remain separately hardcoded.",
    },
    {
        "id": "region_metadata_matches",
        "severity": "medium" if metadata_drift else "pass",
        "pass": not metadata_drift,
        "evidence": metadata_drift,
        "finding": "The candidate has region biome/landform/feature/owner drift from accepted overmap data.",
    },
    {
        "id": "hostile_nonsettlement_entry_gated_before_occupation",
        "severity": "critical",
        "pass": False,
        "evidence": {
            "move_sets_player_region_before_owner_gate": "PlayerRegion = TargetRegion;" in state_text,
            "orc_owner_excluded_from_capture_only": 'Region->OwnerFactionId != TEXT("orcs")' in state_text,
            "pre_move_battle_prompt_only_for_hostile_settlement": (
                'Target->OwnerFactionId == TEXT("orcs") && Target->bSettlement' in campaign_text
            ),
        },
        "finding": (
            "Current candidate can move the player into an Orc-owned non-settlement such as Orc Watch before battle. "
            "Accepted start-state policy requires hostile entry to resolve encounter/battle before occupation or traversal."
        ),
    },
    {
        "id": "battle_recipe_uses_directed_handoff",
        "severity": "high",
        "pass": False,
        "evidence": {
            "hardcoded_level": "/Game/Soul/Maps/Battlefields/Overlays/BF_Orc_Badlands_Overlay",
            "hardcoded_present": "/Game/Soul/Maps/Battlefields/Overlays/BF_Orc_Badlands_Overlay" in campaign_text,
            "accepted_stronghold_recipes": sorted({
                h["battlefield_selection"]["recipe_id"]
                for h in handoffs["handoffs"] if h["destination_region"] == "orc_camp"
            }),
        },
        "finding": (
            "StartBattle opens one hardcoded Orc Badlands overlay instead of consuming the directed battle handoff. "
            "The accepted Stronghold destination currently resolves to orc.war_camp."
        ),
    },
    {
        "id": "battle_victory_returns_to_actual_region",
        "severity": "critical",
        "pass": False,
        "evidence": {
            "hardcoded_award": 'AwardBattleVictory(TEXT("orc_camp"), 360)' in battle_text,
        },
        "finding": (
            "Every candidate battle victory currently awards/captures orc_camp. "
            "A battle launched from Orc Watch can therefore resolve the wrong strategic region."
        ),
    },
    {
        "id": "neutral_rewards_are_data_driven",
        "severity": "decision_required",
        "pass": False,
        "evidence": {
            "generic_gold_reward_250": '+= 250;' in state_text,
            "generic_hero_xp_140": "AddExperience(Hero, 140)" in state_text,
        },
        "finding": (
            "The candidate grants the same first-capture gold/XP reward to generic neutral regions. "
            "Accepted overmap data distinguishes resource, landmark, crossing and pass roles and does not authorize a universal reward."
        ),
    },
    {
        "id": "enemy_force_model_matches_reinforcement_analysis",
        "severity": "model_gap",
        "pass": False,
        "evidence": {
            "single_enemy_region_starts_at_camp": 'EnemyRegion = TEXT("orc_camp")' in state_text,
            "separate_orc_watch_reserve_defined": False,
        },
        "finding": (
            "The current candidate models one movable Orc field army, not an Orc Watch reserve plus Stronghold force. "
            "Reserve strength in the balance lab remains synthetic sensitivity glue and must not be mistaken for runtime state."
        ),
    },
]

payload = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "READ_ONLY_RUNTIME_CANDIDATE_DRIFT_AUDIT",
    "authority": {
        "accepted_geography": "Data/soul_world_overmap_v1_20260922.json",
        "accepted_start_state": "Data/soul_campaign_start_states_v1_20260922.json",
        "accepted_battle_handoff": "Data/soul_overmap_battle_handoff_v1_20260922.json",
        "candidate_root": str(CANDIDATE),
        "candidate_authority": "UNTRACKED_DIRTY_PRIMARY_PROTOTYPE_NOT_CANONICAL",
    },
    "candidate_hashes": {
        str(p.relative_to(CANDIDATE)).replace("\\", "/"): sha(p)
        for p in (STATE_CPP, CAMPAIGN_CPP, BATTLE_CPP)
    },
    "counts": {
        "accepted_regions": len(founder_ids),
        "candidate_regions": len(candidate_regions),
        "accepted_edges": len(accepted_edges),
        "candidate_edges": len(candidate_edges),
        "metadata_drift_items": len(metadata_drift),
        "edge_drift_items": len(edge_drift),
    },
    "checks": checks,
    "convergence_order": [
        "Preserve the candidate files; do not reset or overwrite the dirty primary.",
        "Use accepted overmap/import data for region IDs, positions, adjacency, metadata and directed battle context.",
        "Gate hostile non-settlement entry before changing PlayerRegion or ownership.",
        "Carry actual source/destination/approach/handoff into battle launch and return.",
        "Remove hardcoded orc_camp victory attribution; persist the battle's actual strategic destination.",
        "Keep reserve/garrison numbers outside runtime until a founder-approved force/reaction rule exists.",
        "Only then decide explicit rewards for resource sites, landmarks and neutral captures.",
    ],
}
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

lines = [
    "# Soul Founder Runtime Candidate Drift Audit — 2026-09-22", "",
    "Status: **read-only audit of an untracked dirty-primary prototype; no candidate files modified.**", "",
    f"- Candidate regions: {len(candidate_regions)} / accepted {len(founder_ids)}.",
    f"- Candidate edges: {len(candidate_edges)} / accepted {len(accepted_edges)}.",
    f"- Region metadata drift items: {len(metadata_drift)}.",
    f"- Edge drift items: {len(edge_drift)}.", "",
    "## Findings", "",
]
for check in checks:
    lines.append(f"- **{check['id']} ({check['severity']})** — {check['finding']}")
lines += ["", "## Safe convergence order", ""]
lines += [f"{i}. {item}" for i, item in enumerate(payload["convergence_order"], 1)]
DOC.write_text("\n".join(lines) + "\n", encoding="utf-8")
print(json.dumps({
    "status": payload["status"],
    "metadata_drift_items": len(metadata_drift),
    "edge_drift_items": len(edge_drift),
    "critical_failures": [x["id"] for x in checks if x["severity"] == "critical" and not x["pass"]],
}, indent=2))
