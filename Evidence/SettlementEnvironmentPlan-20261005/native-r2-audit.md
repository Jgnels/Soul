# Native r2 audit — PASS

**51/51 exact expected tests completed with Success.** No missing, failed, duplicate or unexpected completion paths; one queue-empty marker reports51 tests performed. All nine source hashes match the frozen r2 manifest. No native assertion/fatal event appears in the audited log snapshot.

| Group | Expected | Completed success |
|---|---:|---:|
| Soul.Core | 28 | 28 |
| Soul.Integration.Vertical | 18 | 18 |
| Soul.Integration.Settlement | 4 | 4 |
| Soul.Integration.Save.SettlementDomainRoundTrip | 1 | 1 |

[native-r2-audit.json](native-r2-audit.json) contains every exact completed result and line number, queue completion, manifest/log snapshot hashes and source verification. The failed r1 attempt remains preserved.

This validates the native functional suite only. Runtime controls, the complete authored capital, matching miniature and battle environment remain separate qualifications. No source, asset or UE operation was performed by this audit.

Expected-error audit also passes: the negative loaded-binding test declares exactly two `SOUL_SETTLEMENT_DEVELOPMENT_UNREADY` messages and logs exactly two (Verbose), then completes successfully. No unexpected Error/Fatal/assertion lines appear in the audited log snapshot.
