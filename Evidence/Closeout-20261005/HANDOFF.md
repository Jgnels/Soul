# Soul recovery checkpoint — 2026-10-05

## Status

The existing Soul Bannerlord campaign-map worker has been reconciled after the Titan experiment and is ready to become the authoritative noncanonical continuation lane.

- Worktree: `D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929`
- Branch: `codex/soul-bannerlord-campaign-map-20260929`
- Pre-recovery HEAD: `195ea79c9b88096bafc8d18030a2980d945231f1`
- Recovery commit 1: `51dcd86` — qualified campaign terrain startup + HUD encoding fixes
- Recovery commit 2: `4b5dc8a` — qualified r11 battle animation/presentation closure
- No merge/default-branch change performed.

## What was recovered

### Campaign / terrain integration

The previously qualified fresh-launch terrain streaming repair is now committed. Soul loads the known long terrain package directly rather than relying on an asynchronously gathering asset-registry name search. This exact behavior was exercised by later r11 campaign victory/defeat qualifications.

The campaign HUD's corrupted separator characters were also replaced with stable ASCII separators.

Existing terrain/gameplay foundation remains:

- `/Game/SoulCampaignMountain/L_evil_waterfront` integrated through the Soul-owned adapter.
- Existing nine-region founder gameplay topology and ten legal founder routes remain authoritative.
- Terrain fitted campaign anchors/routes, landscape collision checks, camera/navigation, ordinary input, F5/F9, Dragon Graveyard battle and return have prior qualified evidence.
- `SoulTerrainPreview` retains the Mountain05, grassland, mesa and waterfront terrain R&D used by this lane.

### Realtime battle / animation / presentation

The r11 working state is now committed rather than existing only as dirty local source. It includes:

- battle animation variation and compatible reactions;
- directional locomotion / stance presentation;
- aerial fall/landing/death handling;
- reserve-collapse and advance-anchor fixes;
- front-line spacing behavior;
- bounded spell hit reactions and obstruction notices;
- field-strength HUD treatment and formation visual compaction;
- corrected dragon/griffin facing;
- bounded dragon-breath presentation with no gameplay authority;
- battle result labeling that distinguishes ordinary victory/defeat from routed outcomes;
- hammer attachment/presentation;
- associated regression/behavior tests;
- expanded exact cook roots.

## Verification at closeout

Fresh lightweight regression after committing:

- Tools Python tests: **82 passed, 0 failed**
- Campaign-view tests: **7 passed, 0 failed**
- r11 SHA-256 source snapshot: **43/43 exact matches, 0 mismatches**
- `git diff --cached --check` passed for both recovery commits.

The recovered source is the exact source state used by the previously qualified r11 package.

Existing r11 qualification evidence establishes:

- local Development build/cook/stage/package/archive: PASS;
- 181 exact cook roots / 2072 dependency packages;
- packaged campaign victory roundtrip: PASS;
- packaged defeat -> finite recruit -> retry -> defeat return: PASS;
- RBSave persistence exercised;
- 89 native automation tests at the r11 implementation stage: 88 clean + 1 known donor warning, 0 failed;
- clean warmed interactive battle performance:
  - 40 initial active: mean 16.304 ms (~61.3 FPS equivalent), P95 19.005 ms, P99 20.569 ms;
  - 60 initial active: mean 18.818 ms (~53.1 FPS equivalent), P95 21.953 ms, P99 23.888 ms;
  - no >33.333 ms frames in the accepted 60-active sample; one retained 44.6 ms frame in the 40-active sample.

These are qualification results, not release acceptance.

## Current terrain assessment

Soul does **not** need Project Titan.

The useful existing terrain work is already substantial:

- Mountain05 macro-geography study;
- central human plains;
- Grassland_02 terrain transplant;
- western Mesa_01/badlands forms;
- evil waterfront/coastal approach;
- collision/height alignment proofs;
- terrain-following route work;
- settlement-site planning;
- actual Soul campaign integration and battle roundtrip.

The important remaining gap is scale and composition, not lack of donor assets.

The structural Soul world already defines approximately:

- 36 strategic regions;
- 51 routes;
- 6 macro-regions;
- 14 major/candidate-minor settlement slots;
- intended ~9.35 km x 8.55 km world extent;
- low/mid/high elevation concepts;
- rivers, coast, mountain and forest belts;
- faction-distinct landforms.

The current fully integrated terrain work is a strong regional/founder proof, but it does **not** yet physically realize the whole 36-region world at that intended macro scale.

Next terrain work should therefore build from the existing Soul terrain R&D toward one coherent full-world campaign geography, instead of starting another donor comparison or replacing the working terrain/gameplay adapter.

## Intentionally local-only / not committed to GitHub

These are preserved locally but deliberately excluded from recovery commits:

- `Config/DefaultEngine.ini`
  - contains local Android File Server settings and a security token;
  - also carries the known pre-existing trailing blank-line hygiene issue.
- `Soul.uproject`
  - local Nwiro Integration Kit enablement.
- `.mcp.json`
  - local tool connection configuration.
- `Content/ParagonIggyScorch/`
  - licensed donor payload used for local presentation qualification.
- `Content/Soul/Audio/`
  - local derivative SoundWave assets whose source donors are licensed/local.
- large screenshots, raw logs, profiling CSVs, web caches, cooked/package outputs and generated captures under Evidence.
- mounted/junctioned licensed terrain/environment donor content.

Do not delete or reset these just to make `git status` clean. They are local support state, not source-of-truth Git payload.

## Recommended next lane

Continue this same worker branch after GitHub synchronization.

The next mission should be a deep terrain/world pass using GPT-6 Astra Ultra at normal speed, explicitly:

1. inspect the current integrated terrain and `SoulTerrainPreview`;
2. keep the working Landscape/height-query/gameplay adapter;
3. design and begin realizing the full six-macro-region Soul world at the intended campaign scale;
4. make geography explain existing strategic routes/chokepoints;
5. use owned realistic Soul assets rather than Project Titan;
6. preserve SoulCore / RB Save / RB Combat / RB Weather / RB Optimization authority;
7. validate rendered overview + campaign-height views, route grades, crossings, settlement footprints, input/save/battle roundtrip and GTX-1080 performance.

Project Titan was intentionally deleted and must not be reintroduced.
