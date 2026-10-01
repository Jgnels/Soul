# Waterfront packaged RC2 — 2026-09-30

**Win64 Development RC2 is qualified at bounded settings. The next gate is Jeff's ordinary OS mouse/keyboard and physical controller playtest.** These receipts come from the archived `Soul/Binaries/Win64/Soul.exe`, not editor `-game`. No gameplay or licensed source assets changed in this continuation.

Launch locally with `Tools/Open_Soul_RC2.bat`, or directly:
`D:/RefinedBadger/Builds/Soul-RC2-Waterfront-20260930-01/Archive/Windows/Play_Soul_RC2.bat`.

The portable launcher starts the normal default campaign at **1280×720, D3D11, 30 FPS**, with sound and ordinary input enabled, no proof flags. Founder saves go to `%LOCALAPPDATA%/Soul/RC2-Waterfront-Founder/Saved/RBSave`, separate from automated checkpoints and the original player save. Keep the whole `Windows` directory together. `PLAYTEST.txt` lists controls and the included prerequisite installer.

## Verified package

Full build/cook/stage/package/archive through the existing script completed with UAT exit 0 in 1,142 seconds. Archive: 134 files, 3,523,110,082 bytes. SHA-256 of the real game executable: `1ae085eb83541dc2ab523d013bb804d2895b33fb70576d7a52f65bf70d61fc7d`.

All 48 reviewed roots are present in the actual IoStore container, including waterfront, Dragon Graveyard, RB Weather and Firebolt's Niagara dependency. All nine original loose runtime payloads plus the two supplemental TCAT resources match their source hashes. All 13 project plugins mounted; TCAT/PBIL are present, and no Hyper-named project plugin is enabled. Archive has no junctions/symlinks. Actual packed config retains normal campaign defaults. This is a self-contained archive audit and local runtime qualification, not a clean-machine installation test.

| Packaged gate | Result | Peak GPU | Receipt |
| --- | --- | --- | --- |
| Input, viewport selection, injected controller, exact F5/F9; 1080p | PASS, exit 0; 17 rendered captures | 69°C | `rc2-input-01.json` |
| Separate-process F9; 1080p | PASS, fresh capital → exact River Ford snapshot, exit 0 | 74°C | `rc2-load-01.json` |
| Dragon Graveyard victory and return; 720p | PASS, 22/0 survivors, reinforcements on both sides, mana 72, watch captured, F9 and returned fortress selection, exit 0 | 76°C | `rc2-victory-02.json` |
| Dragon Graveyard defeat and return; 720p | PASS, 0/63 survivors, hostile reinforcement, River Ford return, enemy ownership retained, mana 80, exact restored state, exit 0 | 72°C | `rc2-defeat-01.json` |

Accepted runs have no runtime errors or crash signatures. Inspected actual initial campaign, cold-load, battle and defeat-return frames. Terrain checks pass 105/105 route samples and 93/93 dry samples. Runtime evidence/logs/PNG artifacts are under the build root's `Qualification/<run>/`; `artifacts/` preserves each run's captures before later hooks reuse screenshot names.

**Rejected attempt:** `rc2-victory-01.json` reached the existing 85°C thermal cutoff at 1080p and its owned child was terminated (exit 1). It is not acceptance evidence. The successful retry uses 720p; the founder launcher uses that setting. No claim of long-soak or uncapped performance, audio qualification, physical input acceptance, Shipping, or production art approval.

## Bounded packaging corrections

- TCAT's runtime module registers two Slate icons omitted by UAT. `Stage-SoulTcatResources.ps1` now supplements those exact 2,293 bytes into stage and archive after UAT. Applied the same helper to this candidate after its first input run; executable and cooked containers stayed identical. Subsequent cold runs have no missing Slate-resource messages. Vendor source was untouched.
- Cook auto-appended Android File Server defaults to `Config/DefaultEngine.ini`. Removed only that generated append after verifying the original clean prefix. The actual packed config already excludes this section. Future package cook commands disable AndroidFileServer to prevent the side effect; the new command line is statically verified, not a second cook claim.
- Packaged qualification uses the existing bounded runner and hooks, starts in the archived game directory, records the executable hash, and isolates save/crash paths with `-UserDir`. `qualify_soul_rc2.ps1` rejects an existing user directory unless marked as an owned qualification directory.

Seven packaging checks, eleven runner checks, final static package preflight and `git diff --check` pass. Earlier 69 native and 71 Python/view passes remain prior integration evidence; no gameplay change warranted rerunning the native suite. TCAT's existing compile deprecation, shader-preload waits and Unreal's motion-vector console-variable warning are retained in logs; no warning-free build claim is made. Shipping was not attempted because current project policy/scripts select Development only.

## Reproduction and preservation

Run `Tools/qualify_soul_rc2.ps1 -PackageRoot <archive/Windows> -UserDir <fresh isolated directory> -Output <new run directory> -Mode Input|Load|Roundtrip|Defeat`. Input then Load must share UserDir. Use `-Resolution 1280x720` for battle modes on this host. Run one operation at a time; the runner stops at 85°C. Never supply the founder save directory.

`rc2-package-receipt.json` identifies the candidate, accepted/rejected runs and archive hash manifest. `rc2-package-audit.json` covers cook roots, loose payloads, plugins and actual packed config. `rc2-preservation.json` verifies unrelated dirty files, 3,655 licensed-file metadata entries, waterfront/heightfield hashes, and unchanged original checkpoint/backup. Automated save history exists only in the isolated qualification directory. No owned Soul/Unreal build/runtime process remains.

Host `DESKTOP-Q1S3RPU`; existing branch `codex/soul-bannerlord-campaign-map-20260929`; base HEAD `01526778807656bca32d789e747a354664ef75fd`. Changes remain uncommitted. No reset/clean/stash/branch change/merge/push/publish/purchase or external credential action. Continue with founder packaged play, not terrain comparisons or polish.
