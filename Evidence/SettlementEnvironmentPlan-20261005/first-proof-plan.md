# First settlement proof: Crownstead

Status: the canonical gameplay foundation is committed as `2b53a655019ac77f835056beb7d9a93530ae6807`. Editor-r3/Game-r2, native 51/51, source 18/18 and Tools 99/99 passed. The nonvisual proof DA was authored and read back in a fresh editor. Controls-r1 passed two F5 saves, two F9 restores and six actual UI screenshots; visit/UI/qualifier commit awaits the controls evidence audit. The selected authored environment remains blocked by incomplete source packages. `authored_environment=0`, `miniature=0`, `battle_environment=0`: this is not a completed visual settlement proof.

## Source and assignment

Jeff explicitly chose [Hivemind Medieval Kingdom](https://www.fab.com/listings/42d4a792-2b66-423d-9b20-84d6b2c578d8). Its local family is **CastleTown**, with the authored persistent map `/Game/CastleTown/Levels/Persistant/PL_CastleTown`. `Kingdom_Capital` is a different LAYA pack; `Medieval_Megapack/PL_Fortress_Day` is a different Hivemind pack. Preserve those useful donors without relabeling them as the chosen capital.

Crownstead remains canonical `human_capital`, faction `humans`. No region, route, ownership rule, action cost or battle rule changes. The retained campaign map stays `/Game/SoulCampaignMountain/L_evil_waterfront`. The existing small campaign trial is temporary while the new integration boundary is qualified.

The real city loads at its native authored scale in a separate Soul-owned map. It will not be squeezed into the 27-metre campaign selection footprint. Duplicate only the necessary persistent/state-bearing map packages; keep original donor meshes, materials and complete source scenes unchanged. Proposed owned map namespace: `/Game/Soul/Maps/Settlements/`. Existing source sublevels can remain shared where runtime state binding is safe.

## One shared state and one upgrade

Use existing `USoulSettlementScenarioData` for starting buildings and development definitions, existing `FSoulTownRules` for costs, existing `FSoulSettlementRules` for construction/operation, and existing `USoulSettlementStateSubsystem` for saved progress. `Soul.Campaign` and `Soul.Settlements` remain the two RBSave checkpoint domains. Environment and miniature actor visibility are projections, never another save ledger.

First upgrade: `human.tavern`, enabling the already-existing tavern-companion action through definition token `service.tavern_hero`. The existing companion price/result and Knight recruitment remain unchanged. Build cost/duration are explicit provisional proof data, not final balance. Exact values are displayed from that data. `human.keep` is an initial prerequisite.

Required-at-start composition must preserve the source's identifying castle mass, main gates/walls, circulation and sufficient town fabric. Select the actual authored inn/tavern actor group after the complete scene is available. Classify exact actor GUIDs/level instances as start, buildable, optional decoration or incompatible. Do not guess that every matching house name is a tavern, remove the city wholesale, or use an arbitrary proxy house as the completed capital.

`ASoulSettlementBuildingActor` already controls intact/construction/damaged/ruined donor actor groups. Its construction precedence defect is repaired and the generic native presentation test passed. That test exercises actor visibility/collision branches, not authored city appearance. Qualify nested LevelInstance visibility and collision explicitly; hiding only a container does not prove its children disappear. A missing variant remains a documented gap rather than invented replacement art.

The existing campaign town panel now has build/visit actions only under explicit development-proof activation. Visit uses a Soul GameMode and authored `SoulTownViewAnchor`, the same GameInstance state, and the existing controller/HUD. Return restores the campaign map through its existing binding. Building, day advancement, hiring, F5/F9 and return all use the established authority. Campaign build/day/hire/save/load controls passed the real controls run. Visit code compiled, but travel, its authored camera/groups and return have not run against a qualified owned environment; the map binding remains null and travel fails closed.

## Strategic representation and performance

Derive the miniature from the selected authored composition using native merge/reduction or the owned Hyper Mesh-to-Icon workflow. Preserve major masses, gates, street orientation and material family. Keep a base representation and a separate tavern group so the same building ID drives both scales. Never derive a purported complete capital from the currently incomplete source copy.

Stream the real city only for visit/battle; campaign loads reduced meshes or deterministic rendered representations. Keep structural/collision-critical pieces separate from decorative clusters. Use RB Optimization before a bespoke culling manager. LODs, native PBR materials, HISM for repeated clutter and bounded shadow/foliage distances need rendered qualification on GTX 1080. Do not load full furnished city scenes throughout the strategic world. One heavy process, 85 C cutoff, and uncapped performance measurements remain required; no new performance pass is claimed.

## Acceptance sequence

1. **Blocked:** recover and verify all required donor external-actor/dependent packages; render the actual complete city before selecting groups.
2. **Data complete; visuals pending:** `/Game/Soul/Data/Settlements/DA_Soul_HumanCapital_DevelopmentProof` is authored and fresh-read verified. Four starting slots are built; tavern starts unbuilt with 200 gold / two-day provisional definition. The owned wrapper, native camera and exact actor groups remain pending; `OwnedEnvironmentMap` is null.
3. **Pending:** derive matching base/tavern miniature assets from the genuine composition and qualify silhouettes, materials, collision and budgets.
4. **Controls passed; appearance pending:** actual input proves one purchase, one advancement per day, service locked/unlocked and rejection of duplicate build/hire clicks. Authored construction/completion reveal at either scale is not yet demonstrated.
5. **Same-process save/load passed; travel/cold-start pending:** two F5/F9 round trips compare both domains exactly. Expected snapshots are Day 1 / 3000 gold/unbuilt, Day 2 / 3250 gold/one construction day left, and Day 3 / 2500 gold/completed+hired. Separate-process cold-load, visit/return and both visual representations remain unqualified.
6. **Pending:** admit a bounded authored-environment battle qualification at the existing bridge boundary with safe deployment/collision. Do not make friendly Crownstead hostile. The passing native bridge-continuity test is not combat in this environment.
7. **Build/native/regression complete for current tree:** Editor-r3/Game-r2 passed, native 51/51, source 18/18 and Tools 99/99. Controls used 1920×1080 / 20 FPS; this is not a performance pass. Authored-environment victory/defeat return and performance still need qualification. Promote only qualified visuals.

Evidence: [build/preservation receipt](preservation-build-receipt.md), [native r2 audit](native-r2-audit.md), [scenario authoring](development-scenario-authoring.json), [fresh read](development-scenario-fresh-read.json), [controls runtime audit](controls-runtime-audit.md), and [architecture/API audit](development-architecture-audit.md). Root inspected initial, mid-construction, completed, restored and final UI images. Keep the earlier [compile failure/repair](development-compile-repair-r1.json), [native fixture assertion/repair](native-r1-fixture-repair.json), and [historical cook receipt](development-cook-receipt.json) as history; their pending-asset/test status is superseded by the verified results above.

## Current recovery blocker

See `human-capital-local-identity.json`, `castletown-dependency-gaps.json` and `capital-donor-load-r1.json`. Fifteen external-backed maps lack their actor directories, including the Landscape and full-castle chain; two source assets are also missing. The old transfer host was unreachable in bounded checks. The browser exposed no download for the exact listing, and the Epic launcher could not be inspected through the available capture path. Jeff's ownership is accepted; acquisition access and complete bytes are the unresolved issues. A source-location/recovery question is pending while independent implementation and tests continue.
