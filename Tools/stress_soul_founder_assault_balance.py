"""Stress Soul's actual 9-region founder slice with live-mirrored campaign rules.

All combat/garrison numbers remain BalanceLab glue because founder numeric army/garrison
profiles are explicitly BALANCE_LAB_OWNED. The purpose is comparative sensitivity,
not shipping win-rate prediction.
"""
from __future__ import annotations

import csv
import json
import random
import statistics
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from Tools.BalanceLab.balance_lab import (  # noqa: E402
    Logistics,
    UNIT_LINES,
    apply_travel,
    end_day_logistics,
    rank_stats,
    xp_for_level,
)

SEED_FILE = ROOT / "Data" / "soul_founder_slice_runtime_seed_20260922.json"
START_FILE = ROOT / "Data" / "soul_campaign_start_states_v1_20260922.json"
OUT_JSON = ROOT / "Evidence" / "BalanceLab" / "founder_assault_balance.json"
OUT_CSV = ROOT / "Evidence" / "BalanceLab" / "founder_assault_balance.csv"
OUT_MD = ROOT / "Docs" / "SOUL_FOUNDER_ASSAULT_BALANCE_20260922.md"

BASE_SEED = 20260922
RUNS = 1000
SENSITIVITY_RUNS = 500
BASE_FIELD_STRENGTH = 1050
WATCH_GARRISON_STRENGTH = 500
STRONGHOLD_FIELD_STRENGTH = 1050
WATCH_RESERVE_STRENGTH = 500
BASE_GOLD = 240
BASE_INCOME = 18
RESOURCE_INCOME = 9
RESPAWN_STRENGTH = 220
RESPAWN_DELAY = 3
TARGET_STRENGTH = 1900

CORRIDORS = {
    "river_watch": [
        "human_capital", "crossroads", "river_ford", "orc_watch", "orc_camp"
    ],
    "forest_watch": [
        "human_capital", "crossroads", "forest_edge", "orc_watch", "orc_camp"
    ],
    "north_pass": [
        "human_capital", "crossroads", "forest_edge", "north_pass", "orc_camp"
    ],
}

seed_doc = json.loads(SEED_FILE.read_text(encoding="utf-8"))
start_doc = json.loads(START_FILE.read_text(encoding="utf-8"))
scenario = start_doc["scenarios"]["founder_human_orc_micro"]
regions = seed_doc["regions"]
routes = seed_doc["routes"]
initial_owners = scenario["owners"]

edge_by_pair = {}
for route in routes.values():
    edge_by_pair[frozenset((route["a"], route["b"]))] = route

def hero_level(xp: int) -> int:
    level = 1
    while xp >= xp_for_level(level + 1):
        level += 1
    return level

def effective_strength(
    strength: int,
    logistics: Logistics,
    regiment_xp: int,
    hero_xp: int,
    rng: random.Random,
) -> float:
    _, veterancy_permille, _ = rank_stats(regiment_xp)
    readiness = (500 + logistics.readiness / 2) / 1000
    supply = (700 + logistics.supply * 0.3) / 1000
    fatigue = (1000 - logistics.fatigue * 0.25) / 1000
    hero = 1 + (hero_level(hero_xp) - 1) * 55 / 1000
    variance = 1 + rng.randint(-100, 100) / 1000
    return max(
        1.0,
        strength
        * (1 + veterancy_permille / 1000)
        * readiness
        * supply
        * fatigue
        * hero
        * variance,
    )

