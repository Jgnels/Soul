# Human Heartland depth — qualified local candidate

**PLAYTEST BUILD READY: YES.** Opt-in Heartland only. No push, merge or default-map promotion.

Start: `588d0db44ede6a255aa3f2cb80902aa9d6c10fec`.
Qualified source: `69b51de3367c1a15b98e5f0bc25d828e9ebe4fe0`.
Branch: `codex/soul-bannerlord-campaign-map-20260929`.
Final evidence-commit HEAD is recorded in local `final-head.txt`; the source commit above is the executable implementation boundary. Remote remains `4842eeb403f6b9be6effdf9fe18fc5c24b499887`.

## Play and review

- Launch `Tools/ProductionContinuation/play_heartland.cmd` from the Soul worktree. N starts a new persistent session; C restores its last F5. No automated inputs.
- Default **30 FPS**, 85 C hard cutoff, 1920×1080, sound enabled. Keep the guard console open. Functional runs were capped at 10 FPS; they do not qualify sustained 30 FPS safety.
- [Play instructions](PLAY_HEARTLAND.md), [actual cooked visual review](visual-review.html), [machine-readable closeout](final-closeout.json).
- Stage: `C:/Users/Jeff/AppData/Local/Soul/CampaignAlphas/FourFactionAlpha-hcxw3d__/Stage`.
- Executable: `Windows/Soul/Binaries/Win64/SoulComposition.exe` inside that stage.
- SHA256: `821a16413bf5ef4b3127f35fc8b6e1e56418dd1a68884dd10b32f447e1d3f1e9`.
- Receipt: `Local/stage-depth-r8/Diagnostics/receipt.json`. Isolation receipt: `staged-isolation-verification.json`.
- Saves: `Saved/CompositionPlaytest/HeartlandAlpha/HumanSessions/<session>/Saved/RBSave/Domains/`; profile `Soul.Composition3500.HeartlandAlpha`.

Launcher verification checked 13,348 resource entries, 94 candidate cooked hashes, the executable and admitted data. No obsolete stage is selected. Runtime acceptance comes from the cooked proof markers below, not from manifest presence or process exit alone.

## Product results

| Area | Result | Scope and limits |
|---|---|---|
| Human companies | PASS | Paid infantry `human_knight` (140), native Sparrow archers `human_archer` (180), Knight04 shield guard `human_guard` (220). Stock, barracks prerequisites, cap and exact company casualties persist. Cavalry is not qualified or simulated on foot. |
| Companion | PASS minimum | Rowan costs 1200, has explicit Companion/Unassigned state and appears physically near the authored city arrival. Native Knight04 body/idle. City embodiment only; no field follower/combat AI. |
| Windmill / Shrine | PASS, provisional art | Deterministic owned windmill assembly and Hidden_shrine buildings now represent the existing site interactions. Rewards, cooldown and full-mana rejection remain authoritative. |
| Quarry | PARTIAL art | Visible rock/scaffold/cart works, corrected burial, valid 140-gold interaction. Excavated quarry silhouette remains unfinished. |
| Battlefield variety | PASS scoped | Actual city outer approach; owned Woodland clearing; Southern Crossing stone bridge. Field approach is not a siege. |
| Orders | PASS focused contract | HOLD/MOVE persist, CHARGE enables pursuit, FOLLOW follows commander, R releases AI. Real RB movement/collision tests passed. Broader crowd/pathfinding quality remains provisional. |
| Development | PASS | Arcane Hall, Veteran Barracks and Expanded Market show paid state in both native city groups and deterministic campaign miniatures. They reuse authored facades, not new specialist interiors. |
| Mage Guild | PASS scoped | Initial guild cooked; higher tiers and Frost/Water/Air versus forbidden Fire/Lightning checked natively. Existing 1/2/3-day rule retained. |
| Kenney UI / cursors | PASS scoped | Curated CC0 panel/button/modal borders, current font, seven cursor states. Campaign, management, recruitment, companion/development, affinity, battle and diplomacy surfaces reviewed at 1280×800. |
| Audio/VFX | PARTIAL | Existing 11 battle clips reused by admitted roles/spells and load correctly. No broad campaign feedback-cue pass; functional tests were nosound, so audio was not auditioned. |
| Human siege | NOT REACHED | Bounded 12-strip gate survey encountered stacked geometry; no qualified continuous ground-level assault route or live gate-damage/objective adapter. This is inconclusive, not proof that the gate is impossible. |
| Diplomacy V0 | PASS | Human↔Dwarf/Orc/Viking only, existing AP/treasury/save authority. Paid gifts, peace, pact, betrayal and exact restore proven. No autonomous diplomacy. |
| Cold restore | PASS | New company/site/building/diplomacy/capture proofs and an actual pre-sprint Day 2 Crossroads save copied byte-for-byte into an isolated test directory. Original saves preserved. |

## Proven cooked play

`final-closeout.json` records each marker, receipt hash, executable hash and peak temperature. There are 13 accepted runs including separate-process restores, not 13 distinct product features.

- Paid Day 1–7 development and recruitment: 45 infantry, 3 archers, 3 guards; Rowan assigned; Arcane 1/Barracks 2/Market 2. Real AI actions/battle continued during construction.
- Woodland: paid checkpoint legally attacked Forest Edge; natural Human victory returned **18 infantry / 0 archers / 0 guards**. Reinforcements and F5/F9/cold restore passed.
- City: real Dwarf attacker and Human tactical defender. Final-binary mixed fixture redistributes the existing 45 soldiers to 37/4/4; grants no extra troops/resources. Natural defense returned **28 infantry / 4 archers / 0 guards**, then exact company save/load and cold restore.
- Bridge: Orc attack naturally won with **14 Orc survivors / 0 Human survivors**; Aurora captured; campaign returned at Southern Crossing. F5/F9 and final-binary cold restore preserved that loss.
- Final presentation: three developed authored buildings, Rowan, sites, quarry reward, full-mana rejection, remote F5/F9; separate-process restore.
- Diplomacy: two real 250-gold gifts, peace, a normal AI day, a pact, F5, paid betrayal, F9 exact pact/world/treasury restoration, separate-process restore.

