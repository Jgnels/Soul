"""Deterministic founder-slice campaign stress lab on Soul's real 9-region overmap."""
from __future__ import annotations

import csv
import heapq
import json
import math
from copy import deepcopy
from pathlib import Path

from BalanceLab.balance_lab import (
    Params,
    apply_travel,
    end_day_logistics,
    make_faction,
    rank_stats,
    recruit_at_capital,
    weekly_growth,
)

ROOT = Path(__file__).resolve().parents[1]
SEED = ROOT / "Data" / "soul_founder_slice_runtime_seed_20260922.json"
STARTS = ROOT / "Data" / "soul_campaign_start_states_v1_20260922.json"
HANDOFF = ROOT / "Data" / "soul_overmap_battle_handoff_v1_20260922.json"
OUT = ROOT / "Evidence" / "WorldOvermap" / "founder_campaign_stress.json"
CSV_OUT = ROOT / "Evidence" / "WorldOvermap" / "founder_campaign_stress.csv"
DOC = ROOT / "Docs" / "SOUL_FOUNDER_CAMPAIGN_STRESS_20260922.md"

seed = json.loads(SEED.read_text(encoding="utf-8"))
starts = json.loads(STARTS.read_text(encoding="utf-8"))
handoff_doc = json.loads(HANDOFF.read_text(encoding="utf-8"))
scenario = starts["scenarios"]["founder_human_orc_micro"]
regions = seed["regions"]
routes = seed["routes"]
owners = scenario["owners"]
player = scenario["player_faction"]
capital = scenario["player_start_region"]
goal = scenario["enemy_primary_region"]

edge_by_pair = {}
adj: dict[str, list[tuple[str, dict]]] = {rid: [] for rid in regions}
for route_id, route in routes.items():
    a, b = route["a"], route["b"]
    edge_by_pair[frozenset((a, b))] = route
    adj[a].append((b, route))
    adj[b].append((a, route))

handoff_by_pair = {
    (h["source_region"], h["destination_region"]): h
    for h in handoff_doc["handoffs"]
}

STRATEGIES = {
    "road_gate": ["human_capital", "crossroads", "river_ford", "orc_watch", "orc_camp"],
    "forest_watch": ["human_capital", "crossroads", "forest_edge", "orc_watch", "orc_camp"],
    "north_pass": ["human_capital", "crossroads", "forest_edge", "north_pass", "orc_camp"],
    "quarry_detour": [
        "human_capital", "crossroads", "old_quarry", "crossroads",
        "river_ford", "orc_watch", "orc_camp",
    ],
    "shrine_detour": [
        "human_capital", "crossroads", "old_quarry", "ancient_shrine",
        "old_quarry", "crossroads", "forest_edge", "north_pass", "orc_camp",
    ],
}
CORE_STRATEGIES = ("road_gate", "forest_watch", "north_pass")
ENCOUNTER_POLICIES = ("hostile_only", "all_marked")
AP_VALUES = (2, 3, 4)
PREP_DAYS = (0, 3, 7)
RESOURCE_INCOME_VALUES = (6, 9, 12)
VICTORY_XP_REFERENCE = 100
PROJECTION_DAY = 14

def validate_paths() -> list[str]:
    errors = []
    for name, path in STRATEGIES.items():
        if path[0] != capital or path[-1] != goal:
            errors.append(f"{name}: path endpoints do not match founder scenario")
        for a, b in zip(path, path[1:]):
            if frozenset((a, b)) not in edge_by_pair:
                errors.append(f"{name}: non-edge {a}->{b}")
    return errors

def battle_required(region_id: str, policy: str) -> bool:
    owner = owners.get(region_id)
    if owner and owner != player:
        return True
    if policy == "all_marked" and "encounter" in set(regions[region_id].get("site_roles", [])):
        return True
    return False

def friendly_region(region_id: str, captured_resources: set[str], captured_goal: bool) -> bool:
    if region_id in (capital, "crossroads"):
        return True
    if region_id in captured_resources:
        return True
    if region_id == goal and captured_goal:
        return True
    return False

