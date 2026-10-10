#!/usr/bin/env python3
"""
Deterministic 30-day campaign economy model for Soul.

Source authority (frozen snapshot 2c3a9055d71a87ed3ed8b8abf08503efa1218f7c):
  - Start gold ...................... 3000   (SoulFounderPlaytestStateSubsystem.cpp:290)
  - Daily income ................... 450/day (SoulFounderPlaytestStateSubsystem.cpp:290)
  - Knight recruitment cost ........ 140     (:292)
  - Knight pool: avail 8, weekly +4, cap 24 (:291)
  - Archer cost 180 / Guard cost 220 (Heartland alpha, :307)
  - AP/day .......................... 3       (SoulCampaign.h:19)
  - Weekly growth cadence: (Day-1)%7==0 => days 1,8,15,22,29 (SoulCampaign.cpp:40)
  - Building costs/days: HeartlandDevelopment.json (tavern 200/1, arcane_hall 300/1,
      mage_academy 600/2, high_conclave 900/3, market 400/2 maxLvl2, barracks 500/2 maxLvl2)
  - Build cost deducted atomically once at construction start (SoulTown.cpp)
  - Strategic sites (own + occupy + 1 AP each): windmill +100 gold, quarry +140 gold,
      shrine +20 mana (HeartlandDevelopment.json)
  - Market L2 operational: +100 gold/day (:650)
  - Barracks L2 operational: +2 knights/week to pool (:653)
  - Quest region-capture reward: +250 gold once (:504)
  - Siege/capital capture reward: +600 gold once (:617)
  - Tavern hero hire: -1200 gold once (:670)
  - No building upkeep, no army upkeep, no siege gold cost, no diplomacy/ransom cost.
  - AI factions: start gold = player start (3000), DailyIncome EMPTY, NO recruitment
      pools, strategic AI OFF (SoulSixFactionCampaign.cpp:75; LastAIReport "strategic AI OFF").

All values above are VERIFIED FROM CURRENT SNAPSHOT. The models apply ONLY these rules.
Site income, market/barracks bonuses and quest/siege rewards require the Heartland profile
and lawful ownership/occupation; where a scenario assumes them, the archetype notes say so.

This script is deterministic (no RNG) and emits 30_day_simulation.json.
"""
import json, os

# ---- Verified runtime constants ----
START_GOLD = 3000
BASE_INCOME = 450          # gold/day, player founder economy
AP_PER_DAY = 3
KNIGHT_COST = 140
KNIGHT_START_AVAIL = 8
KNIGHT_WEEKLY_GROWTH = 4
KNIGHT_CAP = 24
ARCHER_COST = 180
GUARD_COST = 220
WINDMILL_GOLD = 100        # per 1 AP, repeatable 1/day while owned+occupied
QUARRY_GOLD = 140          # per 1 AP
MARKET_L2_DAILY = 100      # gold/day when market L2 operational
BARRACKS_L2_WEEKLY_POOL = 2
QUEST_CAPTURE_REWARD = 250
SIEGE_CAPTURE_REWARD = 600
TAVERN_HERO_COST = 1200
TROOP_HARD_CAP = 250

BUILDINGS = {
    "human.tavern":       {"gold": 200, "days": 1},
    "human.arcane_hall":  {"gold": 300, "days": 1},
    "human.mage_academy": {"gold": 600, "days": 2},
    "human.high_conclave":{"gold": 900, "days": 3},
    "human.market":       {"gold": 400, "days": 2},   # maxLvl2 (L2 enables daily +100)
    "human.barracks":     {"gold": 500, "days": 2},   # maxLvl2 (L2 enables weekly +2 pool)
}

DAYS = 30

def is_growth_day(day):
    # (Day-1)%7==0 => days 1,8,15,22,29
    return (day - 1) % 7 == 0