The reviewed forest/site captures precede only later diplomacy/test-driver/UI wording changes. Final-binary mixed defense, diplomacy, bridge cold restore and legacy cold restore cover the final stage. We did not rerun every unchanged long proof.

## Diplomacy and save compatibility

Press **L**. Accepted actions cost 1 AP. Gift transfers 250 gold and +100 relation. Peace/pact acceptance uses an exposed fixed score: 400 + relation/2 minus recent betrayal, threshold 500. Two gifts make initial peace acceptable. Pacts last through Day+3 inclusive, then peace. Breaking one declares war, costs relation and records ten-day decaying betrayal memory. Peace blocks hostile movement/occupation in both directions; it grants no military access. Wounded/captured Aurora cannot negotiate. AI↔AI war and Nature/Dark passivity remain unchanged.

Optional `heartland_diplomacy` lives inside the existing Soul.Campaign domain; no RBSave schema/version replacement. Missing legacy company stock defaults to zero and grows through existing weekly rules. Legacy companion assignment derives from the prior hired flag. Validation occurs before state mutation. **Old executables do not understand treaties: use this launcher after creating a diplomacy save.** No 9→36 migration and no overwrite of earlier proof slots.

## Source, assets and reproducibility

`source-boundary.json` names all **98 admitted files** (40 existing, 58 new); `qualified-source-files.json` records their hashes and post-build whitespace-only changes. The commit includes Soul source, deterministic tools and 13 CC0 UI files. It does not include licensed Unreal payloads, local security configuration or inherited R10/Expansion work. Two C++ trailing empty EOF lines were removed after the final build; tokens are unchanged.

Required local licensed additions are recorded by `Local/depth-cook-boundary.json`, `Local/site-source-manifest.json` and the stage's supplemental cooked manifest. Donor windmill/shrine transfer: 103 selected files, 733,707,799 bytes, all SHA-verified; temporary service stopped. Tools used the existing live UE/Nwiro path for asset inspection/authoring; the packaged game intentionally disables the editor integration plugin.

Soul-owned maps:
- `/Game/Soul/Maps/Settlements/L_HumanCapital_Authored` (wrapper retained).
- `/Game/Soul/Maps/Settlements/SL_HumanCapital_Houses` (expected building-state group edit only).
- `/Game/Soul/Maps/Battles/L_Heartland_Woodland` (four trees removed for a 48 m clearing; native ground/trees retained).
- `/Game/Soul/Maps/Battles/L_Heartland_RiverBridge` (qualified owned crossing derivative).

The qualified source still requires the established local licensed assets/plugins. Git alone is not a redistributable game package.

## Validation and preservation

- Fresh **SoulEditor r14** and **SoulComposition r7** builds PASS. Final stage r8 uses the verified additive R2 cook plus fresh executable/data. Frozen base world was not recooked. Generic Soul target was not rebuilt or falsely claimed.
- **46 native tests on final Editor:** Heartland 9, FourFactionAlpha 7, Vertical 19, Settlement 7, CampaignWorld default 2 and Composition 2.
- **8 unchanged implementation tests carried from r12:** controls 6, bridge 1, ground 1.
- **37 source/tool tests + 16 launcher tests** pass (four overlap: 49 unique). See `Local/tools-final-r14.log` and `Local/launcher-tests-final.log`.
- **567 protected donor/reference asset hashes unchanged**, `Local/preservation-assets-final.json`. **17 inherited tracked hashes unchanged**, `Local/inherited-preservation-final.json`. Unrelated tracked/untracked state remains intentionally dirty.
- Intentional Soul-owned Houses map: old `5771463de5bfe0aaf27dd25aa61ea9e760979bf50e2ab73df0f61ecb79835238`; new `4b3ea86516be8d8e4bdf615f68db12395d63642b90bbff89bdf2ba6e99298c3e`. Byte-identical backup: `Local/SL_HumanCapital_Houses.before-depth.umap`.
- 3.5 km candidate, retained 1.5 km default/reference, donor environments, topology and authority preserved. No R10/Expansion activation. Raw evidence/screenshots and licensed assets stay local.
- Highest accepted capped functional temperature: **82 C**, guard 85 C. No uncapped test and no sustained FPS/thermal qualification. Inherited sustained thermal failure remains unresolved.

## Remaining limits and next action

City/forest brightness and donor material warnings, quarry art, bridge crowding, formation pathfinding breadth, scarce-company balance, specialized building interiors, field companions and cavalry remain provisional or unimplemented. No new audio/UI cue completion claim. These are not reasons to reopen terrain, faction matrices or city frameworks.

**Next highest-value four-hour sprint:** one real Human Capital Siege V0. Establish a true ground-level gate approach and assault lane first, then reuse existing combat/campaign authority for gate damage, objective completion and result persistence. Keep this qualified outer-approach battle as the fallback; never call it a siege.

**Product judgment: YES, materially closer** through embodied city development/Rowan, paid differentiated companies, meaningful sites/diplomacy and geographically distinct tactical encounters. This does not claim finished production art, broad siege coverage or Bannerlord/Total War quality parity.
