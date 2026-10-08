# Save compatibility decision

The retained founder campaign continues to use `Soul.VerticalCampaign`. The opt-in 36-region presentation now uses `Soul.Composition3500.Founder`; the Human authored proof uses `Soul.Composition3500.HumanProof`.

All slots are owned by the existing RBSave subsystem, with unchanged `Soul.Campaign` / `Soul.Settlements` domains and schema versions. No second persistence system is introduced. Composition no longer risks overwriting the retained campaign slot when users launch both profiles under the same UserDir.

No automatic 9-to-36 migration is admitted. Existing region-count, ID, ownership and state validation remains intact. A future explicit migration would have to preserve the original slot, transfer both domains atomically, define new-region ownership and qualify rollback; it is not necessary to preserve the playable reference today.

Earlier isolated composition qualification saves still exist in their original evidence UserDirs. They are evidence, not user saves to copy automatically. A manually supplied old checkpoint may be copied to a NEW slot only as an explicit qualification fixture, never silently adopted by normal startup.

Verified regression: fresh native CampaignWorld tests pass in default and composition modes (2/2 each), including isolated slot selection and rejection of a nine-region restore without changing live campaign state. Normal input/F5/F9, independent fresh-process F9, and the existing two-domain construction controls all pass against the freshly built editor binary. The actual files are `Soul.Composition3500.Founder.domain.rbsave` and `Soul.Composition3500.HumanProof.domain.rbsave`. See `save-regression-results.json`. Full authored-city/battle work was not rerun for this slot-routing change. The new cooked boundary now also passes normal input/F5/F9, independent fresh-process F9, Human authored construction/visit/battle/return and final exact two-domain restoration. See `packaged-runtime-results.json`. This additional authored run was required by the first real cook/stage, not by a gameplay redesign.
