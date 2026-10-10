# Regional Soundscapes v0 — AUDIO_VFX_SOULIZATION

Layered campaign-map soundscapes for Soul's six factions/regions, built entirely from the owned/local library. Every clip cites a Copperlight `audio_clips.json` id, its `relative_path` under the `D:\SFX` root (physical files NOT in repo), its source pack, loop/one-shot role, and a confidence/licensing note. These are shortlists, not full inventories.

Design principles (grounded in `Soul/AGENTS.md`):
- **Layered beds, not single tracks.** Each region = one or two seamless ambience loops + sparse positional one-shots (fire, water, crafting, crowd) so the map feels alive cheaply.
- **Presentation only.** Soundscapes present canonical state (region, weather via RBWeather, settlement damage); they do not drive it.
- **State-aware.** Where the library offers day/night and clear/storm siblings (TomMusic BGS loops), swap beds with campaign time/weather rather than crossfading unrelated clips.
- **Favor owned clean provenance.** TomMusic (fantasy, clean) and Kenney/OpenGameArt CC0 are preferred; Sonniss/Epic Stock Media bundle clips are used where the fantasy pack has no equivalent (keep bundle attribution).

Faction/region ids and settlement structures are from `Soul/Data/settlement_blueprints.json` and `city_siege_blueprints.json`. The six regions map to the six factions (five enabled + the disabled `nature_candidate`).

Loop vs one-shot and variation counts, plus full paths, are machine-readable in `audio_integration_v0.json`.

---

## 1. Human Heartland — `humans`
Concept (settlement_blueprints): "Ordered stone capital"; town view base `Medieval_Megapack/Levels/PL_Fortress_Day.umap`.

| Layer | Role | Clip (id) | Source pack | Confidence |
|---|---|---|---|---|
| Base bed | loop | Forest Day (1426) | TomMusic | high |
| Night state | loop | Forest Night (1429) | TomMusic | high |
| Market murmur | loop | ESM Crowd Walla distant (3567) + siblings 3570/3568 | Epic Stock Media (bundle) | medium |
| Tavern hearth | loop | Fire Fireplace Pops Crackle 01 (1382) | Fire and Explosions | high |
| Forge working | loop+one-shot | Fire Roar Blaze Bonfire 01 (1388) + blacksmithhammer (897) | Fire and Explosions / OpenGameArt_CC0 | high |

Build: temperate forest/field bed under the whole heartland; attach the walla + tavern hearth at the market/tavern prefabs (`/Game/Medieval_Megapack/Levels/Prefabs/Tavern`, `BP_MarketStand`), forge fire+anvil at `/Game/Medieval_Megapack/Levels/Prefabs/Forge`. No dedicated preindustrial market ambience exists (gap report) — the walla+hearth+anvil stack IS the procedural market/town bed.

---

## 2. Viking Coast — `vikings`
Concept: "Harbour settlement from docks to great hall"; town view `Water_City/Levels/LV_WaterVillage.umap`.

| Layer | Role | Clip (id) | Source pack | Confidence |
|---|---|---|---|---|
| Base bed | loop | Sea (1438) | TomMusic | high |
| Realism top layer | loop | Rocky Coast of Norway (3663) | Just Sound Effects (Sonniss bundle) | medium |
| Storm state | loop | Sea Storm (1437) | TomMusic | high |
| Harbour market | loop | ESM Crowd Walla (3567) | Epic Stock Media (bundle) | medium |
| Smithy | loop+one-shot | Fire Roar Blaze Bonfire 01 (1388) + blacksmithhammer (897) | Fire and Explosions / CC0 | high |

Build: fantasy Sea loop as the always-on coastal bed; optionally cross-fade the Norway rocky-coast field recording for a harsher Nordic read. Swap to Sea Storm with weather. Attach smithy/market one-shots to `viking.smithy` / `viking.market` house BPs. Shipyard/docks have no dedicated owned creak set — compose from Portcullis/Gate (1485/1482) wood groans if a dock-creak is wanted later (flag, not a purchase).

---

## 3. Dwarf Forge / Mountains — `dwarves`
Concept: "Vertical mountain forge-city"; dynamic layers include smoke/lava (`city_siege_blueprints`).

| Layer | Role | Clip (id) | Source pack | Confidence |
|---|---|---|---|---|
| Interior bed | loop | Cave (1423) | TomMusic | high |
| Exterior wind | loop | 99Sounds Wind (80) | 99Sounds (free bundle) | medium |
| Great Forge furnace | loop | Fire Roar Blaze Bonfire 01 (1388) | Fire and Explosions | high |
| Forge work | one-shot | blacksmithhammer (897) + hammer sequence (3733) | OpenGameArt_CC0 / BigSoundBank_CC0 | high |
| Deep Mine | one-shot (seq) | mine 1–5 (1469–1473) | TomMusic | high |

