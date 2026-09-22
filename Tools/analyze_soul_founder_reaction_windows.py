"""Analyze exact Orc Watch -> Orc Stronghold reaction windows on the founder graph.

This is policy-neutral evidence. It does not choose the canonical reinforcement rule.
"""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SEED = ROOT / "Data" / "soul_founder_slice_runtime_seed_20260922.json"
ASSAULT = ROOT / "Evidence" / "BalanceLab" / "founder_assault_balance.json"
OUT = ROOT / "Evidence" / "WorldOvermap" / "founder_reaction_window_analysis.json"
DOC = ROOT / "Docs" / "SOUL_FOUNDER_REACTION_WINDOWS_20260922.md"

seed = json.loads(SEED.read_text(encoding="utf-8"))
assault = json.loads(ASSAULT.read_text(encoding="utf-8"))
routes = seed["routes"]

edge_by_pair = {}
for route in routes.values():
    edge_by_pair[frozenset((route["a"], route["b"]))] = route

north_path = ["human_capital", "crossroads", "forest_edge", "north_pass", "orc_camp"]
reserve_pair = frozenset(("orc_watch", "orc_camp"))
reserve_edge = edge_by_pair[reserve_pair]
if reserve_edge["action_cost"] != 1:
    raise RuntimeError("Orc Watch -> Orc Stronghold reaction analysis assumes current one-action adjacent road")

def action_slot(cumulative_action: int, ap_per_day: int) -> tuple[int, int]:
    if cumulative_action < 1:
        raise ValueError("actions are 1-based")
    day = (cumulative_action - 1) // ap_per_day + 1
    slot = (cumulative_action - 1) % ap_per_day + 1
    return day, slot

# Current hostile-entry semantics resolve battle on the movement action into the hostile region.
# North Pass becomes the visible adjacent threat after the third movement action.
threat_reveal_action = 3
stronghold_entry_action = 4

rows = []
for ap in (2, 3, 4):
    threat_day, threat_slot = action_slot(threat_reveal_action, ap)
    battle_day, battle_slot = action_slot(stronghold_entry_action, ap)
    day_boundary = battle_day > threat_day
    same_day_gap = battle_day == threat_day and battle_slot > threat_slot

    outcomes = {
        "no_relocation": {
            "eligible": False,
            "certainty": "no",
            "reason": "Reserve is not allowed to leave Orc Watch.",
        },
        "overnight_physical_relocation": {
            "eligible": day_boundary,
            "certainty": "yes" if day_boundary else "no",
            "reason": (
                "At least one campaign-day boundary exists after North Pass reveals the threat."
                if day_boundary else
                "Stronghold contact occurs without a campaign-day boundary after North Pass reveals the threat."
            ),
        },
        "same_day_interleaved_reaction": {
            "eligible": day_boundary or same_day_gap,
            "certainty": "yes" if day_boundary else ("order_dependent" if same_day_gap else "no"),
            "reason": (
                "Overnight window exists."
                if day_boundary else
                "Only a same-day action slot exists; outcome depends on faction/action ordering or an explicit reaction phase."
            ),
        },
        "automatic_adjacent_support": {
            "eligible": True,
            "certainty": "yes",
            "reason": "An intact adjacent reserve is admitted at battle commit without prior strategic relocation.",
        },
    }
    rows.append({
        "ap_per_day": ap,
        "threat_reveal": {
            "region": "north_pass",
            "cumulative_action": threat_reveal_action,
            "day": threat_day,
            "action_slot": threat_slot,
        },
        "stronghold_contact": {
            "region": "orc_camp",
            "cumulative_action": stronghold_entry_action,
            "day": battle_day,
            "action_slot": battle_slot,
        },
        "reaction_gap": {
            "campaign_day_boundary": day_boundary,
            "same_day_later_action_slot": same_day_gap,
            "reserve_move_actions": reserve_edge["action_cost"],
            "reserve_logistics_cost": reserve_edge["logistics_movement_cost"],
            "reserve_road": reserve_edge["road"],
        },
        "policy_outcomes": outcomes,
    })

coarse = {
    s["ap_per_day"]: s
    for s in assault["summaries"]
    if s["profile"] == "watch_reserve_500" and s["corridor"] == "north_pass"
}
coarse_comparison = []
for row in rows:
    ap = row["ap_per_day"]
    previous = coarse[ap]["reserve_arrival_rate"]
    overnight = 1.0 if row["policy_outcomes"]["overnight_physical_relocation"]["eligible"] else 0.0
    coarse_comparison.append({
        "ap_per_day": ap,
        "previous_coarse_reserve_arrival_rate": previous,
        "overnight_physical_relocation_eligibility": overnight,
        "coarse_overstates_overnight_reaction": previous > overnight,
    })

