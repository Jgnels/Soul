# HANDOFF — Lane 9: Campaign Economy, Costs, Pacing & 30-Day Balance Model

**Lane:** `09_ECONOMY_PACING_AND_BALANCE`
**Frozen source:** `Jgnels/Soul` @ `handoff/soul-kiro-20261010` = `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c` (**verified exact**).
**Work branch:** `kiro/economy-pacing-balance-20261010` (created from that exact commit).
**Write scope (respected):** only `Evidence/KiroParallel-20261010/09_ECONOMY_PACING_AND_BALANCE/`. No runtime `Source/`, `Config/`, `Content/`, `.uproject`, maps or saves were edited.

Label legend used throughout: **[VERIFIED FROM CURRENT SNAPSHOT]**, **[VERIFIED OWNED ASSET METADATA]**, **[LOCAL RUNTIME/ASSET CHECK REQUIRED]**, **[DESIGN PROPOSAL]**.

---

## Artifacts in this directory
| File | Purpose |
|---|---|
| `current_economy_audit.csv` | Every exact current economy value with file:line symbol references and labels. **[VERIFIED FROM CURRENT SNAPSHOT]** |
| `30_day_simulation.json` | Deterministic 30-day models for conservative / expansionist / AI-baseline / war-heavy, with per-day rows and the verified constant block. |
| `build_30_day_simulation.py` | RNG-free generator that produces `30_day_simulation.json` from the verified rules. Re-run to re-parameterize. |
| `balance_findings.md` | Full analysis: model results, expansion speed, scarcity, site value, snowball, AI sustainability, siege-loss cost, build-time relevance, unknowns. |
| `recommended_costs_and_income.csv` | Exact **current vs recommended** values, side by side, each labeled. |
| `snowball_controls.md` | Ranked anti-snowball levers, expected curve effect, guardrails, cross-lane dependencies. |
| `HANDOFF.md` | This file. |

---

## Exact current values (quick reference) — **[VERIFIED FROM CURRENT SNAPSHOT]**
- Start gold **3000**, daily income **450/day** (`SoulFounderPlaytestStateSubsystem.cpp:290`).
- Knight **140**; pool 8 / +4 week / cap 24 (`:291-292`). Archer **180**, Guard **220** (Heartland, `:307`). Troop cap **250** (`:116`).
- Buildings (`Data/SettlementEnvironments/HeartlandDevelopment.json`): tavern 200/1d, arcane_hall 300/1d, mage_academy 600/2d, high_conclave 900/3d, market 400/2d (L2), barracks 500/2d (L2). Cost deducted once atomically (`SoulTown.cpp` `BeginConstruction`).
- Sites (own+occupy+1AP, 1/day): windmill **+100g**, quarry **+140g**, shrine **+20 mana**.
- Market L2 **+100g/day** (`:650`); Barracks L2 **+2 knights/week** (`:653`). Quest capture **+250g** (`:504`); siege capture **+600g** (`:617`); hero hire **-1200g** (`:670`).
- Economy engine: `FSoulCampaignEconomy` / `FSoulCampaignRules::AdvanceDay|CanAfford|Recruit|SpendAction` (`Source/SoulCore/{Public,Private}/SoulCampaign.{h,cpp}`); weekly growth on days where `(Day-1)%7==0`.
- **Absent:** building upkeep, army upkeep, siege gold cost, siege attacker supply/readiness cost (0), diplomacy/gift cost (0), ransom (none), hero equipment/maintenance cost (0), repair cost (free), construction concurrency cap (unbounded).
- **AI economy:** gold = player start (**3000**) but **DailyIncome empty, NO recruitment pools, strategic AI OFF** (`SoulSixFactionCampaign.cpp:75`; `LastAIReport` "strategic AI OFF").

## Key findings — **[VERIFIED FROM CURRENT SNAPSHOT]**
1. **Treasury runs away.** Even a conservative player ends 30 days at **19,520 gold** (~6.5x start). Gold is never a binding constraint; **recruitment is pool-limited, not gold-limited.**
2. **AI is economically inert** — frozen at **3,000**, zero income, no pools, AI off. Multi-day campaigns are effectively solo.
3. **War is free / pays.** No upkeep + capture rewards + no siege cost make the war-heavy profile end *richest* (**20,920**). Combat should cost, not profit.
4. **Build-time stops mattering after week 1.** Whole 6-building tree = 2,900g < one week of income; the 1/2/3-day gate is only early-game.
5. **No damping of any kind** — no upkeep, decay, replacement delay, concurrency cap, or rival pressure. Every advantage compounds.

