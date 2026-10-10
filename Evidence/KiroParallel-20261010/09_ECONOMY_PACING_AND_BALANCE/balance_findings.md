# Soul — Campaign Economy, Costs, Pacing & 30-Day Balance Findings (Lane 9)

**Frozen source:** `handoff/soul-kiro-20261010` @ `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c` (verified).
**Scope:** Audit of current costs/income/stock/build-times across recruitment, buildings, sites, diplomacy, heroes, AI and sieges; deterministic 30-day models for four play profiles.

Every statement is labeled:
- **[VERIFIED FROM CURRENT SNAPSHOT]** — read directly from runtime C++/data in the frozen tree.
- **[VERIFIED OWNED ASSET METADATA]** — from owned asset/catalog metadata (none load-bearing here).
- **[LOCAL RUNTIME/ASSET CHECK REQUIRED]** — needs a live runtime/asset check to confirm.
- **[DESIGN PROPOSAL]** — my recommendation, not current behavior.

Exact current values live in `current_economy_audit.csv`; the 30-day arithmetic lives in `30_day_simulation.json` (regenerate with `build_30_day_simulation.py`). Recommendations live in `recommended_costs_and_income.csv`.

---

## 1. The economy model as it actually exists

**[VERIFIED FROM CURRENT SNAPSHOT]** The campaign economy is a single generic ledger, `FSoulCampaignEconomy` (`Source/SoulCore/Public/SoulCampaign.h`): a `Day` counter, action points (`MaxActionPoints=3`), a `TMap<FName,int32> Resources` (the treasury — gold is just the key `"gold"`), a flat `TMap<FName,int32> DailyIncome`, and `RecruitmentPools`. The engine `FSoulCampaignRules` (`SoulCampaign.cpp`) does four things on `AdvanceDay`: increment the day, reset AP, add every `DailyIncome` entry to `Resources`, and on days where `(Day-1)%7==0` grow non-building-linked pools by `WeeklyGrowth` up to `Capacity`. Affordability and recruitment deduct `CostPerUnit` atomically.

**[VERIFIED FROM CURRENT SNAPSHOT]** Only the **player founder economy** is seeded with live numbers (3000 gold, 450/day, knight pool). The economy is intentionally minimal: **there is no building upkeep, no army upkeep, no siege gold cost, no diplomacy/gift cost, and no ransom system.** Repair is immediate and free. These are confirmed absences, not oversights in the audit.

---

## 2. Exact current values (the numbers that matter)

**[VERIFIED FROM CURRENT SNAPSHOT]**
- **Starting gold:** 3000. **Daily income:** 450/day (flat).
- **Knight recruitment:** 140 gold; pool starts at 8, +4/week, cap 24.
- **Heartland extras:** archer 180 (pool 6/+2/cap12), guard 220 (pool 4/+2/cap12).
- **Troop hard cap:** 250.
- **Building costs / build-days** (`HeartlandDevelopment.json`): tavern 200/1, arcane_hall 300/1, mage_academy 600/2, high_conclave 900/3, market 400/2 (maxLvl2), barracks 500/2 (maxLvl2). Cost is deducted **once, atomically** at construction start.
- **Strategic sites** (require ownership + a living company + 1 AP, repeatable once/day): windmill **+100 gold**, quarry **+140 gold**, shrine **+20 mana** (not gold).
- **Economy buildings at L2:** market → **+100 gold/day**; barracks → **+2 knights/week** to the pool.
- **One-time rewards:** quest region capture **+250 gold**; battle/capital capture **+600 gold**.
- **Tavern hero hire:** **-1200 gold**, one-time.

---

## 3. Thirty-day deterministic models (see `30_day_simulation.json`)

The four models apply ONLY the verified rules above; they are RNG-free and reproducible.

| Profile | End gold (Day 30) | End troops | Knights recruited | Notes |
|---|---|---|---|---|
| Conservative player | **19,520** | 21 | 12 | Banks income, 1 gold site, market+barracks, light recruiting |
| Expansionist player | **19,390** | 43 | 34 | 2 gold sites, 5 buildings, hero hire (-1200), 3 quest + 1 siege reward |
| **AI baseline (actual runtime)** | **3,000** | 9 | 0 | **Zero income, zero pools, strategic AI OFF — frozen** |
| War-heavy player | **20,920** | 41 | 32 | 2 gold sites, continuous pool-cap recruiting, 6 capture rewards |