def maintain_day(faction, params: Params, day: int, captured_resources: set[str]) -> None:
    faction.gold += params.base_income + len(captured_resources) * params.resource_income
    if day > 1 and (day - 1) % 7 == 0:
        weekly_growth(faction, params)
    if faction.army.region_id == capital:
        recruit_at_capital(faction, params)

def shortest_route(start: str, target: str) -> dict | None:
    queue = [(0, 0, start, [start])]
    best: dict[str, tuple[int, int]] = {}
    while queue:
        actions, logistics, cur, path = heapq.heappop(queue)
        prior = best.get(cur)
        if prior is not None and prior <= (actions, logistics):
            continue
        best[cur] = (actions, logistics)
        if cur == target:
            hostile_transit = sum(
                1 for rid in path[1:]
                if owners.get(rid) not in (None, player)
            )
            return {
                "path": path,
                "movement_actions": actions,
                "logistics_cost": logistics,
                "hostile_transit_regions": hostile_transit,
            }
        for nxt, route in sorted(adj[cur], key=lambda x: x[0]):
            heapq.heappush(
                queue,
                (
                    actions + route["action_cost"],
                    logistics + route["logistics_movement_cost"],
                    nxt,
                    path + [nxt],
                ),
            )
    return None

def reinforcement_options(source: str, destination: str, ap_per_day: int) -> list[dict]:
    h = handoff_by_pair[(source, destination)]
    options = []
    for slot in h["reinforcement_context"]["alternate_adjacent_entry_slots"]:
        route = shortest_route("crossroads", slot["source_region"])
        if route is None:
            continue
        clear_actions = route["hostile_transit_regions"]
        total_actions = route["movement_actions"] + clear_actions + 1
        options.append({
            "entry_source_region": slot["source_region"],
            "entry_direction": slot["entry_direction"],
            "route_from_crossroads": route["path"],
            "movement_actions": route["movement_actions"],
            "hostile_clear_actions": clear_actions,
            "battle_entry_action": 1,
            "total_actions": total_actions,
            "days_at_ap": math.ceil(total_actions / ap_per_day),
            "clean_route": clear_actions == 0,
            "logistics_cost_before_entry": route["logistics_cost"],
        })
    return sorted(options, key=lambda x: (x["days_at_ap"], x["total_actions"], x["logistics_cost_before_entry"], x["entry_source_region"]))

