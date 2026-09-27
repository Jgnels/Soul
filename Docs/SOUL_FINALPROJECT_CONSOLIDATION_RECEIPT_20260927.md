# Soul final-project consolidation receipt ? 2026-09-27

## CURRENT ITEM
Source reconciliation and a bounded continuation batch are implemented in the authoritative worktree:
`D:\RefinedBadger\Worktrees\Soul-finalproject-consolidated-20260927`.
Branch: `astra/soul-finalproject-consolidated-20260927`.
HEAD remains `5c3656175dbe2b530ead96151978a05add0538d8`; changes are **uncommitted**.

Acceptance achieved at source level: coherent donor additions reconciled into existing authorities; clean checkpoint/recovery behavior retained; independent roster, hero, rank, reinforcement and audit work added; available non-UE checks passed. Commit acceptance and C++/runtime acceptance remain open.

No donor modifications, no packaged RC access or modifications, and no UE/UBT/UAT/cook/package launch. No new map was enabled. No Nature apex, final faction content, or new art direction was invented.

## DONOR RECONCILIATION
Compared dirty donor against committed base `0466e7f` and clean branch against the same base. SHA-256 verification confirms all 24 donor dirty/untracked files are unchanged from session start. The donor's old receipt was treated as a claim, not execution evidence.

| Donor area | Classification | Resolution |
| --- | --- | --- |
| Campaign actor | Already equivalent | Retain clean HUD/recovery implementation. |
| State header and continuation JSON | Already equivalent | Retain clean versions; third garrison and candidate configuration already present. |
| State checkpoint and exhausted-garrison hunks | Already superseded | Keep clean atomic validation and recovery; no second campaign authority. |
| State mana hunks and battle bridge/result header | Compatible-additive | Shared `IsValidFor` predicate, actual campaign mana in descriptor/result, atomic checkpoint receipt; campaign remains consequence writer. |
| State battlefield hunks and recipe header | Conflicting overlap | Preserve `BattlefieldId` / `BattleContext`; extend existing recipe rules with explicit playable admission. No duplicate donor descriptor names. |
| Vertical campaign tests | Mixed duplicate/additive | Preserve clean regressions; carry remaining mana in victory fixture; consolidate geography coverage into one expanded test file. |
| Battle-result validation and mana tests | Compatible-additive | Port and extend with invalid receipts, both result admission paths, legacy migration, and paragon persistence. |
| Battlefield-selection tests | Compatible coverage / conflicting identifiers | Adapt to clean names; cover admission, geography, fallback, lexical ties, frozen map/origin and no preview mutation. |
| Reinforcement rules and scale tests | Compatible-additive | Honor formation limits below side cap; conserve pools at 15/25/35 per side; extend readiness and normalized-cap regressions. |
| Arena and header | Mixed additive/superseded | Keep clean optional spell API and matching profile guard; add valid-caster check, mana transfer, admitted-map binding, 35-side cap, persistent reserve readiness/last arrival HUD. |
| Log auditor and tests | Already equivalent | Do not duplicate donor edits. Add new mana receipt audit as independent continuation. |
| Continuation data tests | Compatible-additive topology only | Port secured retreat-route check; keep clean candidate-admission assertion. |
| Native adapter and runner | Compatible test tooling | Port test-only adapter; runner verifies RB group limit. Native execution is blocked, not passed. |
| Donor handoff and physical acceptance claims | Unqualified/stale | Do not import. Apex question is settled; no new runtime or scale acceptance claimed. |

Campaign commitment/rewards/save remain in `USoulFounderPlaytestStateSubsystem`; transport validation lives in `USoulCampaignBattleBridge`; magic gameplay remains in the existing Soul/RB Magic commit path. RB Save remains the persistence authority.

Continuation beyond donor:
- Finished-roster validation enforces 4 melee + 1 ranged + 1 support/magic + 1 apex within seven; checks one quadruped independently of role and rejects duplicated/overlapping hero identities.
- Hero/paragon kind travels through existing hero recruitment and optional schema-1 checkpoint field; old checkpoints default to Hero. No new playable hero, apex assets, or troop slot.
- Reserve readiness now requires an actual receiving formation; invalid cap normalization agrees between initial deployment and subsequent waves. Hero/apex priority regressions authored.
- Five-rank progress text derives from existing veterancy thresholds. XP saturates at int32 maximum rather than overflowing; rank text is a card foundation, not a rendered UI claim.
- Mana-aware log auditing checks committed, physical, and campaign balances, identity, casts, duplicates, ordering, and missing receipts. Older logs remain supported.

## COMMITS
**None created.** `git add` failed once:
`Unable to create D:/RefinedBadger/Games/Soul/.git/worktrees/Soul-finalproject-consolidated-20260927/index.lock: Permission denied`.
The sandbox allows source writes but denies shared Git metadata writes. No permission bypass, alternate Git authority, or donor mutation attempted.

