# Candidate cooking and promotion isolation

The default `Soul` target, `GameDefaultMap=/Engine/Maps/Entry`, retained terrain profile and project configuration remain unchanged. The candidate is still explicitly opt-in via `-SoulComposition`; neither a successful build nor cook authorizes promotion.

`Source/SoulComposition.Target.cs` uses the existing SoulCore, Soul and SoulRealtimeBattle modules. It introduces no gameplay authority. Its actual UBT receipt is checked by `target-receipt-verification.json`: four additional composition files are NonUFS dependencies, with no rejected World/Expansion data staged by that target.

`Data/CampaignComposition/PackageProfile.json` declares 216 explicit full-runtime cook roots derived from the existing required loads, excluding the six rejected R10 roots. The inherited Expansion-only r9 string load is recorded separately rather than silently admitted as a production root; saved-map native dependencies are still followed normally. The existing global package configuration is intentionally not rewritten or treated as candidate promotion.

The actual first cook used the full 216-root runtime profile. It stopped at the storage guard after 4,261 packages, with 293 dependencies still pending. All explicit root files existed, but that did not prove dependency completion; the run is correctly marked incomplete. Its 14,083,233,660-byte partial output remains preserved. One bounded retry uses a fresh OS temporary payload directory on C:, with receipts remaining in this evidence directory. No donor, old cook, cache or evidence is deleted. Final cook/stage status must be read from the receipt, not inferred from root-file presence.

Disk headroom is limited. The read-only AssetRegistry study measured about 5.81 GiB of source packages in the candidate closure and 25.25 GiB across the larger runtime roots before removing the unnecessary explicit r9 root. These are uncooked source sizes, not fabricated cooked-size estimates. The cook stops before consuming the last 8 GiB, and also preserves the 85 C GPU, 20 GiB process-commit and 4 GiB host-commit-headroom guards. No previous cook, cache or evidence is deleted to make room.

The worker still contains qualified composition adapter changes overlapping inherited inactive source drafts. Exact new changes are preserved separately in `source-delta-working.patch`; the previous production push has its own baseline-relative patch. Build receipts qualify this worker, not a clean checkout of HEAD alone. Unrelated inherited source and machine-specific configuration are not silently committed.

Save profiles now use separate existing RBSave slots. See `save-compatibility.md`. The six-faction starting-position manifest is validated but remains inactive; see `six-faction-groundwork.md`.

## Local staging option

The prepared `--loose-hardlink` stage uses the installed Windows UAT loose-file path (no `-pak` or `-iostore`). In a fresh same-volume temporary stage, cooked binary assets are hardlinked to this run's own completed cook. Installed `CommandUtils.CopyFileIncremental` skips identical timestamps; source hashes and same-file identity are checked after UAT. Metadata/config text is not prelinked. No source/donor content is linked into the stage. This is a local cooked qualification build, not a distributable archive. The source project/default configuration remains unchanged.

This avoids a second large payload copy while preserving both cook attempts. The stage volume retains its 8 GiB guard; the source evidence volume, which only receives small logs/manifests, retains 2 GiB. The 85 C and host-commit guards are unchanged. A successful stage still requires actual input/save/render qualification from `SoulComposition.exe`. No stage success is claimed merely because this option exists.

## Verified cook and discovered runtime boundary

The second full-profile cook completed: 4,518 packages, zero errors, nine unique warnings, 18,049,947,229 cooked bytes. Three existing CastleTown algae material instances report default-material fallback; they were not silently repaired or called final art. The first D: attempt remains preserved.

The first loose stage succeeded but its actual executable failed because HDRIBackdrop content lacked the mount descriptor. The retained fix declares the exact engine `.uplugin` as a NonUFS dependency only for SoulComposition. A target-wide EnablePlugins attempt was rejected by the installed shared build environment and abandoned. Fresh target metadata and a new stage now include the descriptor, without recompiling gameplay code or changing the project/donor plugin. `packaged-boundary-repair.json` preserves both the failure and correction.

Actual staged verification confirms all four composition payloads and canonical world/start JSON bytes match source, experimental payloads and machine support files are absent, default Entry map remains unchanged, and the local Android editor token is already filtered by UAT. No extra sanitization was needed. The linked cooked files were hash-identical after both UAT stages.

## Completed cooked boundary

Corrected stage `Local/stage-loose-r2/Diagnostics/receipt.json` passed actual game-executable qualification: normal input/F5/F9, independent fresh-process F9, and the Human authored development/miniature/visit/battle/natural-victory/return proof. The Human run returned with 33 allied survivors and zero enemies, then restored both RBSave domains exactly. Peak temperature for that capped functional run was 84 C. This does not change the separate uncapped performance FAIL.

The actual campaign log confirms `regions=36 routes=51 world_experiment=0 expansion_experiment=0`, the exact composition map, 81/81 startup collision samples and 0.2065 cm maximum startup discrepancy. No default-map promotion occurred. `packaged-runtime-results.json` contains exact receipts and image hashes, including the original missing-descriptor failure.

`stage-footprint.md` / `.json` measure 12,658 files and 17.56 GiB apparent size. This includes 3.33 GiB CastleTown and 2.46 GiB DwarvenCitadel; the current candidate family itself is about 0.121 GiB cooked. Hardlinked cook/stage bytes are not counted as independent disk allocations. All 95 staged INI files were checked without printing values; the sole sensitive-named engine setting matches installed engine source and is recorded redacted. No Soul machine configuration is committed.

`Tools/ProductionContinuation/play_candidate.py` provides a guarded, visible manual playtest entry point. Both normal and Human launch plans were verified without a new game run. It keeps the qualified temporary stage opt-in, uses separate persistent test UserDirs, and refuses changed candidate hashes. The 20 FPS review cap is explicitly not a performance measurement.

`final-local-recovery.json` records a verified 241-file local-only archive of candidate assets, current source and composition data/height. Donors and full authored cities are not duplicated. No destructive cleanup or licensed-payload commit was used.

Before any default promotion, reconcile the qualified composition adapter source separately from inherited inactive R10/Expansion drafts, preserve the explicit save policy, and resolve the hardware/performance gate. This continuation deliberately does not claim clean-checkout reproducibility from HEAD alone.