def simulate_path(
    strategy: str,
    ap_per_day: int,
    prep_days: int,
    encounter_policy: str,
    resource_income: int,
) -> dict:
    path = STRATEGIES[strategy]
    params = Params(ap=ap_per_day, resource_income=resource_income)
    faction = make_faction("humans", capital, params)
    faction.owned = {capital, "crossroads"}
    faction.army.region_id = capital

    day = 1
    actions_used = 0
    current = capital
    captured_resources: set[str] = set()
    captured_goal = False
    battle_regions: list[str] = []
    action_log: list[dict] = []
    supply_samples = [faction.army.logistics.supply]
    readiness_samples = [faction.army.logistics.readiness]
    fatigue_samples = [faction.army.logistics.fatigue]
    maintain_day(faction, params, day, captured_resources)
    initial_strength_after_recruit = faction.army.strength
    initial_gold_after_recruit = faction.gold

    def finish_day(staging: bool = False) -> None:
        nonlocal day, actions_used
        if staging:
            end_day_logistics(faction.army.logistics, settlement=False, friendly=False)
        else:
            settlement = current == capital or (current == goal and captured_goal)
            end_day_logistics(
                faction.army.logistics,
                settlement=settlement,
                friendly=friendly_region(current, captured_resources, captured_goal),
            )
        supply_samples.append(faction.army.logistics.supply)
        readiness_samples.append(faction.army.logistics.readiness)
        fatigue_samples.append(faction.army.logistics.fatigue)
        day += 1
        actions_used = 0
        maintain_day(faction, params, day, captured_resources)

    for _ in range(prep_days):
        finish_day(staging=False)

    for src, dst in zip(path, path[1:]):
        if current != src:
            raise RuntimeError(f"{strategy}: current region {current} != expected source {src}")
        if actions_used >= ap_per_day:
            finish_day(staging=False)

        route = edge_by_pair[frozenset((src, dst))]
        needs_battle = battle_required(dst, encounter_policy)
        hostile = owners.get(dst) not in (None, player)
        apply_travel(
            faction.army.logistics,
            route["logistics_movement_cost"],
            route["road"],
            hostile,
        )
        actions_used += route["action_cost"]
        action_log.append({
            "day": day,
            "action": "move_to_contact" if needs_battle else "move",
            "source": src,
            "destination": dst,
            "route": route["route"],
            "road": route["road"],
            "logistics_cost": route["logistics_movement_cost"],
        })
        supply_samples.append(faction.army.logistics.supply)
        readiness_samples.append(faction.army.logistics.readiness)
        fatigue_samples.append(faction.army.logistics.fatigue)

        if needs_battle:
            if actions_used >= ap_per_day:
                finish_day(staging=True)
            actions_used += 1
            battle_regions.append(dst)
            faction.hero.xp += VICTORY_XP_REFERENCE
            faction.army.regiment_xp += VICTORY_XP_REFERENCE
            current = dst
            faction.army.region_id = dst
            action_log.append({
                "day": day,
                "action": "battle_commit",
                "region": dst,
                "xp_reference_award": VICTORY_XP_REFERENCE,
            })
        else:
            current = dst
            faction.army.region_id = dst

        if "resource_site" in set(regions[dst].get("site_roles", [])):
            captured_resources.add(dst)
            faction.owned.add(dst)
        if dst == goal:
            captured_goal = True
            faction.owned.add(dst)

    completion_day = day
    completion_ap_used = actions_used
    completion_gold = faction.gold
    completion_strength = faction.army.strength
    hero_level = faction.hero.level
    rank_name, rank_combat_permille, rank_morale = rank_stats(faction.army.regiment_xp)

    projected = deepcopy(faction)
    projected_resources = set(captured_resources)
    projection_gold = projected.gold
    next_day = day + 1
    while next_day <= PROJECTION_DAY:
        projection_gold += params.base_income + len(projected_resources) * params.resource_income
        if next_day > 1 and (next_day - 1) % 7 == 0:
            weekly_growth(projected, params)
        next_day += 1
    projected.gold = projection_gold
    pool_available = sum(pool.available for pool in projected.pools.values())
    pool_capacity = sum(pool.capacity for pool in projected.pools.values())
    reserve_strength = sum(
        pool.available * pool.strength for pool in projected.pools.values()
    )

    final_source = path[-2]
    reinforce = reinforcement_options(final_source, goal, ap_per_day)

    return {
        "strategy": strategy,
        "path": path,
        "ap_per_day": ap_per_day,
        "prep_days": prep_days,
        "encounter_policy": encounter_policy,
        "resource_income": resource_income,
        "completion_day": completion_day,
        "completion_ap_used": completion_ap_used,
        "total_logged_actions": len(action_log),
        "moves": len(path) - 1,
        "battles": len(battle_regions),
        "battle_regions": battle_regions,
        "captured_resources": sorted(captured_resources),
        "logistics_cost_sum": sum(
            edge_by_pair[frozenset((a, b))]["logistics_movement_cost"]
            for a, b in zip(path, path[1:])
        ),
        "road_edges": sum(
            1 for a, b in zip(path, path[1:])
            if edge_by_pair[frozenset((a, b))]["road"]
        ),
        "chokepoint_edges": sum(
            1 for a, b in zip(path, path[1:])
            if edge_by_pair[frozenset((a, b))]["chokepoint"]
        ),
        "supply_at_objective": faction.army.logistics.supply,
        "readiness_at_objective": faction.army.logistics.readiness,
        "fatigue_at_objective": faction.army.logistics.fatigue,
        "minimum_supply": min(supply_samples),
        "minimum_readiness": min(readiness_samples),
        "maximum_fatigue": max(fatigue_samples),
        "initial_strength_after_recruit": initial_strength_after_recruit,
        "initial_gold_after_recruit": initial_gold_after_recruit,
        "strength_at_objective": completion_strength,
        "gold_at_objective": completion_gold,
        "hero_xp_reference": faction.hero.xp,
        "hero_level_from_current_curve": hero_level,
        "regiment_xp_reference": faction.army.regiment_xp,
        "regiment_rank_from_current_curve": rank_name,
        "rank_combat_bonus_permille": rank_combat_permille,
        "rank_morale_bonus": rank_morale,
        "projection_day": PROJECTION_DAY,
        "gold_projection_day14": projected.gold,
        "recruit_pool_available_day14": pool_available,
        "recruit_pool_capacity": pool_capacity,
        "reserve_recruit_strength_day14": reserve_strength,
        "reinforcement_options_for_final_battle": reinforce,
        "action_log": action_log,
    }