def simulate(profile):
    """
    profile fields:
      name, description, income (gold/day), uses market_l2 bonus, barracks_l2 bonus,
      site_gold_per_day (sum of windmill/quarry gold claimed each day, 0 if not pursued),
      build_plan: dict day->list of building ids started that day (cost deducted that day),
      recruit_plan: dict day->knights recruited that day (bounded by pool avail + affordability),
      one_time_rewards: dict day->gold (quest/siege capture rewards earned that day),
      hero_hire_day: int or None (spend 1200 once),
      market_l2_day: int or None (day market reaches L2 => daily +100 starts next day),
      barracks_l2_day: int or None (day barracks reaches L2 => weekly +2 pool from then).
    """
    gold = START_GOLD
    pool_avail = KNIGHT_START_AVAIL
    troops = 9  # founder company baseline (knight-heavy); informational only
    rows = []
    cumulative_recruited = 0
    for day in range(1, DAYS + 1):
        # --- AdvanceDay order mirrors runtime: ++Day, reset AP, add DailyIncome, weekly pool growth ---
        income_today = profile["income"]
        # Market L2 daily bonus applies on/after the day it becomes operational
        if profile.get("market_l2_day") and day > profile["market_l2_day"]:
            income_today += MARKET_L2_DAILY
        gold += income_today

        # Weekly pool growth
        if is_growth_day(day):
            growth = KNIGHT_WEEKLY_GROWTH
            if profile.get("barracks_l2_day") and day >= profile["barracks_l2_day"]:
                growth += BARRACKS_L2_WEEKLY_POOL
            pool_avail = min(KNIGHT_CAP, pool_avail + growth)

        # --- Player actions for the day (deterministic plan) ---
        # Site income (gold sites worked; each costs 1 AP; AP cap 3/day)
        site_gold = profile.get("site_gold_per_day", 0)
        gold += site_gold

        # One-time rewards earned this day
        reward = profile.get("one_time_rewards", {}).get(day, 0)
        gold += reward

        # Building construction started today (atomic deduction)
        built = []
        for bid in profile.get("build_plan", {}).get(day, []):
            cost = BUILDINGS[bid]["gold"]
            if gold >= cost:
                gold -= cost
                built.append(bid)

        # Hero hire
        hired = False
        if profile.get("hero_hire_day") == day and gold >= TAVERN_HERO_COST:
            gold -= TAVERN_HERO_COST
            hired = True

        # Recruitment
        want = profile.get("recruit_plan", {}).get(day, 0)
        can_by_gold = gold // KNIGHT_COST
        recruited = min(want, pool_avail, can_by_gold)
        if recruited > 0 and troops + recruited > TROOP_HARD_CAP:
            recruited = max(0, TROOP_HARD_CAP - troops)
        gold -= recruited * KNIGHT_COST
        pool_avail -= recruited
        troops += recruited
        cumulative_recruited += recruited

        rows.append({
            "day": day,
            "gold_end_of_day": gold,
            "income_applied": income_today,
            "site_gold": site_gold,
            "one_time_reward": reward,
            "buildings_started": built,
            "hero_hired": hired,
            "knights_recruited": recruited,
            "pool_available_end": pool_avail,
            "troops_end": troops,
            "action_points_per_day": AP_PER_DAY,
        })
    return {
        "profile": profile["name"],
        "description": profile["description"],
        "label": "DESIGN PROPOSAL (model built from VERIFIED FROM CURRENT SNAPSHOT rules/values)",
        "assumptions": profile.get("assumptions", []),
        "end_gold": rows[-1]["gold_end_of_day"],
        "end_troops": rows[-1]["troops_end"],
        "total_knights_recruited": cumulative_recruited,
        "daily": rows,
    }

# ---------------- Archetype profiles ----------------

# 1) Conservative player: bank income, minimal recruiting, build economy (market -> barracks),
#    work one safe gold site, no expansion, no hero hire.
conservative = {
    "name": "conservative_player",
    "description": "Banks the 450/day income, builds market then barracks, works one owned gold site (windmill) daily, recruits sparingly. No expansion, no hero hire.",
    "assumptions": [
        "Heartland profile active (market/barracks/site income exist only there).",
        "One owned+occupied gold site worked each day for +100 (windmill).",
        "Market reaches L2 on day 4 (built day1 cost400, 2 days, then level-up assumed by day4) enabling +100/day from day 5.",
        "Barracks L2 by day 10 enabling +2 pool/week from day 10.",
    ],
    "income": BASE_INCOME,
    "site_gold_per_day": WINDMILL_GOLD,
    "market_l2_day": 4,
    "barracks_l2_day": 10,
    "build_plan": {1: ["human.market"], 6: ["human.barracks"]},
    "recruit_plan": {8: 4, 15: 4, 22: 4},
    "one_time_rewards": {},
    "hero_hire_day": None,
}

# 2) Expansionist player: aggressive building + recruiting + two gold sites + quest rewards + hero.
expansionist = {
    "name": "expansionist_player",
    "description": "Pursues both gold sites (windmill+quarry = +240/day for 2 AP), builds tavern->market->barracks->arcane->academy, hires tavern hero, recruits to pool cap, earns quest+siege capture rewards.",
    "assumptions": [
        "Heartland profile active.",
        "Two owned+occupied gold sites worked daily (windmill 100 + quarry 140 = 240) using 2 of 3 AP.",
        "Quest region-capture reward (+250) on days 5, 12, 19 (3 captures).",
        "Siege/capital capture reward (+600) on day 25.",
        "Market L2 day 7 (=> +100/day from day 8); Barracks L2 day 14 (=> +2 pool/week from day14).",
        "Hero hired day 20 (-1200).",
    ],
    "income": BASE_INCOME,
    "site_gold_per_day": WINDMILL_GOLD + QUARRY_GOLD,
    "market_l2_day": 7,
    "barracks_l2_day": 14,
    "build_plan": {
        1: ["human.tavern"],
        2: ["human.market"],
        5: ["human.barracks"],
        9: ["human.arcane_hall"],
        12: ["human.mage_academy"],
    },
    "recruit_plan": {8: 8, 15: 10, 22: 10, 29: 10},
    "one_time_rewards": {5: QUEST_CAPTURE_REWARD, 12: QUEST_CAPTURE_REWARD, 19: QUEST_CAPTURE_REWARD, 25: SIEGE_CAPTURE_REWARD},
    "hero_hire_day": 20,
}

