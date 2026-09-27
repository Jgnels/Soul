# Post-consolidation source continuation

Base: `7a6b293` on `astra/soul-finalproject-consolidated-20260927`.
Authoritative worktree: `D:\RefinedBadger\Worktrees\Soul-finalproject-consolidated-20260927`.
Frozen packaged RC was not accessed. No UE/UBT/UAT/cook/package ran.

Three separate review/commit batches:

1. Controller cursor hit-test and sole campaign click dispatch, with 3 static
   regressions. See `SOUL_INPUT_CORRECTION_20260927.md` for manual acceptance.
2. RB Save failed-load recovery across selected domains, with provider-order and
   corrupt-checkpoint regressions. See `SOUL_CHECKPOINT_RECOVERY_20260927.md`.
3. Command/reinforcement readability and conservation coverage: persistent order
   text counts live groups actually matching HOLD or CHARGE/approach and identifies
   the order new arrivals inherit. Immediate order feedback reports accepted/eligible
   groups. Next-wave size comes from `PreviewWave`, which copies the ledger and calls
   the existing allocator; presentation has no separate reinforcement calculation.
   Existing last-arrival feedback remains. Added repeated-preview, discarded-candidate
   retry and conservation assertions at 15/25/35 per side (30/50/70 combined) to
   UE automation and the existing native adapter. Physical actor rollback is not
   established by these pure-ledger tests.

Executed checks:

- `python -m unittest discover -s Tools -p 'test_*.py'`: **54 passed**.
- `python -m unittest Tools.BalanceLab.tests.test_balance_lab`: **13 passed**.
- `python Source/Soul/Private/Tests/test_weekend_campaign_view.py`: **2 passed**.
- `git diff --check`: passed.

All changed C++ and authored UE/native tests remain uncompiled/unexecuted. The
previously recorded corrupt MSVC headers were not polled again. No physical
30/50/70 qualification or rendered HUD acceptance is claimed. Supervisor must
run `Soul.RealtimeBattle.WavePreviewConservesAt30To70Active`, existing reinforcement
tests and the native runner with a healthy compiler, then check G order feedback,
partial order failure, HOLD inheritance, empty frontline/refill, final exhaustion,
spawn-failure recovery and HUD fit in supported resolutions.

Commit creation blocked on the first staging attempt: Git could not create
`D:/RefinedBadger/Games/Soul/.git/worktrees/Soul-finalproject-consolidated-20260927/index.lock`
(`Permission denied`). HEAD remains `7a6b293`; this source batch is uncommitted.
No alternate index/repository or permission bypass was used. The separate patches
under `Saved/ContinuationPatches-20260927` are transfer/review artifacts, not commits.
Do not apply them over the already-edited worktree. A session with Git metadata
write access must stage/commit these batches, input correction first.

The seven-slot rule remains 4 melee + ranged + support/magic + apex. Hero/paragon
entities stay separate; Nature apex stays undecided. No changes to those settled
foundations, faction content, magic assets or admitted battlefield maps in this batch.
