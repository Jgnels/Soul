# Human Capital Diagnosis: why the capital exposes only a limited slice

**Lane:** SETTLEMENT_ENVIRONMENT_REGISTRY (read-only research)
**Authorities:** Jgnels/Soul (gameplay/source), Jgnels/Copperlight-Asset-Catalog (ownership)
**Scope note:** This is a diagnosis only. It proposes **no modification** to the Human Capital environment, maps, prefabs, props, Source/, Config/, Content/, `.uproject`, or save files. All remediation described below is deferred to the Codex integration lane / Remote-Desktop lane.

---

## 1. One-line answer

The Human Capital does **not** have a missing or incomplete environment. The complete authored map loads successfully. It exposes only a *limited slice* because the authored showcase ships a cluster of **pathological decorative "diner" micro-props** whose Nanite/build memory and build-time cost is wildly disproportionate to their gameplay value, so Soul's qualification policy deliberately exposes a reduced, production-safe slice of the architecture while those props are excluded/replaced and RB Optimization/HLOD handles the retained structures.

---

## 2. The complete authored environment loads (it is not missing)

- **Map:** `/Game/Medieval_Megapack/Levels/PL_Fortress_Day`
- Evidence: `Soul/Evidence/EnvironmentCaptures/safe_capture_manifest.json`
  - `load_ok: true`
  - `anchor_source: "LandscapeBounds"`
  - `center: [-0.986, 0.0, -770.39]`, `span: 50400.0`
  - Two real captures exist: `human_hivemind_overview.png`, `human_hivemind_perspective.png`
- Evidence: `Soul/Docs/ENVIRONMENT_ASSET_AUDIT_20260920.md` — "The full PL_Fortress_Day map loads and passes MapCheck". The pack "exposes 22 map/level assets locally and roughly 3,000 uassets in the cached payload."

So the authored capital is present, large (span 50400 UE units), loads cleanly, and passes MapCheck. The limited slice is a **policy/perf decision**, not an asset gap.

---

## 3. Root cause: pathological diner micro-props

The first real DDC build of `PL_Fortress_Day` exposed a handful of decorative meshes with extreme required-memory and build-time footprints. These are documented in two independent Soul evidence files with matching numbers.

### Required memory (from `Soul/Data/environment_qualification_rules.json` -> `human_hivemind.exclude_or_replace_first`, corroborated by `Soul/Evidence/hivemind_heavy_asset_audit.json` -> `top_memory`)

| Mesh | Required memory (MB) | Full path (where recorded) |
|---|---:|---|
| `SM_Cheese_Var1` | 5193.1 | `/Game/Medieval_Megapack/Diner_Colection/Props/SM_Cheese/Meshes/FBX/SM_Cheese_Var1` |
| `SM_WoodCup_SM_WoodCup` | 4454.9 | `/Game/Medieval_Megapack/Diner_Colection/Props/SM_WoodCup/Meshes/FBX/SM_WoodCup_SM_WoodCup` |
| `SM_WineBottles_Var3` | 3403.0 | `/Game/Medieval_Megapack/Diner_Colection/Props/SM_WineBottles/Meshes/FBX/SM_WineBottles_Var3` |
| `SM_SilverCandle` | 3256.1 | path recorded by UE audit when available |
| `SM_SilverCup` | 2158.7 | `/Game/Medieval_Megapack/Diner_Colection/Props/SM_SilverCup/...` |
| `SM_Cheese_Board` | 1534.3 | `/Game/Medieval_Megapack/Diner_Colection/Props/SM_Cheese/Meshes/FBX/SM_Cheese_Board` |

Six decorative props demand roughly **20 GB** of combined required memory. For comparison, the actual *architecture* in the same audit sits far lower: `SM_Tower_WalkWay_01` ~355 MB, `SM_Tower_Bottom_02` ~242 MB, and the wall kit (`SM_Wall_9M` ~4 MB, `SM_Wall_6M` ~2.9 MB, `SM_Wall_3M` ~1.8 MB) is trivially cheap.

### Build time (from `Soul/Evidence/hivemind_heavy_asset_audit.json` -> `top_build_time`)

| Asset | One-time build (s) |
|---|---:|
| `/Game/Medieval_Megapack/Diner_Colection/Props/SM_WoodCup/.../SM_WoodCup_SM_WoodCup` | 185.74 |
| `/Game/Medieval_Megapack/Diner_Colection/Props/SM_WineBottles/.../SM_WineBottles_Var3` | 157.19 |
| `/Game/Medieval_Megapack/Diner_Colection/Props/SM_Cheese/.../SM_Cheese_Var1` | 133.51 |
| `/Game/Medieval_Megapack/Diner_Colection/Props/SM_SilverCup/.../SM_SilverCup` | 84.59 |
| `/Game/Medieval_Megapack/Diner_Colection/Props/SM_Cheese/.../SM_Cheese_Board` | 54.58 |

Three cutlery/food props alone cost ~476 s (~8 min) of one-time build. By contrast the entire tower/wall/foliage set builds in fractions of a second to a few seconds each.

**Interpretation (per `Soul/Data/environment_qualification_rules.json`):** these are a *"Pathological Nanite/build footprint for inconsequential decoration."* The qualification policy's first instruction is literally `exclude_or_replace_first`.

---

## 4. Why this forces a limited slice (the policy)

`Soul/Data/environment_qualification_rules.json` (`human_hivemind.policy`):

> "Use modular architecture and selected dressing; do not ship the full showcase prop population unchanged."

The policy keeps the structure and drops the pathological decoration:

