# Soul — Snowball Controls & Pacing Levers (Lane 9)

**Frozen source:** `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c` (verified).
This document isolates the mechanisms that let an advantage compound unchecked, and proposes the smallest set of deterministic controls to flatten the runaway curve the 30-day models expose. Exact numbers are in `recommended_costs_and_income.csv`.

Labels: **[VERIFIED FROM CURRENT SNAPSHOT]** = current behavior; **[DESIGN PROPOSAL]** = my recommendation.

---

## 1. Why it snowballs today (verified)

**[VERIFIED FROM CURRENT SNAPSHOT]** Four reinforcing loops, none of them damped:

1. **Flat overflowing income.** 450 gold/day with a total build tree of only 2,900 gold and unit cost 140 means gold is never scarce. The treasury only grows (conservative model: 3,000 → 19,520 by Day 30).
2. **Combat pays, never costs.** Capture rewards (+250 quest, +600 siege) add gold; there is **no army upkeep, no siege cost, no repair cost**. Winning strictly increases the surplus; losing a siege costs the attacker nothing in gold.
3. **No rival pressure.** AI factions have **zero income and no recruitment pools** and the **strategic AI is OFF** — they cannot spend, rebuild, or counter-expand. The leader races an inert field.
4. **No decay or catch-up.** Nothing reduces a lead over time: no upkeep drag, no diminishing site returns, no replacement delay, no concurrency cap on construction ("unbounded_by_rule").

The only thing that is scarce is the **recruitment pool** (cap 24, +4/week). That paces *army size* but does nothing to the treasury or to the AI's inability to compete.

---

## 2. Control levers, ranked by leverage

### Tier 1 — Fixes that change the game's shape
**[DESIGN PROPOSAL]**
- **A. Give the AI a real economy (highest leverage).** Seed AI `DailyIncome` (match the player's base) and at least one recruitment pool per faction. Without this, no other balance change matters because there is no opponent. *(Also the headline Astra/Codex action in `HANDOFF.md`.)*
- **B. Replace flat income with sub-linear territory-scaled income.** Lower the base (≈180/day) and add a per-owned-region increment (≈30/region) that is soft-capped. Expansion still pays, but the curve flattens instead of exploding; captured regions gain lasting value instead of one-time rewards.

### Tier 2 — Running drains that make scale cost something
**[DESIGN PROPOSAL]**
- **C. Light army upkeep** (≈2 gold/unit/day). A 24-unit army ≈ 48/day against ≈180 base income — a felt but non-crippling drain that makes a large standing army a real commitment and gives war a continuous cost.
- **D. Building upkeep** (≈5/building/day) + **paid, time-gated repair** (≈40% of replacement value, 3-5 days; lab range 0.25-0.60 / 3-7). Over-development now has an operating cost and damage has consequences.
- **E. Siege-attempt cost + attacker supply drain** (one-time ~150 gold + ~50 permille/day supply; coordinate mechanics with Lane 7). Failed sieges must hurt.

### Tier 3 — Caps and tapers that remove free compounding
**[DESIGN PROPOSAL]**
- **F. Defeated-army replacement delay** (≈3 days; lab range 2-4). A wiped army cannot instantly re-field; this is the classic anti-snowball brake and the lab calls replacement semantics "first-order balance authority."
- **G. Daily gold-site claim cap / diminishing returns.** Limit gold-site income to one claim/day (or decay repeat claims) so sites are a steady supplement, not a firehose; also reduce per-use values (windmill 100→60, quarry 140→90).
- **H. Concurrent major-construction cap of 2** (lab range 1-2) so the 1/2/3-day build-time distinction stays meaningful as territory grows.
- **I. Scale down one-time capture rewards** (quest 250→150, siege 600→350) so a capture funds a couple of replacements, not a fresh company.

---

## 3. Expected effect on the 30-day curve

**[DESIGN PROPOSAL]** With Tier 1+2 applied, the conservative player's Day-30 treasury should land roughly flat-to-modestly-positive (hundreds, not ~19k), because base 180/day + modest site/region income is largely consumed by upkeep and recruitment. The war-heavy profile should go from *richest* to *tightest* (upkeep + siege cost + smaller rewards), correctly making sustained war the most expensive playstyle. Crucially, with lever A the AI's treasury and army stop being frozen, so the player no longer races an empty board.

These are directional expectations from the model's structure; exact re-runs require the recommended constants to be wired at runtime and re-simulated (the deterministic model in `build_30_day_simulation.py` can be re-parameterized once values are chosen).

---

## 4. Guardrails (do not do these)
**[DESIGN PROPOSAL]**
- Do **not** raise unit costs or pool growth to "fix" the economy — the problem is income abundance and an inert AI, not unit price. Changing prices without fixing income/AI just slows the player while the AI still can't compete.
- Do **not** use travel/movement cost to hide the runaway (the lab explicitly warns: "do not use travel cost to hide map-layout problems").
- Do **not** introduce a second economy/treasury authority — all changes must live on `FSoulCampaignEconomy`/`FSoulCampaignRules` and the Heartland seeding path.
- Do **not** preserve annihilated-regiment veterancy by default when adding replacement delay (lab note).

---

## 5. Coordination / dependencies
- **Lane 2 (AI army composition/recruitment):** owns *how* the AI spends; this lane supplies *that* the AI must have income + pools (lever A). These must land together.
- **Lane 4 (prisoners/ransom/diplomacy V1):** ransom and gift costs are economy sinks/sources this lane does not implement; expose cost hooks on `FSoulCampaignEconomy` for them.
- **Lane 7 (Siege V1):** owns siege mechanics; this lane supplies the siege-attempt gold cost and attacker-supply-drain economy knobs (lever E).
- **Lane 8 (strategic events/sites):** new sites must use the capped/tapered income model (lever G), not the current flat +100/+140.
- **Lane 1 (campaign victory):** an economic-control victory path depends on territory-scaled income (lever B) being the source of economic lead.