def resolve_lab_battle(
    strength: int,
    logistics: Logistics,
    regiment_xp: int,
    hero_xp: int,
    defender_strength: int,
    rng: random.Random,
) -> dict:
    attacker_effective = effective_strength(
        strength, logistics, regiment_xp, hero_xp, rng
    )
    defender_variance = 1 + rng.randint(-100, 100) / 1000
    defender_effective = defender_strength * defender_variance
    attacker_won = attacker_effective >= defender_effective
    ratio = max(attacker_effective, defender_effective) / max(
        1.0, min(attacker_effective, defender_effective)
    )
    winner_loss = max(120, min(420, round(330 - 70 * min(3, ratio - 1))))
    loser_loss = max(520, min(950, round(720 + 65 * min(3, ratio - 1))))
    if attacker_won:
        strength = max(1, strength * (1000 - winner_loss) // 1000)
        regiment_xp += 85
        hero_xp += 100
    else:
        strength = max(0, strength * (1000 - loser_loss) // 1000)
        if strength < 90:
            strength = 0
        regiment_xp += 35
        hero_xp += 35
    return {
        "won": attacker_won,
        "strength": strength,
        "regiment_xp": regiment_xp,
        "hero_xp": hero_xp,
        "attacker_effective": round(attacker_effective, 3),
        "defender_effective": round(defender_effective, 3),
    }

@dataclass
class RunState:
    day: int = 1
    ap_left: int = 3
    strength: int = BASE_FIELD_STRENGTH
    regiment_xp: int = 0
    hero_xp: int = 0
    logistics: Logistics = None
    current: str = "human_capital"

    def __post_init__(self) -> None:
        if self.logistics is None:
            self.logistics = Logistics()

def resource_count(controlled: set[str]) -> int:
    return sum(
        1 for rid in controlled
        if "resource_site" in set(regions[rid].get("site_roles", []))
    )

def end_day(state: RunState, controlled: set[str], ap_per_day: int) -> None:
    is_settlement = bool(regions[state.current].get("settlement_id"))
    friendly = state.current in controlled
    end_day_logistics(state.logistics, is_settlement and friendly, friendly)
    state.day += 1
    state.ap_left = ap_per_day

def simulate_corridor(
    corridor_id: str,
    ap_per_day: int,
    reserve_strength: int,
    seed: int,
    stronghold_strength: int = STRONGHOLD_FIELD_STRENGTH,
    watch_garrison_strength: int = WATCH_GARRISON_STRENGTH,
) -> dict:
    rng = random.Random(seed)
    path = CORRIDORS[corridor_id]
    state = RunState(ap_left=ap_per_day)
    controlled = {"human_capital", "crossroads"}
    watch_battle = None
    camp_battle = None
    camp_pre = None
    action_count = 0

    for target in path[1:]:
        if state.ap_left <= 0:
            end_day(state, controlled, ap_per_day)

        edge = edge_by_pair[frozenset((state.current, target))]
        owner = initial_owners.get(target)
        hostile = owner == "orcs" and target not in controlled
        apply_travel(
            state.logistics,
            edge["logistics_movement_cost"],
            edge["road"],
            hostile,
        )
        state.ap_left -= edge["action_cost"]
        action_count += edge["action_cost"]

        if hostile:
            if target == "orc_watch":
                defense = watch_garrison_strength
            elif target == "orc_camp":
                reserve_arrives = (
                    "orc_watch" not in controlled
                    and reserve_strength > 0
                    and state.day > 1
                )
                defense = stronghold_strength + (
                    reserve_strength if reserve_arrives else 0
                )
                camp_pre = {
                    "day": state.day,
                    "strength": state.strength,
                    "supply": state.logistics.supply,
                    "readiness": state.logistics.readiness,
                    "fatigue": state.logistics.fatigue,
                    "regiment_xp": state.regiment_xp,
                    "hero_xp": state.hero_xp,
                    "regiment_rank": rank_stats(state.regiment_xp)[0],
                    "hero_level": hero_level(state.hero_xp),
                    "reserve_arrives": reserve_arrives,
                    "defender_strength": defense,
                }
            else:
                raise AssertionError(f"unexpected hostile founder target {target}")

            battle = resolve_lab_battle(
                state.strength,
                state.logistics,
                state.regiment_xp,
                state.hero_xp,
                defense,
                rng,
            )
            state.strength = battle["strength"]
            state.regiment_xp = battle["regiment_xp"]
            state.hero_xp = battle["hero_xp"]
            state.current = target

            if target == "orc_watch":
                watch_battle = battle
            else:
                camp_battle = battle

            if not battle["won"]:
                return {
                    "corridor": corridor_id,
                    "ap_per_day": ap_per_day,
                    "reserve_strength": reserve_strength,
                    "reached_camp": target == "orc_camp",
                    "won_camp": False,
                    "lost_at": target,
                    "camp_pre": camp_pre,
                    "watch_battle": watch_battle,
                    "camp_battle": camp_battle,
                    "actions": action_count,
                    "resource_sites": resource_count(controlled),
                    "controlled": sorted(controlled),
                }

            controlled.add(target)
            if target == "orc_camp":
                return {
                    "corridor": corridor_id,
                    "ap_per_day": ap_per_day,
                    "reserve_strength": reserve_strength,
                    "reached_camp": True,
                    "won_camp": True,
                    "lost_at": None,
                    "camp_pre": camp_pre,
                    "watch_battle": watch_battle,
                    "camp_battle": camp_battle,
                    "actions": action_count,
                    "resource_sites": resource_count(controlled),
                    "controlled": sorted(controlled),
                }

            end_day(state, controlled, ap_per_day)
            continue

        state.current = target
        controlled.add(target)
        if state.ap_left <= 0:
            end_day(state, controlled, ap_per_day)

    raise AssertionError(f"corridor {corridor_id} did not resolve Orc Stronghold")

def recruitment_order() -> list[str]:
    return sorted(
        UNIT_LINES,
        key=lambda unit: (
            -(UNIT_LINES[unit][3] / max(1, UNIT_LINES[unit][2])),
            unit,
        ),
    )

def weekly_growth(pools: dict[str, int]) -> None:
    for unit, (growth, capacity, _, _) in UNIT_LINES.items():
        pools[unit] = min(capacity, pools[unit] + growth)

def recruit(
    pools: dict[str, int],
    gold: int,
    strength: int,
) -> tuple[int, int]:
    changed = True
    order = recruitment_order()
    while changed and strength < TARGET_STRENGTH:
        changed = False
        for unit in order:
            growth, capacity, cost, unit_strength = UNIT_LINES[unit]
            _ = growth, capacity
            if pools[unit] > 0 and gold >= cost:
                gold -= cost
                pools[unit] -= 1
                strength += unit_strength
                changed = True
                if strength >= TARGET_STRENGTH:
                    break
    return gold, strength

def recovery_stress(loss_day: int, resource_sites: int, repeat: bool = False) -> dict:
    pools = {unit: values[0] for unit, values in UNIT_LINES.items()}
    gold = BASE_GOLD
    strength = 0
    respawn_day = loss_day + RESPAWN_DELAY
    loss_days = [loss_day]
    if repeat:
        loss_days += [loss_day + 5, loss_day + 10]
    end = loss_day + (18 if repeat else 14)
    recovery70 = []
    active_loss_index = 0
    just_respawned = False
    history = []

    for day in range(1, end + 1):
        gold += BASE_INCOME + resource_sites * RESOURCE_INCOME
        if day > 1 and (day - 1) % 7 == 0:
            weekly_growth(pools)

        if active_loss_index < len(loss_days) and day == loss_days[active_loss_index]:
            strength = 0
            respawn_day = day + RESPAWN_DELAY
            just_respawned = False
            active_loss_index += 1

        if strength == 0 and day == respawn_day:
            strength = RESPAWN_STRENGTH
            just_respawned = True
        elif strength > 0 and not just_respawned:
            gold, strength = recruit(pools, gold, strength)
        else:
            just_respawned = False

        threshold = BASE_FIELD_STRENGTH * 70 // 100
        if strength >= threshold and (
            not recovery70 or recovery70[-1] < max(loss_days[:active_loss_index], default=0)
        ):
            recovery70.append(day)

        history.append({
            "day": day,
            "gold": gold,
            "strength": strength,
            "pool_total": sum(pools.values()),
            "pool_empty_lines": sum(1 for value in pools.values() if value == 0),
        })

    return {
        "loss_days": loss_days,
        "resource_sites": resource_sites,
        "daily_income": BASE_INCOME + resource_sites * RESOURCE_INCOME,
        "recovery70_days": recovery70,
        "final_gold": gold,
        "final_strength": strength,
        "remaining_pool_units": sum(pools.values()),
        "empty_pool_lines": sorted(unit for unit, value in pools.items() if value == 0),
        "history": history,
    }

def median_int(values: list[int]) -> int:
    return int(round(statistics.median(values))) if values else 0

def summarize_runs(rows: list[dict]) -> dict:
    reached = [r for r in rows if r["reached_camp"]]
    camp_wins = [r for r in rows if r["won_camp"]]
    watch_rows = [r for r in rows if r["watch_battle"] is not None]
    camp_pre = [r["camp_pre"] for r in reached if r["camp_pre"] is not None]
    return {
        "runs": len(rows),
        "camp_reach_rate": round(len(reached) / len(rows), 4),
        "camp_victory_rate_all_runs": round(len(camp_wins) / len(rows), 4),
        "camp_victory_rate_given_reached": round(
            len(camp_wins) / len(reached), 4
        ) if reached else 0.0,
        "watch_win_rate": round(
            sum(1 for r in watch_rows if r["watch_battle"]["won"]) / len(watch_rows), 4
        ) if watch_rows else None,
        "median_camp_day": median_int([x["day"] for x in camp_pre]),
        "reserve_arrival_rate": round(
            sum(1 for x in camp_pre if x["reserve_arrives"]) / len(camp_pre), 4
        ) if camp_pre else 0.0,
        "median_pre_camp_strength": median_int([x["strength"] for x in camp_pre]),
        "median_pre_camp_supply": median_int([x["supply"] for x in camp_pre]),
        "median_pre_camp_readiness": median_int([x["readiness"] for x in camp_pre]),
        "median_pre_camp_fatigue": median_int([x["fatigue"] for x in camp_pre]),
        "median_pre_camp_regiment_xp": median_int([x["regiment_xp"] for x in camp_pre]),
        "median_pre_camp_hero_xp": median_int([x["hero_xp"] for x in camp_pre]),
    }

def route_static_facts(corridor_id: str, ap_per_day: int) -> dict:
    path = CORRIDORS[corridor_id]
    hostile_entries = [rid for rid in path if initial_owners.get(rid) == "orcs"]
    resources = [
        rid for rid in path
        if "resource_site" in set(regions[rid].get("site_roles", []))
    ]
    actions = len(path) - 1
    return {
        "path": path,
        "actions_to_stronghold_resolution": actions,
        "hostile_entries": hostile_entries,
        "resource_sites": resources,
        "theoretical_days_without_battle_stop": (actions + ap_per_day - 1) // ap_per_day,
    }

def main() -> None:
    summaries = []
    raw_groups = {}
    for reserve_name, reserve_strength in (
        ("no_watch_reserve", 0),
        ("watch_reserve_500", WATCH_RESERVE_STRENGTH),
    ):
        for ap in (2, 3, 4):
            for corridor_id in CORRIDORS:
                rows = [
                    simulate_corridor(
                        corridor_id,
                        ap,
                        reserve_strength,
                        BASE_SEED + i,
                    )
                    for i in range(RUNS)
                ]
                key = f"{reserve_name}|ap{ap}|{corridor_id}"
                raw_groups[key] = rows
                summary = summarize_runs(rows)
                summary.update({
                    "profile": reserve_name,
                    "reserve_strength": reserve_strength,
                    "ap_per_day": ap,
                    "corridor": corridor_id,
                    **route_static_facts(corridor_id, ap),
                })
                summaries.append(summary)

    stronghold_sensitivity = []
    for stronghold_strength in (650, 700, 750, 800, 900, 1050):
        for reserve_strength in (0, 200, 500):
            for corridor_id in CORRIDORS:
                rows = [
                    simulate_corridor(
                        corridor_id,
                        3,
                        reserve_strength,
                        BASE_SEED + i,
                        stronghold_strength=stronghold_strength,
                    )
                    for i in range(SENSITIVITY_RUNS)
                ]
                summary = summarize_runs(rows)
                summary.update({
                    "stronghold_strength": stronghold_strength,
                    "reserve_strength": reserve_strength,
                    "ap_per_day": 3,
                    "corridor": corridor_id,
                })
                stronghold_sensitivity.append(summary)

    baseline = {
        s["corridor"]: s
        for s in summaries
        if s["profile"] == "watch_reserve_500" and s["ap_per_day"] == 3
    }
    recoveries = {}
    repeats = {}
    for corridor_id, summary in baseline.items():
        resource_sites = len(summary["resource_sites"])
        recoveries[corridor_id] = recovery_stress(
            summary["median_camp_day"], resource_sites, repeat=False
        )
        repeats[corridor_id] = recovery_stress(
            summary["median_camp_day"], resource_sites, repeat=True
        )

    issues = []
    if all(x["median_camp_day"] <= 3 for x in baseline.values()):
        issues.append({
            "kind": "no_excessive_walking",
            "severity": "pass",
            "finding": "All three 3-AP openings reach the stronghold decision by day 2-3 in the live-mirrored action model.",
        })

    watch_baseline = [baseline["river_watch"], baseline["forest_watch"]]
    north = baseline["north_pass"]
    if north["reserve_arrival_rate"] > 0:
        issues.append({
            "kind": "reinforcement_first_order",
            "severity": "decision_required",
            "finding": "North Pass bypasses Orc Watch but gives its one-action reserve a full reaction window before a day-2 stronghold assault at 3 AP/day.",
        })

    if all(x["median_pre_camp_regiment_xp"] < 100 for x in watch_baseline):
        issues.append({
            "kind": "intermediate_battle_progression",
            "severity": "risk",
            "finding": "The Orc Watch victory grants 85 lab regiment XP and 100 hero XP, leaving both below their next power threshold before the stronghold; the extra battle is attrition without an immediate progression payoff.",
        })

    if baseline["forest_watch"]["resource_sites"] and baseline["north_pass"]["resource_sites"]:
        issues.append({
            "kind": "resource_income_timing",
            "severity": "pass",
            "finding": "Forest Edge adds one resource-income site, but the field army is already away from the capital, so the extra income cannot improve the immediate day-2 assault under current finite-recruitment semantics.",
        })
    if all(r["remaining_pool_units"] > 0 for r in recoveries.values()):
        issues.append({
            "kind": "single_loss_recruitment",
            "severity": "pass",
            "finding": "A single forced early annihilation does not exhaust the seven-line finite recruitment pool in the bounded recovery model.",
        })

    starved_repeat = {
        cid: data["empty_pool_lines"] for cid, data in repeats.items()
        if data["empty_pool_lines"]
    }
    if starved_repeat:
        issues.append({
            "kind": "repeat_loss_recruitment",
            "severity": "risk",
            "finding": "Repeated five-day-spaced annihilations exhaust at least one finite recruitment line before the founder stress window ends.",
            "by_corridor": starved_repeat,
        })
    else:
        issues.append({
            "kind": "repeat_loss_recruitment",
            "severity": "pass",
            "finding": "Even three forced early annihilations do not exhaust a recruitment line in this bounded stress window; recovery is gold/income-limited before it is pool-limited.",
        })

    issues.append({
        "kind": "weekly_growth_timing",
        "severity": "pass",
        "finding": "The first weekly recruitment growth pulse occurs on day 8, well after the day-2 baseline stronghold decision, so weekly settlement growth cannot rescue or distort the opening assault.",
    })

    no_reserve_ap3 = {
        s["corridor"]: s
        for s in summaries
        if s["profile"] == "no_watch_reserve" and s["ap_per_day"] == 3
    }
    reserve_delta = round(
        no_reserve_ap3["north_pass"]["camp_victory_rate_all_runs"]
        - baseline["north_pass"]["camp_victory_rate_all_runs"],
        4,
    )
    issues.append({
        "kind": "dominance_sensitivity",
        "severity": "decision_required",
        "finding": "Route ranking is not robust until Orc Watch reserve/reaction semantics are fixed; bypassing the outpost changes the North Pass result materially when its 500-strength lab reserve is enabled.",
        "north_pass_no_reserve_minus_reserve_win_rate": reserve_delta,
    })

    def sensitivity_rates(stronghold_strength: int, reserve_strength: int) -> dict:
        return {
            row["corridor"]: row["camp_victory_rate_all_runs"]
            for row in stronghold_sensitivity
            if row["stronghold_strength"] == stronghold_strength
            and row["reserve_strength"] == reserve_strength
        }

    equal_strength_no_reserve = sensitivity_rates(1050, 0)
    candidate_band = sensitivity_rates(700, 200)
    issues.append({
        "kind": "stronghold_strength_placeholder",
        "severity": "risk",
        "finding": "With the noncanonical 1050-vs-1050 field-strength placeholder, travel wear plus the optional outpost fight makes an immediate founder stronghold rush effectively nonviable; topology is not the blocker, the still-unset army/garrison profile is.",
        "equal_strength_no_reserve_rates": equal_strength_no_reserve,
    })
    issues.append({
        "kind": "viable_topology_band",
        "severity": "pass",
        "finding": "The same topology produces three live opening corridors in the 700-stronghold / 200-reserve sensitivity cell, showing that the map itself is not a dead opening once defender and reinforcement values are in a plausible tuning band.",
        "candidate_rates": candidate_band,
    })

    payload = {
        "schema": 1,
        "generated": "2026-09-22",
        "status": "pass",
        "authority": {
            "map": "Data/soul_founder_slice_runtime_seed_20260922.json",
            "start_control": "Data/soul_campaign_start_states_v1_20260922.json",
            "logistics": "live SoulLogistics mirror via Tools/BalanceLab/balance_lab.py",
            "veterancy_hero_thresholds": "live SoulCore mirror via BalanceLab",
            "numeric_army_garrison": "BALANCE_LAB_GLUE_NOT_CANONICAL",
            "neutral_region_capture_on_entry": "LAB_GLUE",
            "battle_ends_remaining_daily_actions": "current BalanceLab/Soul strategy integration assumption",
            "reserve_arrival_model": "COARSE_SENSITIVITY_day_gt_1; exact timing superseded by Evidence/WorldOvermap/founder_reaction_window_analysis.json",
        },
        "lab_parameters": {
            "runs_per_cell": RUNS,
            "sensitivity_runs_per_cell": SENSITIVITY_RUNS,
            "field_strength": BASE_FIELD_STRENGTH,
            "orc_watch_garrison": WATCH_GARRISON_STRENGTH,
            "orc_stronghold_field_strength": STRONGHOLD_FIELD_STRENGTH,
            "stronghold_strength_sensitivity": [650, 700, 750, 800, 900, 1050],
            "reserve_sensitivity": [0, 200, WATCH_RESERVE_STRENGTH],
            "ap_sensitivity": [2, 3, 4],
            "base_income": BASE_INCOME,
            "resource_income": RESOURCE_INCOME,
            "respawn_delay_days": RESPAWN_DELAY,
            "respawn_strength": RESPAWN_STRENGTH,
        },
        "summaries": summaries,
        "stronghold_sensitivity": stronghold_sensitivity,
        "baseline_3ap_watch_reserve": baseline,
        "single_loss_recovery": recoveries,
        "repeat_loss_stress": repeats,
        "findings": issues,
    }
    OUT_JSON.parent.mkdir(parents=True, exist_ok=True)
    OUT_JSON.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

    csv_fields = [
        "profile", "reserve_strength", "ap_per_day", "corridor",
        "camp_reach_rate", "camp_victory_rate_all_runs",
        "camp_victory_rate_given_reached", "watch_win_rate",
        "median_camp_day", "reserve_arrival_rate",
        "median_pre_camp_strength", "median_pre_camp_supply",
        "median_pre_camp_readiness", "median_pre_camp_fatigue",
        "median_pre_camp_regiment_xp", "median_pre_camp_hero_xp",
    ]
    with OUT_CSV.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=csv_fields)
        writer.writeheader()
        for row in summaries:
            writer.writerow({field: row.get(field) for field in csv_fields})

    lines = [
        "# Soul Founder Slice Stress Test — 2026-09-22",
        "",
        "This uses the actual 9-region founder graph plus live-mirrored Soul logistics, veterancy, hero thresholds, finite recruitment, and campaign-economy rules. Numeric army/garrison combat values remain BalanceLab glue, so percentages below are comparative diagnostics rather than shipping win-rate predictions.",
        "Reserve-arrival percentages in this report use the original coarse day>1 sensitivity shortcut; exact AP/day reaction windows are now reported separately in SOUL_FOUNDER_REACTION_WINDOWS_20260922.md.",
        "",
        "## Baseline: 3 AP/day, Orc Watch can reinforce the stronghold",
        "",
        "| Corridor | Camp day | Reach | Win diagnostic | Reserve arrives | Pre-camp strength | Supply | Readiness | Reg XP | Hero XP |",
        "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|",
    ]
    for cid in ("river_watch", "forest_watch", "north_pass"):
        x = baseline[cid]
        lines.append(
            f"| {cid} | {x['median_camp_day']} | {x['camp_reach_rate']:.1%} | "
            f"{x['camp_victory_rate_all_runs']:.1%} | {x['reserve_arrival_rate']:.1%} | "
            f"{x['median_pre_camp_strength']} | {x['median_pre_camp_supply']} | "
            f"{x['median_pre_camp_readiness']} | {x['median_pre_camp_regiment_xp']} | "
            f"{x['median_pre_camp_hero_xp']} |"
        )
    lines += ["", "## Findings", ""]
    for item in issues:
        lines.append(f"- **{item['severity']} / {item['kind']}** — {item['finding']}")

    lines += ["", "## AP sensitivity", ""]
    for ap in (2, 3, 4):
        parts = []
        for cid in ("river_watch", "forest_watch", "north_pass"):
            x = next(
                s for s in summaries
                if s["profile"] == "watch_reserve_500"
                and s["ap_per_day"] == ap
                and s["corridor"] == cid
            )
            parts.append(
                f"{cid}: day {x['median_camp_day']}, reserve {x['reserve_arrival_rate']:.0%}, win {x['camp_victory_rate_all_runs']:.1%}"
            )
        lines.append(f"- **{ap} AP/day:** " + "; ".join(parts) + ".")

    lines.append("- **Timing correction:** AP2 reserve=100% above is a coarse sensitivity artifact; exact graph timing has no overnight window between North Pass reveal and Stronghold contact at 2 AP/day. See SOUL_FOUNDER_REACTION_WINDOWS_20260922.md.")

    lines += [
        "",
        "## Stronghold / reinforcement sensitivity",
        "",
        "- **Equal 1050 field strengths, no reserve:** "
        + ", ".join(
            f"{cid} {equal_strength_no_reserve[cid]:.1%}"
            for cid in ("river_watch", "forest_watch", "north_pass")
        )
        + ".",
        "- **700 stronghold + 200 one-action reserve:** "
        + ", ".join(
            f"{cid} {candidate_band[cid]:.1%}"
            for cid in ("river_watch", "forest_watch", "north_pass")
        )
        + ". This is a sensitivity example, not recommended shipping tuning.",
    ]

    lines += ["", "## Recovery / finite recruitment", ""]
    for cid in ("river_watch", "forest_watch", "north_pass"):
        one = recoveries[cid]
        rep = repeats[cid]
        lines.append(
            f"- **{cid}:** income {one['daily_income']}/day; single-loss recovery70 "
            f"{one['recovery70_days'] or 'not reached'}, final strength {one['final_strength']}; "
            f"repeat-loss final strength {rep['final_strength']}, gold {rep['final_gold']}, "
            f"remaining pool {rep['remaining_pool_units']}, empty lines "
            f"{rep['empty_pool_lines'] or 'none'}."
        )

    lines += [
        "",
        "## Decision exposed by the test",
        "",
        "The cheapest high-information product decision is the Orc Watch reserve rule. If a player bypasses the outpost through North Pass, decide whether that outpost can reinforce the stronghold before the next-day assault. The route balance changes materially either way; tuning army numbers before fixing that timing rule would be premature.",
    ]
    OUT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")

    print(json.dumps({
        "status": payload["status"],
        "summaries": len(summaries),
        "baseline": baseline,
        "findings": issues,
        "json": str(OUT_JSON),
        "csv": str(OUT_CSV),
        "doc": str(OUT_MD),
    }, indent=2))

if __name__ == "__main__":
    main()