- **`architecture_keep`:** `walls, gatehouse, towers, Building_A, Building_A_02, Building_B, Building_C, Building_D, Forge, Tavern, scaffolding, market forms`.
- **`optimization_stack`:** `RB Optimization representation/culling/HLOD where applicable`, then `author-level removal of pathological micro-props`, retaining `full detail only on camera-important structures`.
- **`global.rule`:** "Asset demo richness is a donor, not a shipping requirement. City readability and tactical function outrank prop count." and `do_not_destructively_modify_donor: true`.

So the limited slice is the intended *production-safe* exposure: the architecture Soul actually needs (walls/gates/towers/prefab buildings/Forge/Tavern) minus the ~20 GB of diner micro-props the engine would otherwise try to build and resident.

This is reinforced in `Soul/Docs/FACTION_CITY_AND_SIEGE_PLAN_20260920.md` (Interior interaction scope, locked 2026-09-20):

> "The Hivemind town can therefore remain visually dense and fully enterable while Soul strips the extremely expensive diner-detail meshes that add no siege gameplay."

---

## 5. The corrupt alternate map (do not use)

`Soul/Docs/ENVIRONMENT_ASSET_AUDIT_20260920.md`:

> "The alternate `PL_Fortress_Day1` package is locally corrupt/unloadable (failed package name-table seek) and should not be used. `PL_Fortress_Day` is the qualified source map."

- Full path of the bad alternate (from `Soul/Data/settlement_blueprints.json` humans.donor.alternate_map): `Levels/PL_Fortress_Day1.umap`.
- Consequence: the capital cannot fall back to `PL_Fortress_Day1`. The *single* qualified capital map is `PL_Fortress_Day`. This narrows the production surface and is a second reason the exposed environment is a single, carefully-qualified slice rather than a pair of interchangeable maps.

---

## 6. Why only architecture bleed matters, not clutter (prefab contamination)

`Soul/Evidence/hivemind_prefab_external_refs.json` inventories external-actor references per prefab (Building_A/A_02/B/C/D, Forge, Tavern), each carrying ~93-100 external refs across 69-307 files. `Soul/Data/environment_asset_bindings.json` explains the human.witch_collegium binding to `Building_A`:

> "Building_A external-actor refs are unusually potion/wine/interior heavy; strongest existing arcane/adventurer shell."

That same potion/wine/interior density is exactly what pulls the pathological `Diner_Colection` wine/cup/cheese meshes into prefab payloads. So the diner-prop cost is not confined to a decorative corner of the map; it bleeds into the very prefab buildings Soul wants to reuse. Qualifying the capital therefore means exposing the architectural shells while *clustering/instancing/removing* the micro-props they reference, per the global interaction policy (`food_cups_plates_bottles_cutlery` are in the `cluster_or_instance` list, not the `interactive` list).

---

## 7. Remediation path (deferred, non-destructive, for Codex / Remote Desktop)

Recorded here for completeness; **this lane performs none of it.** Ordering comes straight from `environment_qualification_rules.json.optimization_stack` and `FACTION_CITY_AND_SIEGE_PLAN_20260920.md` (Optimization authority):

1. **RB Optimization first** — representation/culling/HLOD on retained architecture (walls/gates/towers/prefab buildings). RB Optimization is the authority (`Soul/Docs/AUTHORITY_MAP.md`: "RB Optimization: representation/performance state"); do not duplicate it with a bespoke city-wide system.
2. **Author-level removal/replacement** of the six pathological diner micro-props (and anything else the UE audit surfaces at that magnitude).
3. **Retain full detail** only on camera-important structures; cluster/instance the rest of the clutter.
4. Preserve collision/nav and interior tactical openings while simplifying props (interior-interaction scope locked 2026-09-20).

The net result the plan expects: the capital can be *visually dense and fully enterable* at a production-safe cost, exposing the full authored footprint rather than the current limited slice, **without destructively modifying the donor** (`do_not_destructively_modify_donor: true`).

---

## 8. Evidence index

| Claim | File | Key |
|---|---|---|
| Map loads, span 50400 | `Soul/Evidence/EnvironmentCaptures/safe_capture_manifest.json` | `load_ok`, `span`, `anchor_source` |
| Map passes MapCheck; ~3000 uassets | `Soul/Docs/ENVIRONMENT_ASSET_AUDIT_20260920.md` | "Hivemind UE qualification update" |
| Pathological prop memory | `Soul/Data/environment_qualification_rules.json` | `human_hivemind.exclude_or_replace_first` |
| Prop memory corroboration | `Soul/Evidence/hivemind_heavy_asset_audit.json` | `top_memory` |
| Prop build times | `Soul/Evidence/hivemind_heavy_asset_audit.json` | `top_build_time` |
| architecture_keep / optimization_stack | `Soul/Data/environment_qualification_rules.json` | `human_hivemind.architecture_keep`, `.optimization_stack` |
| Corrupt alternate map | `Soul/Docs/ENVIRONMENT_ASSET_AUDIT_20260920.md` | "PL_Fortress_Day1 ... locally corrupt/unloadable" |
| Prefab diner contamination | `Soul/Evidence/hivemind_prefab_external_refs.json` | `Building_A` refs; `Soul/Data/environment_asset_bindings.json` human.witch_collegium reason |
| Interior scope / strip diner meshes | `Soul/Docs/FACTION_CITY_AND_SIEGE_PLAN_20260920.md` | "Interior interaction scope — locked 2026-09-20" |
| RB Optimization authority | `Soul/Docs/AUTHORITY_MAP.md` | "RB Optimization: representation/performance state" |
