# Reproducing the admitted source boundary

Source milestone: `43060da2e40d415afe2dd24387e4b2f9d29bb2d3`, on `codex/soul-bannerlord-campaign-map-20260929`. The qualified-builds receipt records all162 compiled source hashes, three target binaries and editor-module hashes. `source-admission-replay.json` verifies that HEAD reproduces the source bytes after Git line-ending normalization. No untracked C++ implementation is required.

This is a local licensed-content boundary, not a self-contained public checkout. Keep the existing owned content/mounts and local machine configuration. The36 Nature packages remain local-only; their exact donor/copy manifest is `nature-donor-copy.json`. Do not commit those vendor packages.

Fresh target commands used the installed UE5.8 Build.bat, `Win64 Development`, the existing Soul.uproject, `-WaitMutex -NoPCH -NoUBA -MaxParallelActions=2 -ForceRulesCompile -gather`. SoulEditor additionally used `-OverrideBuildEnvironment` and the existing forced include `Tools/SoulNoPchCompatibility.h`. Run one heavy build/UE process at a time. Exact build output is retained in `corrected-SoulEditor-build.log`, `corrected-Soul-build.log`, `corrected-SoulComposition-build.log`.

The additive roster cook receipt is `Local/cook-factions-r1/Diagnostics/receipt.json`. It admits only the explicit Viking/Nature closure against the historical composition cook. The stage is `Local/stage-controlled-factions-r1/Diagnostics/receipt.json`; pre/post manifests and hashes define its actual file boundary. This is isolated local staging, not distribution packaging or default-map promotion. The existing temp-file aging/removal problem remains undiagnosed.

Runtime proofs use `Tools/ProductionContinuation/qualify_runtime.py` through the existing controller/bridge, with isolated UserDirs/save slots and capped10FPS/85C guarding. `run_cooked_pairs.py` specifies the exact ordered fixtures; `run_cooked_closeout.py` sequences the remaining reverse Human, all-six cold-restore and stage-integrity gates. These scripts refuse failed prerequisites and do not prescribe battle winners.

Final native authority is `final-native-results.json`: SixFaction8, combat12, Vertical19, default CampaignWorld2 and Composition CampaignWorld2. Source/tool tests:17. Rejected intermediate test/pose receipts are preserved and superseded explicitly, never deleted.

No R10/Expansion feature, new save schema, autonomous scheduler or terrain/art change was admitted by this mission. Full six-faction AI is OFF.