errors = validate_paths()
runs = []
for ap in AP_VALUES:
    for prep in PREP_DAYS:
        for policy in ENCOUNTER_POLICIES:
            for resource_income in RESOURCE_INCOME_VALUES:
                for strategy in STRATEGIES:
                    runs.append(simulate_path(strategy, ap, prep, policy, resource_income))

def pick(strategy: str, *, ap=3, prep=0, policy="hostile_only", resource_income=9) -> dict:
    for row in runs:
        if (
            row["strategy"] == strategy
            and row["ap_per_day"] == ap
            and row["prep_days"] == prep
            and row["encounter_policy"] == policy
            and row["resource_income"] == resource_income
        ):
            return row
    raise KeyError(strategy)

baseline_hostile = {name: pick(name) for name in CORE_STRATEGIES}
baseline_marked = {
    name: pick(name, policy="all_marked")
    for name in CORE_STRATEGIES
}
prep7 = {
    name: pick(name, prep=7)
    for name in CORE_STRATEGIES
}

findings = []
road = baseline_hostile["road_gate"]
forest = baseline_hostile["forest_watch"]
north = baseline_hostile["north_pass"]
if road["completion_day"] == forest["completion_day"] == north["completion_day"]:
    findings.append({
        "id": "equal_day_core_corridors",
        "severity": "info",
        "finding": "All three core founder corridors reach the stronghold on the same campaign day at 3 AP/day under hostile-only encounters.",
        "evidence": {k: v["completion_day"] for k, v in baseline_hostile.items()},
    })
if road["minimum_readiness"] > forest["minimum_readiness"] > north["minimum_readiness"]:
    findings.append({
        "id": "route_logistics_gradient",
        "severity": "info",
        "finding": "Road -> forest -> pass produces a clean readiness gradient; route geometry is mechanically legible rather than cosmetic.",
        "evidence": {k: v["minimum_readiness"] for k, v in baseline_hostile.items()},
    })
if forest["gold_projection_day14"] > road["gold_projection_day14"]:
    findings.append({
        "id": "forest_resource_tradeoff",
        "severity": "info",
        "finding": "Forest Edge gives the forest routes an economic payoff that offsets their worse logistics.",
        "evidence": {
            "road_day14_gold": road["gold_projection_day14"],
            "forest_day14_gold": forest["gold_projection_day14"],
            "delta": forest["gold_projection_day14"] - road["gold_projection_day14"],
        },
    })
fm = baseline_marked["forest_watch"]
nm = baseline_marked["north_pass"]
if (
    fm["completion_day"] <= nm["completion_day"]
    and fm["minimum_readiness"] >= nm["minimum_readiness"]
    and fm["battles"] == nm["battles"]
    and fm["captured_resources"] == nm["captured_resources"]
):
    findings.append({
        "id": "north_pass_incentive_gap",
        "severity": "action",
        "finding": "With every marked encounter active, North Pass has no systemic strategic reward in the current model: same battle count/resource payoff, no faster completion, and worse logistics than Forest Watch.",
        "recommended_test": "Give North Pass a concrete tactical/intelligence/avoidance benefit before changing topology; validate it in battle rather than compensating with arbitrary AP discounts.",
    })
