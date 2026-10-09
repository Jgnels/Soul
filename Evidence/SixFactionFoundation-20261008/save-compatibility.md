# Six-faction qualification save boundary

The existing `Soul.Campaign` and `Soul.Settlements` RBSave providers remain authoritative. Their domain schemas remain unchanged. No new persistence subsystem or slot migration is introduced.

The opt-in `-SoulComposition -SoulSixFactionProof` profile uses `Soul.Composition3500.SixFactionProof`. It adds a required profile marker, extension version 1, an explicit unique 36-region/owner list, and five non-Human faction records to the existing campaign payload. Humans continue using the existing player army/economy fields; there is no second persistent Human copy. All six share canonical graph/rules. Neutral ownership is preserved as None.

Retained, Founder, HumanProof and OrcProof slots remain unchanged. The exact new Viking encounter uses `Soul.Composition3500.VikingProof`. Nine-region saves are rejected, not migrated. A six-faction payload cannot restore into a legacy profile. Parse/validation completes in temporary state before assignment, and actual RBSave cross-domain rollback is covered by native tests.

Earlier saves produced during this mission stored the Viking strategic army with no admitted combat unit. Those saves retain None exactly after Viking infantry admission. They remain battle-ineligible until a future explicit game action or reviewed migration changes the roster. New six-faction games assign the newly admitted Viking unit. Nature and Dark remain unbound and cannot substitute another army.

Qualification forces (45 Human, 30 each other faction), 3 AP and starting resource balances are test fixtures, not accepted six-faction balance. AI, diplomacy and non-Human recruitment remain off/unimplemented. Camera pose persistence is not claimed.

The separate Viking matchup fixture uses the existing two-side encounter rules; its automated battle proof does not advance strategic days or exercise strategic AI. The six-faction sandbox explicitly disables the existing enemy-AI turn handler. No full six-faction AI is enabled by either profile.
