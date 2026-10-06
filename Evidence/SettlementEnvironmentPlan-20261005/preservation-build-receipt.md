# Settlement development preservation and build receipt

**Preservation and completed validation evidence verified. Full Human Capital dependency closure remains unresolved.** Exact baseline/current SHA256 comparisons and result/log hashes are in [preservation-build-receipt.json](preservation-build-receipt.json).

| Verification | Result | Evidence |
|---|---:|---|
| CastleTown protected donor packages | 704/704 unchanged; 0 changed/missing | [Baseline manifest](castletown-donor-maps-before.json): 43 maps plus 661 external packages, 288,973,257 bytes |
| DefaultEngine.ini, Soul.uproject, .mcp.json | 3/3 byte size and SHA256 unchanged | [Protected baseline](preserved-before.json); no protected content disclosed |
| SoulEditor Win64 Development, r3 | Exit 0; `Result: Succeeded`; NoPCH | [Result](Local/Builds/editor-settlement-r3.log.result.json), [log](Local/Builds/editor-settlement-r3.log) |
| Soul Win64 Development, r2 | Exit 0; `Result: Succeeded`; NoPCH | [Result](Local/Builds/game-settlement-r2.log.result.json), [log](Local/Builds/game-settlement-r2.log) |
| Native r2 | 51/51 completed Success, no missing/duplicate/unexpected completion | [Audit](native-r2-audit.json), [review](native-r2-audit.md) |
| Source checks | 18/18, OK | [Receipt](source-checks-root-r1.txt) |
| Tool checks | 99/99, OK | [Receipt](tools-checks-final-r1.txt) |

The native audit's expected-test manifest hash and exact 351,424-byte log-prefix hash were independently rechecked. Its 51 tests comprise 28 Core, 18 Vertical, four Settlement and one settlement save-domain round trip. Later editor log appends do not invalidate that bounded prefix. Native tests establish functional rules and integration behavior; they do not establish a finished authored environment or rendered state parity.

Earlier failures remain historical evidence:

- Editor r1 exited 6. The bounded repair added the explicit `Engine/GameInstance.h` include and renamed the shadowing local scenario variable. [Failed result](Local/Builds/editor-settlement-r1.log.result.json), [compile repair](development-compile-repair-r1.json).
- Native r1 asserted because the duplicate-definition test fixture passed an element of a TArray back to the same array's `Add`. It was fixed by copying the fixture value first, then rebuilding and rerunning. [Failed audit](native-r1-audit.json), [fixture repair](native-r1-fixture-repair.json). Native r1 is not counted as a pass.
- Intermediate successful Editor r2/Game r1 logs are hashed in the JSON history; the final successful build pair above supersedes them for this receipt. The scenario cook change has its own [bounded receipt](development-cook-receipt.json).

The donor comparison covers exactly the 704 files inventoried before inspection, not all 2,457 local donor files or the complete original capital. Missing original packages and external-actor closure remain documented in [dependency gaps](castletown-dependency-gaps.json) and [local identity](human-capital-local-identity.json). Unchanged mounted packages do not prove that the full authored capital has been recovered.

The parent's active runtime-controls attempt is excluded. No controls, visit, miniature, battle-environment, visual-quality or performance acceptance is inferred here. This audit launched no UE/build, changed no source or donor, and only hashed the protected project files.