Ten ordered source patch batches substitute for review/transfer, not for commits:
1. reinforcement caps and retreat tests
2. shared result validation and campaign mana
3. playable battlefield admission
4. reinforcement readability and active scale
5. seven-slot apex and paragon foundation
6. reserve readiness and normalized wave caps
7. resource receipt adversarial regressions
8. existing five-rank readability
9. mana continuity log audit
10. roster direction and receipt fixture review

Each patch passed `git apply --check --whitespace=error` and applied in sequence to a scratch copy of starting HEAD; exact normalized text equality verified for all 31 source/test/mechanics files. LF patch normalization was corrected during this check. Receipt documentation is an additional final patch.

Patch directory: `Saved/ConsolidatedPatches` in the authoritative worktree.
Supervisor transfer archive: `D:\RefinedBadger\Parallel\Supervisor-20260926\SOUL_FINALPROJECT_CONSOLIDATED_PATCHES_20260927.zip`.
Do **not** reapply patches over this already-modified worktree. In a writable Git session, commit the current source in the recorded batches, or apply the sequence to a fresh `5c36561` checkout and commit each patch. No cherry-pick of the dirty donor chain is required.

## TESTS
Executed here:
- `python -m unittest discover -s Tools -p 'test_*.py'`: **51 PASS**, including 16 log-audit and 4 continuation data/topology tests.
- `python -m unittest Tools.BalanceLab.tests.test_balance_lab -v`: **13 PASS**. Lab mirrors/glue, not production C++ execution.
- `python Source/Soul/Private/Tests/test_weekend_campaign_view.py`: **2 PASS**. Source/geometry checks, not rendered acceptance.
- Five non-UE validators: **5 PASS** ? world overmap, campaign starts, battle handoff, directed approach profiles, battlefield environment expansion. Existing validator outputs restored after copying fresh evidence to `Saved/ConsolidatedValidation`; no unrelated generated-data changes retained.
- `git diff --check`: **PASS**.
- Ordered patch replay: **PASS**, exact source reproduction; donor SHA-256 comparison: **24/24 unchanged**.
- Native runner attempted once: **NATIVE_BLOCKED** before compilation. Installed `D:\DevTools\BuildTools\VC\Tools\MSVC\14.44.35207\include\algorithm` begins with zero bytes. No toolchain file was modified. Do not count adapter assertions or authored C++ automation as executed tests.

## RUNTIME_REQUIRED
All changed C++ remains uncompiled. Supervisor owns runtime scheduling; do not launch these concurrently with that lane.
- Build/UHT/link all touched Soul modules; run `Soul.Integration.Vertical`, `Soul.Core.Faction.SevenUnitContract`, `Soul.Core.HeroRecruitment`, `Soul.Core.Regiment.Veterancy`, and `Soul.RealtimeBattle` automation.
- Three-encounter/reload and loss/recruit/retry flows with mana continuity; atomic invalid receipt rejection; legacy schema-1 restore; hero/paragon kind roundtrip.
- Physical 30/50/70 actor support: body/reserve conservation, group limits, spawn-failure rollback, stale actor reuse, HOLD inheritance, frame time and actual result return.
- HUD fit at supported resolutions, command/reserve legibility, optional absent/mismatched spell presentation and normal Firebolt gameplay. No new VFX assets/cook roots added.
- Campaign-selected map/origin setup: Dragon Graveyard remains the only enabled binding. Other maps require independent ground/bounds/payload/cook qualification before enabling.
- Native adapter compile/run once a healthy non-UE compiler is available; adapter does not verify Unreal ABI, reflection, actor behavior or FName semantics.

## NEXT THREE HIGH-VALUE ITEMS
1. From a session permitted to write shared Git metadata, review and create the small commits, preserving the existing clean checkpoint chain. No source reconciliation redo needed.
2. Extend the existing regiment/card consumer and hero recruitment presentation when their content/runtime integration is admitted; use `FSoulVeterancy::ProgressSummary` and hero kind without deriving troop rank from hero level. The vertical tavern still uses its existing hired flag; it is not upgraded to a new playable companion in this batch.
3. Supervisor runs the C++ and ordinary-input gates above after its runtime lane is available; independently qualify one geography-specific map candidate before enabling it. Keep settled roster validation; leave Nature apex unresolved.

## UNVERIFIED CLAIMS
No UE build, runtime, screenshot, UAT, cook or package acceptance. No claim that 70 physical actors are qualified, campaign balance is accepted, or alternate environments are safe. No final seven-unit faction data has been populated. Rank progress has no newly rendered card consumer. Hero/paragon foundation carries identity only, with no invented stat bonuses. Battle result mana is integer remaining balance floored from the existing float runtime; no mana-restoring battle effect is supported by the bound. Source tests cannot establish UObject lifetime, visual readability, process behavior or gameplay balance.

## LEVEL-4 BLOCKER
No founder product decision is required for this source batch. **Environment blocker:** shared Git metadata write permission prevents commits; installed compiler-header corruption prevents native compilation. Both are isolated from the completed reversible source work. Runtime remains reserved for the supervisor by explicit instruction.