# 3) AI baseline: VERIFIED runtime reality. Start gold 3000, NO income, NO pools, strategic AI OFF.
ai_baseline = {
    "name": "ai_baseline",
    "description": "Mirrors the VERIFIED six-faction runtime AI economy: 3000 start gold, zero DailyIncome, zero recruitment pools, strategic AI OFF. The AI never spends, recruits, builds, or earns.",
    "assumptions": [
        "SoulSixFactionCampaign.cpp:75 sets AI gold = player start gold (3000); DailyIncome empty; no pools.",
        "AdvanceDay(P.Value.Economy) adds nothing; LastAIReport: 'strategic AI OFF'.",
        "This is the actual current state, NOT a design proposal. Treat gold as frozen stock.",
    ],
    "income": 0,
    "site_gold_per_day": 0,
    "market_l2_day": None,
    "barracks_l2_day": None,
    "build_plan": {},
    "recruit_plan": {},   # AI has no pools; cannot recruit regardless
    "one_time_rewards": {},
    "hero_hire_day": None,
}

# 4) War-heavy play: income + both sites + heavy recruiting to pool cap, repeated siege rewards,
#    minimal building; models attrition replacement by continuous recruitment.
war_heavy = {
    "name": "war_heavy_player",
    "description": "Spends almost all gold replacing battle losses by recruiting to the pool cap every week; works both gold sites; earns repeated siege/quest rewards; builds only barracks for faster pool refill. Models the cost of continuous warfare under current (free) attrition rules.",
    "assumptions": [
        "Heartland profile active.",
        "Both gold sites worked daily (+240).",
        "Barracks built day1, L2 by day8 (=> +2 pool/week from day8).",
        "Quest/siege rewards: +250 on days 6,13,20,27 and +600 on days 10,24 (sustained offensives).",
        "Recruits to the full available pool every growth week to replace losses.",
        "Note: current snapshot has NO army/building/siege gold upkeep, so war is cheaper than its design intent; see snowball_controls.md.",
    ],
    "income": BASE_INCOME,
    "site_gold_per_day": WINDMILL_GOLD + QUARRY_GOLD,
    "market_l2_day": None,
    "barracks_l2_day": 8,
    "build_plan": {1: ["human.barracks"]},
    "recruit_plan": {1: 8, 8: 6, 15: 6, 22: 6, 29: 6},
    "one_time_rewards": {6: 250, 13: 250, 20: 250, 27: 250, 10: 600, 24: 600},
    "hero_hire_day": None,
}

profiles = [conservative, expansionist, ai_baseline, war_heavy]
results = {
    "schema": 1,
    "generated_for": "Evidence/KiroParallel-20261010/09_ECONOMY_PACING_AND_BALANCE",
    "frozen_source_sha": "2c3a9055d71a87ed3ed8b8abf08503efa1218f7c",
    "model_type": "deterministic (no RNG); applies only VERIFIED FROM CURRENT SNAPSHOT runtime rules",
    "horizon_days": DAYS,
    "verified_constants": {
        "start_gold": START_GOLD, "base_income_per_day": BASE_INCOME, "ap_per_day": AP_PER_DAY,
        "knight_cost": KNIGHT_COST, "knight_pool_start": KNIGHT_START_AVAIL,
        "knight_weekly_growth": KNIGHT_WEEKLY_GROWTH, "knight_cap": KNIGHT_CAP,
        "archer_cost": ARCHER_COST, "guard_cost": GUARD_COST,
        "windmill_gold": WINDMILL_GOLD, "quarry_gold": QUARRY_GOLD,
        "market_l2_daily": MARKET_L2_DAILY, "barracks_l2_weekly_pool": BARRACKS_L2_WEEKLY_POOL,
        "quest_capture_reward": QUEST_CAPTURE_REWARD, "siege_capture_reward": SIEGE_CAPTURE_REWARD,
        "tavern_hero_cost": TAVERN_HERO_COST, "troop_hard_cap": TROOP_HARD_CAP,
        "building_costs_days": BUILDINGS,
        "weekly_growth_days": [d for d in range(1, DAYS + 1) if is_growth_day(d)],
        "building_upkeep": 0, "army_upkeep": 0, "siege_gold_cost": 0,
        "diplomacy_gift_cost": 0, "ransom_system": "none",
    },
    "models": [simulate(p) for p in profiles],
}

out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "30_day_simulation.json")
with open(out, "w") as f:
    json.dump(results, f, indent=2)
print("wrote", out)
for m in results["models"]:
    print(f"  {m['profile']:>22}: end_gold={m['end_gold']:>7}  end_troops={m['end_troops']:>3}  recruited={m['total_knights_recruited']}")
