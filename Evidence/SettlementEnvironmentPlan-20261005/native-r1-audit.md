# Native r1 attempt — FAILED / INCOMPLETE

Only **31 of 51** expected tests completed with `Success`. The process asserted during `Soul.Integration.Settlement.DevelopmentDefinitionsRejectInvalidData`; it has a start record but no completed result. **19 tests never started.**

| Group | Expected | Completed success |
|---|---:|---:|
| Soul.Core | 28 | 28 |
| Soul.Integration.Vertical | 18 | 0 |
| Soul.Integration.Settlement | 4 | 2 |
| Soul.Integration.Save.SettlementDomainRoundTrip | 1 | 1 |

The log reports `Array.h:2196` rejecting an element from the same container being modified. The native stack points to `SoulSettlementDevelopmentTests.cpp:55`, then the validation-test lambda/caller. It subsequently records `StaticShutdownAfterError` and `RequestExitWithStatus(1, 3, ...)`. This is an aborted attempt, not a completed failed-test row or a suite pass.

[native-r1-audit.json](native-r1-audit.json) preserves exact completion records, missing names, assertion lines, log hash and source drift. [native-expected-tests-r1.json](native-expected-tests-r1.json) preserves the original51-name expectation. No queued/started marker was credited as success. Test-only fixture repair and a fresh r2 run are required; no runtime/source edits were made by this audit.