### Headline results
1. **[VERIFIED FROM CURRENT SNAPSHOT] The player treasury runs away.** Even the *conservative* player ends 30 days with ~**6.5x** their starting gold (19,520 vs 3,000) while never running low. The base income alone (450/day × 30 = 13,500) dwarfs all available spending. Gold is never a binding constraint for any active player profile; **recruitment is pool-limited, not gold-limited** — exactly the pattern the balance report flagged ("recovery is gold/income-limited before pool-limited" was the *intended* worry; in practice at these numbers gold is a non-constraint).
2. **[VERIFIED FROM CURRENT SNAPSHOT] The AI economy is inert.** AI factions get a frozen 3,000-gold lump, **no income, no recruitment pools, and the strategic AI is OFF.** They cannot build, recruit, replace losses, or respond economically. Any multi-day campaign is economically a solo game: the player compounds while every rival stands still. This is the single largest balance gap.
3. **War is nearly free.** With no army/building/siege/upkeep cost, the war-heavy profile ends *richer* than the peaceful ones (20,920) because capture rewards (+250/+600) add income with no offsetting drain. Combat currently *pays* instead of *costs*.

---

## 4. Expansion speed, scarcity and the real bottlenecks

- **[VERIFIED FROM CURRENT SNAPSHOT] Company scarcity is the only brake.** The knight pool caps at 24 and grows +4/week (+6/week with barracks L2). Over 30 days a player can field at most pool-start 8 + growth (4 growth weeks × 4-6) ≈ 24-32 knights before the cap, far under the 250 troop cap. **The pool, not the treasury, paces army size.** Gold piles up unused.
- **[VERIFIED FROM CURRENT SNAPSHOT] Site value is high and uncapped per-day-but-cheap.** A single gold site returns 100-140 gold for 1 AP. With 3 AP/day a player can bank 240+ gold/day from two sites on top of 450 base — but since gold is already overflowing, sites currently add runaway, not meaningful choice.
- **[VERIFIED FROM CURRENT SNAPSHOT] 1/2/3-day construction stops mattering fast.** The whole 6-building Heartland tree costs 2,900 gold total — less than one week of base income. By ~Day 7 a player can afford the entire tree many times over, so build-*time* (not cost) is the only remaining gate, and only early. As territory grows, construction cost is irrelevant. **The 1/2/3-day distinction is meaningful only in the first week.**
- **[VERIFIED FROM CURRENT SNAPSHOT] Recruitment-vs-development is a false tradeoff** at current prices: the player can do *both* every week and still bank gold. There is no scarcity forcing a choice.

---

## 5. Snowballing & AI sustainability

- **[VERIFIED FROM CURRENT SNAPSHOT] Unbounded snowball.** Capture rewards (+250/+600) + free war + no upkeep + inert AI means every success strictly compounds with no decay or cost. There is no economic rubber-band, no attrition drain, and no rival pressure. Once the player wins the first battle, nothing economic can slow them.
- **[VERIFIED FROM CURRENT SNAPSHOT] AI cannot sustain anything** because it has no income and no pools; it is a static target, not an economic actor. See the "highest-value integration action" in `HANDOFF.md`.
- **[VERIFIED FROM CURRENT SNAPSHOT] Siege-loss cost is zero.** A failed siege costs no gold and (attacker-side) no supply/readiness (`siege_attacker_*_cost = 0`); only the defender bleeds supply (70/day). Losing a siege is nearly consequence-free economically.

---

## 6. What is explicitly *not* present (so Codex doesn't assume it)
**[VERIFIED FROM CURRENT SNAPSHOT]** No building upkeep; no army upkeep; no siege/attacker gold or supply cost; no diplomacy/gift gold cost; no ransom valuation; no hero equipment/maintenance cost; no AI treasury income or recruitment pools; no defeated-army replacement delay (lab marks it "undefined"); no concurrent-construction cap ("unbounded_by_rule").

**[DESIGN PROPOSAL]** headline fixes (full set and exact numbers in `recommended_costs_and_income.csv` and `snowball_controls.md`): give AI factions real income + pools; add light army upkeep OR a per-day gold sink that scales with territory; add a siege-attempt cost and a defeated-army replacement delay; taper strategic-site income or cap daily site claims; make base income scale sub-linearly with territory so the first week's build-time gating stays meaningful.

---

## 7. Unknowns / local checks required
- **[LOCAL RUNTIME/ASSET CHECK REQUIRED]** Exact in-engine behavior of the "unbounded" construction slots and whether any UI/soft limit exists at runtime.
- **[LOCAL RUNTIME/ASSET CHECK REQUIRED]** Whether any non-Heartland profile seeds income/pools differently than the player founder path audited here.
- **[LOCAL RUNTIME/ASSET CHECK REQUIRED]** Dwarf/Orc/Viking unit costs — none exist in data; must be authored before their economies can be modeled.
- **[LOCAL RUNTIME/ASSET CHECK REQUIRED]** Confirm the +250/+600 reward hooks fire at most once per region in a long campaign (code gates them with `RewardedRegions`, but re-capture churn should be checked live).