rm = baseline_marked["road_gate"]
if rm["regiment_rank_from_current_curve"] != fm["regiment_rank_from_current_curve"]:
    findings.append({
        "id": "encounter_progression_pressure",
        "severity": "watch",
        "finding": "Marked encounters make route choice alter hero/regiment progression before the stronghold.",
        "evidence": {
            "road": {
                "battles": rm["battles"],
                "hero_level": rm["hero_level_from_current_curve"],
                "regiment_rank": rm["regiment_rank_from_current_curve"],
            },
            "forest": {
                "battles": fm["battles"],
                "hero_level": fm["hero_level_from_current_curve"],
                "regiment_rank": fm["regiment_rank_from_current_curve"],
            },
            "north": {
                "battles": nm["battles"],
                "hero_level": nm["hero_level_from_current_curve"],
                "regiment_rank": nm["regiment_rank_from_current_curve"],
            },
        },
        "note": "100 XP/victory is donor-reference stress glue, not a locked Soul reward.",
    })

prep_evidence = {}
for name in CORE_STRATEGIES:
    zero = baseline_hostile[name]
    seven = prep7[name]
    prep_evidence[name] = {
        "strength_gain": seven["strength_at_objective"] - zero["strength_at_objective"],
        "completion_day_delay": seven["completion_day"] - zero["completion_day"],
        "gold_delta_at_objective": seven["gold_at_objective"] - zero["gold_at_objective"],
    }
findings.append({
    "id": "seven_day_preparation_tradeoff",
    "severity": "info",
    "finding": "A full weekly-growth wait is measurable against its seven-day opportunity cost; this is the first founder-slice finite-pool preparation check.",
    "evidence": prep_evidence,
})

reinforcement_summary = {
    name: baseline_hostile[name]["reinforcement_options_for_final_battle"]
    for name in CORE_STRATEGIES
}
findings.append({
    "id": "reinforcement_timing",
    "severity": "info",
    "finding": "Final-battle reinforcement entries are now measured from Crossroads using real graph distance, hostile-clear actions, and the battle-entry action.",
    "evidence": reinforcement_summary,
})

sensitivity = {
    "completion_day_by_ap_hostile_only": {
        name: {str(ap): pick(name, ap=ap)["completion_day"] for ap in AP_VALUES}
        for name in CORE_STRATEGIES
    },
    "resource_income_day14_gold_forest_watch": {
        str(value): pick("forest_watch", resource_income=value)["gold_projection_day14"]
        for value in RESOURCE_INCOME_VALUES
    },
    "prep_strength_and_delay": prep_evidence,
}

result = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "pass" if not errors else "fail",
    "authority": {
        "topology": "Data/soul_founder_slice_runtime_seed_20260922.json",
        "start_control": "Data/soul_campaign_start_states_v1_20260922.json",
        "travel_logistics": "Tools/BalanceLab/balance_lab.py LIVE MIRROR functions applied to real founder edge costs",
        "recruitment_economy": "Tools/BalanceLab/balance_lab.py LAB GLUE until settlement-local runtime pools/costs are authoritative",
        "hero_xp": "current Soul XP curve; 100 XP/victory donor-reference stress assumption",
        "battle_attrition": "NOT_MODELED",
        "town_construction_progression": "NOT_MODELED_UNRESOLVED_RUNTIME_RULE",
    },
    "assumptions": [
        "3 AP/day is the current center; 2 and 4 are sensitivity bounds.",
        "Resource sites are captured when an uncontested route action enters them.",
        "hostile_only requires battle commitment for start-hostile regions; all_marked additionally activates regions tagged encounter.",
        "Battle commitment costs one action; battle attrition is intentionally excluded until realtime battle feeds back authoritative consequences.",
        "100 XP per victory is reference-only stress glue used to expose progression pressure, not a shipping reward.",
        "Day-14 economy projection continues base/resource income and weekly recruitment-pool growth but does not invent occupation income or field-army reinforcement.",
    ],
    "run_count": len(runs),
    "core_baseline_hostile_only": baseline_hostile,
    "core_baseline_all_marked": baseline_marked,
    "sensitivity": sensitivity,
    "findings": findings,
    "errors": errors,
}
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")