findings = [
    {
        "id": "ap2_coarse_overstatement",
        "severity": "correction",
        "finding": (
            "At 2 AP/day the attacker reveals North Pass on day 2 action 1 and reaches the Stronghold on "
            "day 2 action 2. The prior day>1 shortcut therefore overstates an overnight reinforcement window: "
            "there is no day boundary between threat reveal and contact."
        ),
    },
    {
        "id": "ap3_true_overnight_window",
        "severity": "info",
        "finding": (
            "At 3 AP/day the attacker reaches North Pass on day 1 action 3 and the Stronghold on day 2 action 1. "
            "This is the only tested AP value with an unambiguous overnight physical-reaction window."
        ),
    },
    {
        "id": "ap4_same_day_rush",
        "severity": "info",
        "finding": (
            "At 4 AP/day North Pass is reached on day 1 action 3 and the Stronghold on day 1 action 4. "
            "An Orc Watch reinforcement requires an explicit same-day reaction/interleaving rule or automatic adjacent support."
        ),
    },
    {
        "id": "physical_relocation_opportunity_cost",
        "severity": "design_test",
        "finding": (
            "If Orc Watch reinforcement is modeled as physical strategic relocation, the reserve cannot simultaneously remain "
            "at the outpost. That makes bypassing the Watch a meaningful trade: the defender may reinforce the Stronghold by "
            "vacating the forward position instead of duplicating strength."
        ),
    },
]

payload = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "POLICY_NEUTRAL_REACTION_EVIDENCE",
    "authority": {
        "topology_and_action_cost": "Data/soul_founder_slice_runtime_seed_20260922.json",
        "previous_sensitivity": "Evidence/BalanceLab/founder_assault_balance.json",
        "canonical_reinforcement_policy": "UNDECIDED_REQUIRES_FOUNDER_APPROVAL",
    },
    "assumptions": [
        "Movement into a hostile region resolves contact/battle on that movement action, matching the current founder stress model.",
        "North Pass is the adjacent threat-reveal point for the bypass corridor.",
        "Orc Watch -> Orc Stronghold is one strategic movement action on the current road edge.",
        "No policy below is canonical; the matrix exists to expose consequences before a product decision.",
    ],
    "north_pass_path": north_path,
    "reserve_route": {
        "source": "orc_watch",
        "destination": "orc_camp",
        "action_cost": reserve_edge["action_cost"],
        "logistics_movement_cost": reserve_edge["logistics_movement_cost"],
        "road": reserve_edge["road"],
    },
    "reaction_windows": rows,
    "previous_coarse_comparison": coarse_comparison,
    "findings": findings,
    "candidate_for_next_prototype": {
        "status": "PROPOSAL_ONLY_REQUIRES_FOUNDER_DECISION",
        "rule_shape": (
            "Physical relocation over the real Orc Watch -> Stronghold edge; no reserve duplication. "
            "Overnight reaction is unambiguous; same-day reaction requires an explicit reaction/order rule."
        ),
        "why_test_first": (
            "It preserves graph causality and opportunity cost without granting teleport support, while making the "
            "remaining ambiguity small and directly testable in the campaign turn model."
        ),
    },
}
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

lines = [
    "# Soul Founder Reaction Windows — 2026-09-22", "",
    "Status: **policy-neutral evidence; no canonical reinforcement rule changed.**", "",
    "The earlier founder assault sensitivity deliberately used a coarse day>1 reserve shortcut. "
    "This analysis resolves the actual action slots on the current graph.", "",
    "| AP/day | North Pass revealed | Stronghold contact | Overnight window | Same-day reaction |",
    "|---:|---|---|---|---|",
]
for row in rows:
    t = row["threat_reveal"]
    b = row["stronghold_contact"]
    overnight = "yes" if row["reaction_gap"]["campaign_day_boundary"] else "no"
    same = row["policy_outcomes"]["same_day_interleaved_reaction"]["certainty"]
    lines.append(
        f"| {row['ap_per_day']} | day {t['day']} / action {t['action_slot']} | "
        f"day {b['day']} / action {b['action_slot']} | {overnight} | {same} |"
    )
lines += ["", "## Findings", ""]
for item in findings:
    lines.append(f"- **{item['id']} ({item['severity']})** — {item['finding']}")
lines += [
    "", "## Candidate for the next prototype — not a product decision", "",
    payload["candidate_for_next_prototype"]["rule_shape"],
    "",
    payload["candidate_for_next_prototype"]["why_test_first"],
]
DOC.write_text("\n".join(lines) + "\n", encoding="utf-8")
print(json.dumps({
    "status": payload["status"],
    "reaction_windows": len(rows),
    "coarse_overstatement_ap": [
        x["ap_per_day"] for x in coarse_comparison if x["coarse_overstates_overnight_reaction"]
    ],
}, indent=2))
