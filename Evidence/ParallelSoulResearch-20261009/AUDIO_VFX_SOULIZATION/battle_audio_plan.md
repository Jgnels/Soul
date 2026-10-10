# Battle + Magic + UI Audio Plan v0 — AUDIO_VFX_SOULIZATION

Cue plan for Soul's tactical battles, magic schools, and UI. Soul today has only "a tiny dedicated battle-audio set" (user) — this batch gives it a full melee/ranged/siege/magic/UI spine from owned assets. All picks are grounded in `Copperlight-Asset-Catalog/catalog/media/audio_clips.json`; full paths and variation lists are in `audio_integration_v0.json`.

Combat/siege events are grounded in `Soul/Data/battlefield_recipes.json` and `city_siege_blueprints.json`. Architecture rule (`AGENTS.md`): canonical combat resolution is deterministic and presentation-independent — these cues *play* resolved outcomes, they never decide them. Bind cues to resolved events with Soul's stable content IDs and explicit RNG streams for variation selection.

Legend: **loop/one-shot**, variation count = distinct owned clips available, P0/P1/P2 priority.

---

## BATTLE

### Melee — the backbone (TomMusic, CC-clean, high confidence)
| Event | Cue | Clips (ids) | Var | Loop | Prio |
|---|---|---|---|---|---|
| Melee swing | BAT_SWORD_SWING | Sword Attack 1–3 (1449–1451) | 3 | one-shot | P0 |
| Impact (hit lands) | BAT_SWORD_IMPACT | Sword Impact Hit 1–3 (1455–1457) | 3 | one-shot | P0 |
| Block | BAT_SWORD_BLOCK | Sword Blocked 1–3 (1452–1454) | 3 | one-shot | P0 |
| Parry / retaliation ping | BAT_SWORD_PARRY | Sword Parry 1–3 (1458–1460) | 3 | one-shot | P1 |

Rotate variations with a dedicated RNG stream + light pitch randomization so repeated hits in the initiative timeline don't machine-gun the same sample. Parry maps naturally to Soul's **retaliation** mechanic.

### Ranged
| Event | Cue | Clips (ids) | Var | Loop | Prio |
|---|---|---|---|---|---|
| Arrow release | BAT_BOW_RELEASE | Bow Attack 1–2 (1439–1440) | 2 | one-shot | P0 |
| Arrow impact | BAT_BOW_IMPACT | Bow Impact Hit 1–3 (1444–1446) | 3 | one-shot | P1 |

Binds to `human.archery_range` / `viking.hunter_range` ranged unit families. Bow Blocked 1–3 (1441–1443) available if shields vs arrows need a distinct block.

### Deaths / grunts (Human Elements, owned)
| Event | Cue | Clips (ids) | Var | Loop | Prio |
|---|---|---|---|---|---|
| Hit reaction / death | BAT_DEATH_GRUNT | pain/grunt set 2267, 2063, 2064, 2224, 2225, 2266 | 6 | one-shot | P0 |
| Monstrous death | (variant) | Troll Scream (2232); ESM Humanoid Creatures Vol4 (3562) | 2 | one-shot | P1 |

Trim long takes to single reactions. Keep per-family tone: human grunts for human/viking/dwarf, troll/creature screams for orcs/dark/beasts. These are owned (Human Elements pack) — no purchase.

### Siege impacts (city_siege_blueprints damage events)
| Event | Cue | Clips (ids) | Var | Loop | Prio |
|---|---|---|---|---|---|
| Stone wall/gate hit + collapse | BAT_SIEGE_IMPACT_DEBRIS | Explosion With Debris 07 (1375), Blast Debris Large 01 (1285) | 2 | one-shot | P0 |
| Timber/palisade break | BAT_SIEGE_WOOD_BREAK | Impact Wood Breaks And Falls 01 (2938) | 1 | one-shot | P1 |

Bind to siege objectives (`gatehouse`, walls, `mountain_gate`, `bridge_watch`, `elephant_gate`) and `damage_strategy` state transitions (intact→damaged→ruined). Low-pass the blast transient so it reads as masonry, not modern ordnance. Pair with VFX_SMOKE_SIEGE_DEBRIS + VFX_SPARKS_IMPACT.

### War signals — PROVEN GAPS (do not fabricate)
| Event | Cue | Status | Prio |
|---|---|---|---|
| War drums (orc.war_drum_tower morale) | BAT_WAR_DRUMS | **GAP** — 0 "drum" clips in 4383 records | P1 |
| War / signal horn (charge, siege signal) | BAT_WAR_HORN | **GAP** — only modern cinematic braams (3653/3654) + boat horns; no period war horn | P1 |
| Cavalry hoofbeats / charge ("if available") | BAT_CAVALRY | **GAP** — no horse-gallop set in Animals pack | P2 |