## Implementation order (for Codex) — **[DESIGN PROPOSAL]**
Smallest clean authority is `FSoulCampaignEconomy` + `FSoulCampaignRules` (`SoulCore`) and the Heartland seeding path in `SoulFounderPlaytestStateSubsystem.cpp` / `SoulSixFactionCampaign.cpp`. Do **not** create a second economy authority.
1. **AI income + pools** — in `SoulSixFactionCampaign.cpp::InitializeSixFactionState` seed each AI `FSoulFactionCampaignState.Economy.DailyIncome["gold"]` and at least one `RecruitmentPool` (coordinate the spend policy with Lane 2). *(Highest value — see below.)*
2. **Territory-scaled sub-linear income** — replace the flat `DailyIncome["gold"]=450` seed with base + per-owned-region increment, soft-capped (compute in `AdvanceDay` or at seed time from owned-region count).
3. **Army upkeep** — deduct per-unit gold in `AdvanceDay` (new drain on `Resources`), tuned ~2/unit/day.
4. **Building upkeep + paid/time-gated repair** — per-building daily cost; repair cost fraction 0.40 and 3-5 days (lab ranges).
5. **Siege-attempt cost + attacker supply drain** — economy knob paired with Lane 7 mechanics.
6. **Defeated-army replacement delay (~3 days)** and **construction concurrency cap (2)**.
7. **Taper sites/rewards** — windmill 100→60, quarry 140→90, daily site-claim cap; quest 250→150, siege 600→350.
Exact numbers: `recommended_costs_and_income.csv`.

## Acceptance / tests — **[DESIGN PROPOSAL]**
- Re-run `build_30_day_simulation.py` with chosen constants; conservative Day-30 treasury should be roughly flat (hundreds, not ~19k) and war-heavy should be the tightest, not the richest.
- With AI income+pools, an AI faction that loses an army can re-field within the replacement delay and its treasury changes over 30 days (no longer frozen at 3,000).
- No new economy authority introduced; all deductions/credits route through `FSoulCampaignRules`.
- Unit prices (140/180/220), pool growth (+4/wk), cap (24) and troop cap (250) unchanged.

## Conflicts / dependencies
- **Lane 2** owns AI spend policy; this lane requires AI income+pools exist (land together).
- **Lane 4** owns ransom/gift; expose cost hooks on `FSoulCampaignEconomy`, do not implement here.
- **Lane 7** owns siege mechanics; this lane supplies siege economy knobs only.
- **Lane 8** new sites must use capped/tapered income, not flat +100/+140.
- **Lane 1** economic-victory path depends on territory-scaled income being the lead source.

## Explicit unknowns — **[LOCAL RUNTIME/ASSET CHECK REQUIRED]**
- Runtime behavior of "unbounded" construction slots; any UI/soft limit.
- Whether non-Heartland profiles seed income/pools differently than the audited founder path.
- Dwarf/Orc/Viking unit costs — none exist in data; must be authored before their economies can be modeled.
- Confirm +250/+600 reward hooks fire at most once per region under re-capture churn in a long campaign.

---

## Highest-value Astra/Codex integration action
**Give AI factions a functioning economy: seed non-zero `DailyIncome` and at least one `RecruitmentPool` per AI faction in `SoulSixFactionCampaign.cpp::InitializeSixFactionState`, and turn on an economic spend policy (with Lane 2).** **[DESIGN PROPOSAL, grounded in VERIFIED FROM CURRENT SNAPSHOT fact]** The AI is currently economically inert (3,000-gold lump, zero income, no pools, strategic AI OFF), so every multi-day campaign is a solo runaway for the player. This single change is the precondition for every other balance lever to matter — no economy fix is observable while the only opponent cannot earn, spend, or rebuild.

**Branch:** `kiro/economy-pacing-balance-20261010` • **Source SHA:** `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c` • **HANDOFF:** `Evidence/KiroParallel-20261010/09_ECONOMY_PACING_AND_BALANCE/HANDOFF.md`
