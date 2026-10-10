# HANDOFF — AUDIO_VFX_SOULIZATION → Codex integration lane

**Lane:** AUDIO_VFX_SOULIZATION (parallel research/production-intelligence)
**Date:** 2026-10-09
**Status:** Research complete. **NO IMPORTS, NO PROJECT EDITS performed.** Deliverables are integration-ready shortlists for the Codex lane to execute.

## What I produced
Five files in `Evidence/ParallelSoulResearch-20261009/AUDIO_VFX_SOULIZATION/`:

| File | What it is |
|---|---|
| `audio_integration_v0.json` | Machine-readable. 42 curated audio cues across Campaign / Settlement / Battle / Magic / UI. Each cue: Copperlight `clip_id`, exact `relative_path`, `source_pack`, `D:\SFX` root, intended Soul event/structure-id, loop vs one_shot, variation_count (+ sibling ids), confidence, priority, caveats. |
| `vfx_integration_v0.json` | Machine-readable. 18 VFX systems enumerated from the **7** evidenced owned Fab products (`VFX_CAPABILITY.md`), including the 15 footstep sub-systems and spark/fire/smoke variants. Each: `fab_<id>`, intended event, loop/one_shot, variation_count, confidence, priority, caveats, favor-Niagara rationale. |
| `regional_soundscapes.md` | Six campaign regions (the six factions), each a layered soundscape from owned clips. |
| `battle_audio_plan.md` | Battle + Magic + UI cue plan with event bindings. |
| `HANDOFF.md` | This file. |

## How the JSON is structured (for Codex)
- Both JSON files begin with a `conventions` / `confidence_scale` / `priority_scale` block, then an array (`cues` / `systems`).
- `clip_id` is the stable Copperlight `audio_clips.json` record id — use it to re-look-up any field.
- Variation sets are pre-grouped (e.g. "Sword Attack 1–3 = ids 1449,1450,1451"); import each as **one randomizing Sound Cue** driven by a Soul RNG stream (determinism per `AGENTS.md`).
- Cues with `clip_id: null` are intentional **gaps/deferrals** (not oversights) — see below.

## Hard constraints Codex MUST respect
1. **`D:\SFX` is NOT in the repo.** Copperlight holds metadata-only mirrors; the ~19.3 GB of actual audio lives on the founder's archive laptop at `D:\SFX`. `relative_path` values are relative to that root. Codex must source the real bytes from `D:\SFX` before importing — the catalog proves ownership/provenance, not the files themselves.
2. **DO NOT MODIFY** (per user): `Source/`, `Config/`, `Content/`, `.uproject`, existing maps/assets, build targets, save files. Do not launch builds/cooks or save Unreal packages. Read-only inspection only in this lane.
3. **DO NOT MODIFY the existing RBWeather presentation layer** (`Plugins/RBWeather/Content/Presentation/Audio` and `.../VFX`). It is the *integration-pattern reference* (naming `S_RB_*`, `NS_RB_*`; Niagara-based) and the **Weather authority**. Region/battle beds layer *under* it — do not create a second Weather/wind authority (`AGENTS.md`).
4. **Do NOT invent `/Game/...` paths.** The only evidenced world anchors are in `Soul/Data/environment_asset_bindings.json` (e.g. `/Game/Medieval_Megapack/Levels/Prefabs/Forge`, `.../Tavern`, `.../Meshes/Props/BP_MarketStand`, Water_City/Viking_Village house BPs). Cues reference those where they exist; anything else is marked COMPOSITE/UNMAPPED.
5. **Do not commit/push/merge/reset/clean/stash** — the orchestrator publishes.

## Licensing / provenance (carry these forward)
- **CC0 / clean (high):** TomMusic Free Fantasy pack (fantasy combat/magic/ambience workhorse), Kenney Interface Sounds (`Audio` pack, all UI cues), OpenGameArt_CC0 (blacksmith hammer, metal steps), BigSoundBank_CC0, Freesound Flint Strike (id 104).
- **Attribution required:** `OpenGameArt_ATTRIBUTION_REQUIRED` pack; the Yle (ylearkisto) **loom** (ids 102/103) and **scythe** (id 101) recordings require **CC BY 4.0 attribution**.
- **Licensed bundle (medium):** Epic Stock Media packs and all **Sonniss GDC2026** bundle clips (crowd walla, Rocky Coast of Norway, Haunting Ambiences, cinematic horn braams). Owned via bundle; keep bundle attribution with the asset.
- Copperlight provenance is **DISCOVERED by default** — do not claim license verification beyond what Copperlight asserts.

