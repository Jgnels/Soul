# Starting Doctrines — Faction / Start-Doctrine Replayability

Lane: `01_CAMPAIGN_VICTORY_AND_OBJECTIVES`
Source snapshot: `Jgnels/Soul` @ `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c`
Legend: **[VCS]** Verified from Current Snapshot · **[VOA]** Verified Owned Asset Metadata · **[LCR]** Local Runtime/Asset Check Required · **[DP]** Design Proposal

---

## What a doctrine is (and is NOT)

**[DP]** A *starting doctrine* is a thin, deterministic **overlay** on the campaign start state that biases a faction toward a particular victory approach without changing geography, factions, or shipped mechanics. A doctrine adjusts only values that are already authored or already serialized:

- starting army composition and secondary garrison profile (`starting_army` / `secondary_garrison` in `soul_campaign_start_states_v1_20260922.json`);
- starting recruitment pool `Available` / `WeeklyGrowth` (`FSoulRecruitmentPool`);
- starting treasury `Economy.Resources['gold']` / `DailyIncome`;
- one or two pre-constructed civic buildings (from the faction's `civic_structures` in `Data/settlement_blueprints.json`);
- which victory approaches start **enabled/highlighted** (the `ActiveApproachMask` default) and which objective track is suggested first.

**[VCS] Hard constraints a doctrine must respect:**
- It does **not** add/remove regions or change the 36-region graph (`soul_world_overmap_v1_20260922.json`).
- It does **not** edit the locked founder micro scenario or the start-state file in place; a doctrine is a *separate* overlay data file keyed by `faction_id + doctrine_id`.
- It does **not** create units/buildings that are not already in `settlement_blueprints.json` / the faction roster. **No invented IDs.**
- Army/garrison numbers are `BALANCE_LAB_OWNED` (**[VCS]** per the start-state `rules`); doctrines may only shift *within* ranges Lane 9 / Balance Lab approve. Values below are **[DP]** placeholders flagged `BALANCE_PENDING`.

**[VCS]** Each faction's `civic_structures` (seat/economy/hero/military/magic/defense) and 7 `recruit_structures` are already authored (see `Data/settlement_blueprints.json`), so doctrine pre-builds and pool biases reference only real IDs.

---

## Serialization

**[DP]** New overlay file (NOT editing locked files): `Data/soul_starting_doctrines_v0.json` (authored later by the integration lane; schema sketched in `HANDOFF.md`). The selected `doctrine_id` is captured in the `Soul.Campaign` save domain alongside `CampaignVictory` (schema 2), so a reload restores the doctrine-shaped start deterministically.

---

## Humans (seat `human_capital`, Heartland) **[VCS seat]**

Civic set **[VCS]**: `human.keep`, `human.forge`, `human.tavern`, `human.market`, `human.walls`. Arcane: `human.witch_collegium`. The Heartland is the solved baseline, so Human doctrines are the reference template.

| Doctrine | Leans toward | Start overlay (BALANCE_PENDING) | Suggested first objective |
|---|---|---|---|
| **Crown Militant** | Military Domination / Last Realm Standing | +1 starting fighter company; `human.barracks` pre-built L1; +0 gold | `obj.e2.first_blood`, `obj.m5.first_siege_prep` |
| **Mercantile League** | Economic / Site Control | `human.market` pre-built L1; +starting gold; −1 starting company | `obj.m2.take_a_site` |
| **Collegium Ascendant** | Magical Supremacy | `human.witch_collegium` pre-built L1; hero +1 known spell; +arcane income | `obj.e5.touch_the_arcane`, `obj.m3.hero_ascendant` |

Note: Human **Diplomacy V0** already exists, so Humans also act as the Alliance/Federation reference once Lane 4 lands; no Human-specific alliance doctrine is needed in V0.

## Dwarves (seat `dwarf_hold`, Crownspine) **[VCS seat]**

Civic set **[VCS]**: `dwarf.great_forge`, `dwarf.kings_hall`, `dwarf.mine`, `dwarf.caravan_hall`, `dwarf.gates`. Arcane: `dwarf.rune_forge`. Mountain forge-city — naturally economic + siege-resilient.

| Doctrine | Leans toward | Start overlay (BALANCE_PENDING) | Suggested first objective |
|---|---|---|---|
| **Deepforge Industry** | Economic / Site Control | `dwarf.mine` pre-built L1; bias crossbow/stoneguard pools; +gold income | `obj.m2.take_a_site` (holds `dwarf_high_quarry`) |
| **Hold Unbreakable** | Military Domination (defensive) | `dwarf.gates` pre-built L1; +garrison at `dwarf_forge_approach`; stronger defender posture | survive + counter-siege (Lane 7 owns Siege V1) |

**[LCR]** A walkable Dwarf Hold is **Lane 6**, not Lane 1; Dwarf doctrine text that implies visitation is gated on that lane.

## Orcs (seat `orc_camp`, Eastern Badlands) **[VCS seat]**

Civic set **[VCS]**: `orc.warchief_hall`, `orc.salvage_forge`, `orc.spoils_market`, `orc.war_drum_tower`, `orc.patched_walls`. "War camp occupying a ruined city" — aggressive, salvage economy.

| Doctrine | Leans toward | Start overlay (BALANCE_PENDING) | Suggested first objective |
|---|---|---|---|
| **Warband Rush** | Last Realm Standing | +1 starting fighter company; `orc.war_drum_tower` pre-built L1; −gold | fast `obj.m1.breakout` then seat raids |
| **Salvagers** | Economic / Site Control | `orc.spoils_market` pre-built L1; bias on captured-region income (ties to escalation unrest exemption) | hold ruined-field sites |

## Vikings (seat `viking_harbour`, Northern Fjords) **[VCS seat]**

Civic set **[VCS]**: `viking.great_hall`, `viking.shipyard`, `viking.smithy`, `viking.market`, `viking.watch`. Arcane: `viking.shaman_lodge`. Harbour raiders — mobile, coastal.

| Doctrine | Leans toward | Start overlay (BALANCE_PENDING) | Suggested first objective |
|---|---|---|---|
| **Sea Raiders** | Military Domination (tempo) | +starting mobility/readiness; `viking.shield_hall` pool bias; −wall investment | early `obj.m1.breakout` along the coast |
| **Northern Jarldom** | Alliance / Federation | `viking.great_hall` pre-built L1; +diplomatic standing seed with one neighbour | `obj.m4.open_relations` (needs Lane 4) |

**[LCR]** A walkable Viking Harbour is **Lane 6**.

---

## Doctrine ↔ approach coverage matrix **[DP]**

| Faction | Military | Last Realm | Economic | Magical | Alliance |
|---|---|---|---|---|---|
| Humans | Crown Militant | Crown Militant | Mercantile League | Collegium Ascendant | (Diplomacy V0 ref, Lane 4) |
| Dwarves | Hold Unbreakable | — | Deepforge Industry | (rune_forge available) | — |
| Orcs | Warband Rush | Warband Rush | Salvagers | (shaman_court available) | — |
| Vikings | Sea Raiders | Sea Raiders | (viking.market) | (shaman_lodge available) | Northern Jarldom |

Every playable faction can plausibly pursue ≥3 approaches; doctrines make 2 of them *cheap* at start, which is the replayability knob. Gaps (—) are deliberate: not every faction should be equally good at every approach.

---

## Unknowns / dependencies

- **[LCR]** Final starting army/garrison/pool numbers are Balance-Lab-owned; all values here are `BALANCE_PENDING`.
- **Lane 9** must confirm starting gold and per-building income before Mercantile/Deepforge/Salvagers are tunable.
- **Lane 4** must land Diplomacy V1 before Northern Jarldom / any Alliance-leaning doctrine is playable (ships LOCKED in V0).
- **Lane 6** gates any doctrine implying a walkable Dwarf Hold / Viking Harbour.
- **Lane 3** hero progression must confirm "hero +1 known spell / +1 level" start overlays are save-safe additive fields on `FSoulHeroState`.
