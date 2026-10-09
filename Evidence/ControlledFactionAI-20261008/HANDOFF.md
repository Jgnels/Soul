# Controlled six-faction foundation — qualified closeout

Branch: `codex/soul-bannerlord-campaign-map-20260929`.
Starting HEAD: `18ba64b0ded98253237af74f6c8903af9e056a93`.
Gameplay/source milestone: `43060da2e40d415afe2dd24387e4b2f9d29bb2d3` — **Admit controlled faction actions and side-correct exact infantry battles**.
Ending HEAD: exact post-commit SHA in **final-head.txt**. This sidecar is written after the scoped evidence commit, avoiding a self-referential Git hash. `final-git-receipt.json` records both local commits and preserved dirty state.
No push, merge, default promotion, reset, clean, stash or donor save.

## Result

- CONTROLLED-AI ADMISSION POLICY: **PASS**.
- NON-HUMAN ATTACKER BATTLE SUPPORT: **PASS**.
- NEXT NATURE/DARK MATCHUP: **PASS — Nature, both directions against Humans**.
- ORDERED BATTLE PAIRS ADMITTED: **14 / 30**. All 14 have natural-result evidence: 11 fresh cooked pairs this mission; 3 inherited Human→Dwarf/Orc/Viking proofs retained with current focused native regression.
- READY FOR FIRST BOUNDED AUTONOMOUS MILITARY-AI EXPERIMENT: **YES**, opt-in observer scope through the existing admission API. Not a playable full-strategy release.
- FULL SIX-FACTION AI: **STILL OFF**.
- WORLD FOUNDATION: **KEEP**. Frozen 3.5 km composition remains opt-in; retained 1.5 km default unchanged.

## Controlled actions and safety

`PrepareControlledAction` validates canonical faction, exact living army/owner/source, destination ownership, legal edge, normal AP cost and battle readiness without mutation. Hostile actions require an explicit exact ordered roster pair, a living defender and a geographically matching existing playable recipe. `ExecuteControlledAction` revalidates profile, load revision and complete campaign-domain snapshot immediately before existing authority executes. Neutral captures use existing movement; hostile results use the existing bridge/RBCombat. No competing strategy/save authority.

Off-thread, malformed, missing-army, wrong-owner, invalid-region, nonadjacent, insufficient-AP, unsupported/unbound roster, stale profile/load/state, lost recipe and occupied-bridge cases reject safely. Replays and duplicate results reject. See `controlled-action-policy.md`, `adversarial-coverage.md`, `final-native-results.json`.

## Natural cooked battles

All rows below include actual exact bodies, accepted RBCombat contacts, natural result, side-correct ownership/survivors/return, F5/F9 and separate-process identical saved bytes. No forced winner or ownership override; staging traverses legal edges and ordinary day/AP rules.

| Attacker → defender | Natural result | Effective attacker / defender survivors | Target owner after result | Attacker return region |
|---|---|---|---|---|
| dwarves → humans | defender_victory | 0 / 23 | humans | dwarf_high_quarry |
| orcs → humans | attacker_victory | 20 / 0 | orcs | north_pass |
| vikings → humans | attacker_victory | 9 / 0 | vikings | viking_snow_pass |
| humans → nature | defender_victory | 0 / 10 | nature | river_ford |
| nature → humans | defender_victory | 0 / 26 | humans | orc_ruined_field |
| dwarves → orcs | defender_victory | 0 / 22 | orcs | dwarf_forge_approach |
| orcs → dwarves | attacker_victory | 16 / 0 | orcs | north_pass |
| dwarves → vikings | defender_victory | 0 / 8 | vikings | north_pass |
| vikings → dwarves | attacker_victory | 12 / 0 | vikings | dwarf_mountain_pass |
| orcs → vikings | attacker_victory | 18 / 0 | orcs | north_pass |
| vikings → orcs | defender_victory | 0 / 16 | orcs | dwarf_mountain_pass |

Per-pair `*-reverse-result.json` records target, physical survivors, reserve waves, RB contacts, slot, image hashes, runtime kind and temperatures. Existing morale defeat can zero effective campaign survivors while physical bodies/reserves remain; both are recorded. Human→Nature naturally won in editor and lost in cooked execution; Viking→Human lost in editor and won in cooked execution. Neither balance nor deterministic physical-combat replay is claimed.

Existing Dragon Graveyard `dragon_pass` supports exercised canonical North Pass/Dwarf Mountain Pass/Snow Pass encounters. Nature fights at canonical Orc Watch via the existing geographically matching `dragon_watch` fallback; this is not a new Nature forest/capital battlefield. Directed approach coverage remains **20 explicit /82 unset**. No speculative records were added. See approach and pair plans/matrices.

## Nature infantry and Nwiro

Selected from the primary AoEAssetRenderLab library within the 30-minute selection budget (decision ~5.5 minutes; native candidate gate within 14 minutes). Dark Executioner body/weapon exists, but no complete exact attack/hit/death set was verified in that candidate. Nature offered a complete native-skeleton infantry set with less adaptation. Dark is not rejected as future content; its battles remain explicitly unsupported now.