These three are the only battle items the owned library cannot satisfy. **Purchase justified** for a war-drum/tribal-percussion set and a war-horn (lur/carnyx) set — both are faction-defining (orc drums, viking/orc horns) and genuinely absent. Cavalry only matters if a mounted unit family ships. Until then: placeholder drums = pitched/filtered low Impact hits; do NOT ship the cinematic braam as the primary charge horn (it may serve only as a distant pre-battle swell).

---

## MAGIC
All magic one-shots are TomMusic (`SFX\Spells`), CC-clean, high confidence unless noted. Each binds to a spell school and (where owned) a VFX system in `vfx_integration_v0.json`.

| School | Cue | Clips (ids) | Var | VFX pairing | Prio |
|---|---|---|---|---|---|
| Fire | MAG_FIRE_CAST | Fireball 1–3 (1582–1584), Firespray 1–2 (1587–1588) | 5 | VFX_FIRE_SPELL (M5) | P0 |
| Frost | MAG_FROST_CAST | Ice Barrage/Freeze/Throw/Wall (1589–1596) | 6 | special-ability burst (tinted) | P0 |
| Earth | MAG_EARTH_CAST | Rock Meteor/Throw/Wall (1597–1602) | 6 | special-ability burst (tinted) | P1 |
| Water | MAG_WATER_CAST | Waterspray 1–2 (1606–1607), Wave Attack 1–2 (1608–1609) | 4 | special-ability burst (tinted) | P1 |
| Air | MAG_AIR_CAST | wind bed slice (80), repurposed | 1 | smoke/fog or wind particles | P2 |
| Dark/Chaos | MAG_DARK_CHAOS | Haunting Ambiences bed (3465) + ritual chanting set | — | VFX_SMOKE_FOG_ENV | P1 |
| Buffs | MAG_FIRE_BUFF | Firebuff 1–2 (1585–1586) | 2 | fire candle/aura | P1 |
| Impacts (generic) | MAG_SPELL_IMPACT | Spell Impact 3 (1605) [1603/1604 are dup of torch] | 1 | spark/smoke | P0 |

Notes:
- **Fire is the only fully-owned school** for both audio and VFX. Frost/Water/Earth have owned *audio* but no dedicated owned *VFX* (visual-only gap — see `vfx_integration_v0.json` `spell_school_vfx_coverage`). Air and Dark/Chaos are **audio gaps/partials**: Air has no dedicated cast one-shot (repurpose wind); Dark/Chaos is built from owned haunting beds + the 46 ritual-chanting files rather than a single cast clip.
- **Duplicate warning:** import `Spell Impact 3` (1605) as the generic magic-impact; do NOT import both `Spell Impact 1/2` and `Torch Impact 1/2` — they are byte-identical pairs (AUDIO_CAPABILITY_GAPS.md).
- Ice Wall / Rock Wall map to summoned-terrain spells and tie into layered-siege/battlefield terrain prep.

---

## UI
Kenney Interface Sounds (`kenney_interface-sounds\Audio`, pack "Audio") are **CC0** — the most cost-effective UI win. High confidence throughout.

| Event | Cue | Clip (id) | Var | Prio |
|---|---|---|---|---|
| Selection | UI_SELECT | select_006 (3074) + select_00x family | ~8 | P0 |
| Order confirmation | UI_ORDER_CONFIRM | confirmation_001 (3005) +002–004 | 4 | P0 |
| Build complete (HUD) | UI_BUILD_COMPLETE | bong_001 (2995) | 1 | P1 |
| Recruit | UI_RECRUIT | confirmation_002 (3006) | 1 | P1 |
| Save / load | UI_SAVE_LOAD | confirmation_003 (3007) | 1 | P2 |
| Invalid action / error | UI_ERROR | error_001 (3013) +002–008 | 8 | P1 |
| Diplomacy | UI_DIPLOMACY_LATER | **deferred** (user: "diplomacy later") | 0 | P2 |

Notes:
- Audition shorter `select_00x` siblings for the high-frequency selection cue (006 is ~1.9s — too long for rapid clicks).
- `UI_BUILD_COMPLETE` (HUD chime) pairs with the world-side `SET_CONSTRUCTION_COMPLETE` thunk (Gate Close, id 1482) for a two-part completion event.
- Save/load audio is presentation only; **RBSave** owns save logic — do not couple.
- Diplomacy intentionally unscoped; the Kenney switch/glass/pluck families can cover it later at zero acquisition cost.

---

## Integration guidance for Codex (summary)
1. **No imports done here.** This is the shortlist + bindings. Codex performs the actual import into Soul's content.
2. Variation sets are pre-grouped (e.g. Sword Attack 1–3) — import as a single randomizing Sound Cue per event, driven by a Soul RNG stream for determinism/replayability.
3. Respect loop vs one-shot as specified; beds loop seamlessly (TomMusic BGS and 90s Fire loops are built for it).
4. De-duplicate the TomMusic Spell/Torch Impact pair.
5. Treat the three battle gaps (drums, war horn, cavalry) and the Air/Dark cast gaps as flagged — placeholder or acquire, do not fabricate sources or `/Game` paths.