## Duplicate warnings (avoid double-import)
- **TomMusic:** `Spell Impact 1` (1603) == `Torch Impact 1` (1616); `Spell Impact 2` (1604) == `Torch Impact 2` (1617). Byte-identical pairs. Import only one of each; prefer `Spell Impact 3` (1605, unique) as the generic magic-impact.
- (Library-wide there are 3 duplicate groups / 6 files; the third is a loom recording duplicate not in this batch.)

## Proven gaps & unmapped/composite events (flagged, NOT fabricated)
- **War drums** (`BAT_WAR_DRUMS`): 0 "drum" clips in all 4383 records, yet `orc.war_drum_tower` is a real civic structure with a morale effect. **Purchase justified** (tribal/war percussion set).
- **War / signal horn** (`BAT_WAR_HORN`): no period war horn; only modern cinematic braams + boat horns. **Purchase justified** (lur/carnyx/battle-horn set). Do not ship the braam as the primary charge signal.
- **Cavalry** (`BAT_CAVALRY`, user asked "if available"): no horse-gallop set owned. Pending a mounted unit family; cheap targeted buy only then.
- **Air magic cast** (`MAG_AIR_CAST`): no dedicated clip; repurpose a wind slice. **Dark/Chaos magic**: no single cast clip — built from owned haunting beds + 46 ritual-chanting files.
- **Spell-school VFX** frost/water/earth/air/dark: **visual-only gap** — audio owned, no owned Niagara per school; interim = the footstep product's "special ability" burst tinted per school. (Fire school is fully covered: M5 product.)
- **Windmill** (`SET_WINDMILL`): **UNMAPPED** — no windmill structure id exists in `settlement_blueprints.json` (closest economy buildings: `human.market`, `viking.market`, `dwarf.caravan_hall`) and no owned windmill audio. Compose from wind + a creak if/when a windmill structure is added. No `/Game` path invented.
- **Diplomacy UI** (`UI_DIPLOMACY_LATER`): intentionally deferred (user: "diplomacy later"). No cue bound.
- **Nature Forest region** (`nature_candidate`): faction is `enabled:false` in Soul data. Audio is ready but **gate integration behind the faction being enabled**; do not wire into shipping content yet.

## Where things attach (quick map)
- Human: Forge/Tavern/Market prefabs (evidenced `/Game` paths) + Forest Day/Night bed.
- Viking: Sea bed + smithy/market house BPs; harbour/fjord battlefields.
- Dwarf: Cave interior bed + exterior wind + furnace/anvil/mine (forge-city, Deep Mine).
- Orc: dry wind + camp fires + walla; **drums gap** at `war_drum_tower`.
- Dark: filtered night/haunting beds + ritual chanting at sanctum/ward spire.
- Nature: forest day/night + river + chop (gated on faction enable).
- Battle/Magic/UI: see `battle_audio_plan.md`; TomMusic combat+spells, Human Elements grunts, Fire&Explosions siege, Kenney CC0 UI.

## Suggested integration order (for Codex)
1. **P0 battle spine** (TomMusic melee/ranged/impact + Human Elements grunts) — biggest felt improvement over today's tiny set.
2. **P0 UI** (Kenney CC0 select/confirm/error) — cheapest, instant polish.
3. **P0 settlement life** (forge fire+anvil+sparks VFX, tavern hearth, walla) at evidenced prefab anchors.
4. **P0 region beds** (TomMusic BGS loops) under RBWeather.
5. **P0 magic Fire school** (audio + M5 Niagara) then P1 Frost/Earth/Water audio.
6. **P0 siege** impact audio + smoke/spark VFX.
7. Address flagged gaps (drums/horn purchases; per-school VFX) as a separate acquisition pass.