- Faction/unit: `nature / nature_bear_warrior`.
- Body: `/Game/Animals_Warrior_Pack/Mesh/Bear/SK_Bear_Full`.
- Skeleton: `/Game/Animals_Warrior_Pack/Mesh/Mannequin/SK_Mannequin_Skeleton`.
- Weapon: `/Game/Animals_Warrior_Pack/Mesh/Warrior_02/SM_Warrior_02_Axe`. Despite its package name, actual geometry/materials depict a **sword**; profile `Soul.Nature.BearBlade`, right-hand attachment.
- Twelve native full non-additive armed clips cover idle/crouch, directional movement, four attacks, hit and death. Native skeleton/side/reserve checks and eight rendered poses pass; actual cooked battles reviewed.
-36 packages copied read-only from `D:/Unreal Projects/AoEAssetRenderLab/Content/Animals_Warrior_Pack`; licensed payloads remain local-only with matching donor/copy hashes.

Nwiro was used directly against the installed UE 5.8.3 editor for project, body, weapon, skeleton and animation inspection. Its dependency registry was not ready after two attempts; no dependency-completeness claim was made. Actual cook manifests supply that proof. The read-only Nwiro session required bounded owned-process termination after WM_CLOSE; it is not a clean runtime-exit receipt. No donor map/package was saved. Source body LOD0 is 129,425 triangles, a future performance consideration rather than final optimization approval.

## Six-faction turn and persistence

`controlled-turn-cooked.json`: eight legal moves, six neutral captures, eight stale/replay rejections; 12 owned / 24 neutral becomes 18 owned / 18 neutral. All six identities, army ownership, troop counts, AP/day/resources survive real controller F5/F9 and separate-process restoration. Dark safely participates without a combat roster. Editor equivalent also passes.

RBSave campaign schema 1 and settlement schema remain unchanged. Existing slots are preserved; new proofs use named isolated slots. Older unbound Viking/Nature saves remain unbound rather than silently acquiring a roster. No 9→36 migration. Camera-pose persistence is not claimed. See `save-compatibility.md` and state receipts.

## Builds, source and cooked boundary

Fresh **SoulEditor, Soul and SoulComposition PASS**. `qualified-builds.json` records all 162 compiled source hashes plus target/module hashes. `source-admission-replay.json` ties that source to committed HEAD; no untracked C++ implementation required. Existing licensed/local assets and documented compatibility flags remain prerequisites. Exact file scope: `source-commit-scope.json`, reproduction instructions: `reproduction-boundary.md`.

**43 native tests PASS**: SixFaction 8, combat 12, Vertical 19, default CampaignWorld 2, Composition CampaignWorld 2. **17 source/tool tests PASS**. Known combat-test warnings concern inherited Paragon Rig imports and temporary test-world actor destruction, not assertion failures. An intermediate Viking test incorrectly expected Sword; only that test expectation was corrected, then all three targets rebuilt and the final suite passed. Rejected receipt retained. Earlier incomplete text logging and rejected pose framing are also preserved with explicit superseding evidence.

Additive Viking/Nature cook: **0 errors, 0 warnings**. Isolated stage uses the fresh Composition executable and 104 supplemental files against the preserved base cook. Pre/post checks verify 12,761 manifest entries, all 10,754 base cooked hashes, supplemental hashes and executable. Historical base manifest still matches. This is local staging, not store-ready distribution. The older unexplained temp-file aging/removal issue remains **undiagnosed**.

## Preservation, scope and limits

`preservation-final.json` verifies 496 protected baseline entries, 72 Nature donor/copy checks, 162 compiled source files and fresh binaries/modules. Frozen candidate height/map, retained reference, terrain donors, authored Human/Dwarf environments, previous saves and inherited dirty/config bytes remain preserved. Existing inherited modifications are listed in the final Git receipt; they are not included merely to make status clean. Large local logs/screenshots, licensed assets, mounts and inactive R10/Expansion evidence remain local.

No terrain, road, Dwarf exterior, authored-city framework or broad art work. Reviewed images are in `visual-review.html`, with actual inspection notes in `cooked-visual-inspection.json` and the Nature pose receipts.

All rendered runs were capped and guarded at 85°C. Final cooked functional peak: **72.0°C**. The earlier editor Dwarf proof peaked 83°C at 15 FPS; subsequent rendered functional runs used 10 FPS. **No new performance qualification**; the previous sustained 85°C cutoff remains open. Capped results are not a 40 FPS claim.

Automated battles control the attacker side. Interactive Human defensive hero/controller/HUD switching is **not qualified**; the Human-oriented Hero/spell UI remains visible to the observer, with non-Human mana 0. Nature's high source LOD cost, inherited ground tiling and existing world art debt remain. Dark lacks an exact admitted roster; Nature↔Dwarf/Orc/Viking remain explicitly unsupported. No six-faction release or final balance claim.

## Next action

Begin a separately scoped, opt-in bounded military action-selector experiment using this admission API: fixed seed, strict turn/action limits, legal candidates only, isolated save, no forced results. Retain observer scope until Human defensive controls are independently qualified. `next-bounded-experiment.md` defines that boundary; nothing activates automatically.

All requested mission phases and the additional six non-Human ordered pairs are complete. Actual elapsed qualification time is recorded in `final-closeout.json`; no 8-hour-duration claim is made.