Build: Cave loop drives the enclosed vertical hold; wind bed on the exterior mountain approach. The forge district stacks furnace roar + anvil + hammer sequence; `dwarf.mine` runs a randomized mine-pick loop. This is the richest crafting region from owned assets alone.

---

## 4. Orc Badlands — `orcs`
Concept: "War camp in a ruined city"; battlefield `orc.badlands` = `LandscapePackTwo/Maps/Mesa_01.umap`.

| Layer | Role | Clip (id) | Source pack | Confidence |
|---|---|---|---|---|
| Base bed | loop | 99Sounds Wind (80) | 99Sounds (free bundle) | medium |
| Camp fires | loop | Fire Roar Blaze Bonfire 01 (1388) | Fire and Explosions | high |
| Camp crowd | loop | ESM Crowd Walla distant (3567) | Epic Stock Media (bundle) | medium |
| War drums | loop | **GAP — none owned** | — | gap |

Build: dry wind bed over the mesa/ruin camp; fires at war-camp hearths; low distant walla for the occupying horde. **The `orc.war_drum_tower` civic structure (morale bonus) has NO owned war-drum audio** — a search for "drum" over all 4383 clips returns zero. This is the clearest region-defining gap: see `battle_audio_plan.md` and the purchase flag in `audio_integration_v0.json` (BAT_WAR_DRUMS). Placeholder = pitched low Impact hits, temporary.

---

## 5. Nature Forest — `nature_candidate` (DISABLED)
Concept: "Forest/tree settlement"; faction `enabled:false` in `settlement_blueprints.json`.

| Layer | Role | Clip (id) | Source pack | Confidence |
|---|---|---|---|---|
| Day bed | loop | Forest Day (1426) | TomMusic | high |
| Night bed | loop | Forest Night (1429) | TomMusic | high |
| River/spring | loop | River Loop (1619) / Waterfall Loop (1621) | TomMusic | high |
| Woodwork | one-shot | chop 1–4 (1465–1468) | TomMusic | high |

Build: forest day/night beds + positional river at `nature.healing_spring` / river battlefields; chop one-shots for the timber settlement. **Flag:** this faction is a *candidate* (`enabled:false`, roster/map pending). Audio is ready but Codex should gate integration behind the faction being enabled — do not wire it into shipping content until the faction is turned on.

---

## 6. Dark Region — `dark`
Concept: "Otherworldly dark-fantasy fortress; remove technological reads"; battlefields `dark.ash_plain`, `dark.corrupted_valley`.

| Layer | Role | Clip (id) | Source pack | Confidence |
|---|---|---|---|---|
| Base bed | loop | Forest Night (1429) low/filtered | TomMusic | high |
| Dread atmosphere | loop | 344 Audio Haunting Ambiences Vol.5 (3465) | 344 Audio (Sonniss bundle) | medium |
| Wind desolation | loop | 99Sounds Wind (80) | 99Sounds | medium |
| Ritual/occult | loop | ritual chanting set (46 files, see gap report) | mixed | medium |

Build: a quiet filtered forest-night or wind base with the haunting-ambience bed layered for dread; add ritual chanting around `dark.befouler_sanctum` / `dark.ward_spire`. Occult coverage is GREEN-YELLOW (gap report: 46 chanting files + eerie loops), so the dark region is well-served without purchases; the remaining weakness is fine-grained "witch-house" micro-foley (alchemy props, whispers) — not needed for a v0 region bed.

---

## Cross-region notes
- **Weather beds** (rain/storm/snow/wind as *weather*) are owned by **RBWeather** (`Plugins/RBWeather/Content/Presentation/Audio`: `S_RB_RainLoop`, `S_RB_SnowLoop`, `S_RB_BlizzardLoop`, `S_RB_Thunder`, etc.). Region soundscapes must layer *under* RBWeather, not duplicate it (AGENTS.md: no second authority for Weather). The TomMusic `* Storm`/`* Rain` BGS siblings are region-flavored beds, not a weather system.
- **Duplicate caution:** TomMusic `Spell Impact 1/2` == `Torch Impact 1/2` (ids 1603/1604 == 1616/1617) — relevant if torch loops and spell impacts are both pulled into a region.
- **One region bed can serve two factions** when thematically adjacent (Forest Day serves both Human Heartland and Nature Forest); de-duplicate imports by sharing a single Sound Cue with per-region modulation.