fieldnames = [
    "strategy", "ap_per_day", "prep_days", "encounter_policy", "resource_income",
    "completion_day", "total_logged_actions", "battles", "logistics_cost_sum",
    "supply_at_objective", "readiness_at_objective", "fatigue_at_objective",
    "strength_at_objective", "gold_at_objective", "gold_projection_day14",
    "hero_level_from_current_curve", "regiment_rank_from_current_curve",
    "recruit_pool_available_day14", "reserve_recruit_strength_day14",
]
with CSV_OUT.open("w", encoding="utf-8", newline="") as handle:
    writer = csv.DictWriter(handle, fieldnames=fieldnames)
    writer.writeheader()
    for row in runs:
        writer.writerow({key: row[key] for key in fieldnames})

lines = [
    "# Soul Founder Campaign Stress — 2026-09-22",
    "",
    "Deterministic non-UE stress test using the actual 9-region founder topology.",
    "This is a decision aid, not production balance authority.",
    "",
    "## Baseline: 3 AP/day, no intentional prep, resource income 9",
    "",
    "| Corridor | Encounters | Day | Logistics | Supply | Readiness | Day-14 gold | Hero level | Regiment |",
    "|---|---:|---:|---:|---:|---:|---:|---:|---|",
]
for name in CORE_STRATEGIES:
    row = baseline_hostile[name]
    lines.append(
        f"| {name} | {row['battles']} | {row['completion_day']} | {row['logistics_cost_sum']} | "
        f"{row['supply_at_objective']} | {row['readiness_at_objective']} | "
        f"{row['gold_projection_day14']} | {row['hero_level_from_current_curve']} | "
        f"{row['regiment_rank_from_current_curve']} |"
    )
lines += ["", "## If every marked encounter fires", ""]
for name in CORE_STRATEGIES:
    row = baseline_marked[name]
    lines.append(
        f"- **{name}**: {row['battles']} battles; day {row['completion_day']}; "
        f"hero level {row['hero_level_from_current_curve']}; regiment {row['regiment_rank_from_current_curve']}; "
        f"readiness {row['readiness_at_objective']}."
    )
lines += ["", "## Findings", ""]
for item in findings:
    lines.append(f"- **{item['id']} ({item['severity']})** — {item['finding']}")
    if item.get("recommended_test"):
        lines.append(f"  - Next test: {item['recommended_test']}")
lines += [
    "",
    "## Boundaries",
    "",
    "- No realtime-battle attrition or casualty feedback is invented here.",
    "- Settlement-local recruitment identity remains a known runtime gap; the seven-line pool model is existing Balance Lab glue.",
    "- Town construction progression remains intentionally excluded until its concurrency/day semantics are decided.",
    "- Special-site rewards (including Lost Shrine) are not assigned synthetic power values.",
]
DOC.write_text("\n".join(lines) + "\n", encoding="utf-8")

print(json.dumps({
    "status": result["status"],
    "run_count": result["run_count"],
    "baseline": {
        name: {
            "day": row["completion_day"],
            "battles": row["battles"],
            "supply": row["supply_at_objective"],
            "readiness": row["readiness_at_objective"],
            "day14_gold": row["gold_projection_day14"],
        }
        for name, row in baseline_hostile.items()
    },
    "finding_ids": [x["id"] for x in findings],
    "errors": errors,
}, indent=2))
if errors:
    raise SystemExit(1)
