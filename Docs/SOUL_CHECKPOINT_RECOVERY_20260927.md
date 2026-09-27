# Failed checkpoint recovery

Source review found that RB Save restored providers one at a time. A valid
campaign domain could overwrite live progress before a malformed settlement
domain rejected the checkpoint. Missing-domain selection checks alone did not
prevent this mixed-state outcome.

`URBSaveSubsystem::RestoreRegisteredDomains` now preflights selected identities,
schemas and field names, captures every selected provider's live state, then
restores. Failure restores attempted providers in reverse order, including the
failing provider. A rollback failure is explicitly appended as `RECOVERY FAILED`
to the returned error; remaining rollbacks still run. Unselected providers are
excluded throughout. No file format/schema change or second Save authority.

This applies to both selected-domain and all-domain restore. Providers must now
successfully capture before load and must be able to restore their own capture.
This protects Soul's initialized campaign/settlement pair. Generic third-party
providers with irreversible restore side effects or invalid self-captures cannot
be promised atomic recovery; the callback contract and error reporting remain
the boundary. Unreal callback execution/GC and other installed provider restore
behavior still require qualification.

Authored UE regression: `Soul.Integration.Vertical.RBSaveFailedLoadRecoversAllDomains`.
It uses real campaign and settlement providers, both registration orders, scoped
and all-domain APIs, and an older victory checkpoint after further progress.
Cases: malformed later JSON, later schema mismatch, invalid field identity,
duplicate domain identity, uncapturable live campaign, and successful load after
the failures. Assertions preserve day/AP/gold/mana/location/survivors/result,
replay protection and settlement wall integrity. **Not executed here.**

Supervisor validation: build RBSave and Soul, run this regression plus existing
vertical and settlement save tests, then perform save/play/load and failed-load
recovery across repeated battles. Verify F5/F9 reports failures and subsequent
valid operations succeed. Do not equate Python tooling checks with C++ execution.
