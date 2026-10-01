# Settlement and Battle Variety Qualification

## FACT

- Worker: `D:\RefinedBadger\Worktrees\Soul-bannerlord-campaign-map-20260929`
- Branch: `codex/soul-bannerlord-campaign-map-20260929`
- Base: `6d53714559d34a123b5394206866925a165373c4`
- Licensed donor mounts are ignored junctions: `Content/AlienPlanet` and `Content/Fantasy_Pack`.
- No donor asset was edited, resaved, or staged.
- Ashport and North Pass use `SM_BigBetweenTower`; the hostile stronghold uses `SM_BigTowerComplex`.
- Human settlements retain the Kingdom Capital and Medieval Megapack silhouettes already present in the slice.
- The obsolete 3.8 km narrow Ashport pier was removed after runtime evidence showed it read as a debug line.
- Hostile Dragon Graveyard encounters now use Barbarian, Fantasy Barbarian, Viking Ulf, Orc Hummer, and Troll visual roles.
- Three geography-matched encounter recipes choose distinct authored spaces in `L_showcase_level`.
- Strategic authority remains in the existing campaign subsystem and battle bridge.

## Verification

- `python -m unittest Source.Soul.Private.Tests.test_weekend_campaign_view`: 5 passed.
- `python -m unittest discover -s Tools -p 'test_*.py'`: 77 passed.
- SoulEditor Win64 Development: passed after final source and test changes.
- Soul Win64 Development: passed after final production-source changes.
- Unreal automation `Soul.`: 69/69 passed, zero warnings, zero skipped (`AutomationFinal4/index.json`).
- Runtime licensed-asset audit: all 19 meshes and animations loaded (`asset-load-audit.json`).
- D3D11 campaign input pass: `Evidence/EvilCorridor-20261001/Local/settlement-input-8/summary.json`.
- Save/load, recruitment, legal movement, pan/zoom, exploration, and battle selection passed in that run.
- Clean Ashport evidence: `Evidence/EvilCorridor-20261001/Local/User/Saved/Screenshots/settlement-input-8_battle_prompt.png`.

- Campaign to Dragon Graveyard to campaign return: `Evidence/EvilCorridor-20261001/Local/settlement-roundtrip-1/summary.json`.
- Verified result: victory at Ashport, 18 allied survivors, 0 hostile survivors, RBSave persistence, campaign return.
- Battle screenshot: `Evidence/EvilCorridor-20261001/Local/User/Saved/Screenshots/Vertical_Battle.png`.
- Campaign return screenshot: `Evidence/EvilCorridor-20261001/Local/User/Saved/Screenshots/Vertical_Campaign_Return.png`.
- D3D11 stress pass: 35 active + 65 reserves per side, 70 simultaneous actors, 17 reinforcement waves per side, 1,555 contacts, 123 PBIL queries, one RB Magic cast, clean shutdown, `SOUL_RT_ARENA_PASS`.
- Stress result: player victory with 8 survivors and 0 enemy survivors after 55.97 seconds; the process remained healthy through the required 60-second post-result observation.
- Inspected stress screenshot: `Battle35FinalUser/Saved/Screenshots/Vertical_Battle.png`; it shows the authored Dragon Graveyard stone ring, lava gate, combat formations, and compact HUD rather than the former empty ground plane.

## INFERENCE

- The hostile silhouettes now read as a coherent faction family while preserving the existing nine-region topology.
- The role-based hostile roster provides visible formation variety without adding campaign factions or combat authority.
- Hyper Mesh to Icon is installed at `D:\Unreal Projects\HyperMeshtoIconCreatorv4`; its highest-value next use is a consistent far-zoom settlement icon layer after 3D silhouettes are approved.

## UNKNOWN / remaining production work

- Exact Hivemind Medieval Kingdom and Modular Viking Village packs are not currently available on disk; broken legacy junctions point to removed VaultCache entries.
- The current human capital is a strong vertical-slice silhouette, not a hand-authored production city.
- At 1280x720 the five hostile archetypes are subtle in the wide stress screenshot; a closer showcase shot or portrait/icon layer would communicate the roster more strongly.
- Biome dressing, authored road splines, settlement LOD/icon transitions, and a dedicated final lighting pass remain.
