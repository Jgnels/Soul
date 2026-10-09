# Controlled-action rejection coverage

Evidence authority: `final-native-results.json`, with eight passing tests in `Local/final-six-native`. Assertions are in `Source/Soul/Private/Tests/SoulSixFactionCampaignTests.cpp`. These are native arranged-state tests, not natural battle outcomes.

| Requirement | Passing native coverage |
|---|---|
| Malformed faction, missing army, wrong owner, invalid/stale region | `ControlledActionAdmission`: preparation rejects, clears its output, reports a reason and preserves the exact campaign snapshot. |
| Nonadjacent destination / insufficient AP | `ControlledActionAdmission`: normal SoulCore edge and AP rules reject before mutation. |
| Stale profile / restore revision | `ControlledActionAdmission`: proposals cannot cross profile or load boundaries. |
| Destination owner or other campaign state changes | `ControlledActionAdmission`: execution revalidates authority and snapshot; no stale capture or spend. |
| Off-thread calls | `ControlledActionAdmission`: actual thread-pool prepare/execute calls reject before reading live state. |
| Replayed action | `ControlledActionAdmission`: normal movement/capture invalidates the old proposal. |
| Unsupported pair / unbound Nature or Dark | `ExactBattleRejection`, `ControlledReverseBattleConsequences`, `ControlledNonHumanPairs`: explicit pair admission, old unbound Nature save and absent Dark roster remain enforced. No silent roster upgrade. |
| Recipe disappears after preparation | `ControlledReverseBattleConsequences`: execution rejects and preserves campaign state. |
| Bridge becomes occupied after preparation | `ControlledReverseBattleConsequences`: campaign and existing pending encounter remain intact. |
| Battle-side result mapping | `ControlledReverseBattleConsequences`, `ControlledNonHumanPairs`: arranged attacker win/defeat verify owners, survivors, AP, return, untouched unrelated armies/Human hero, restoration and duplicate-result rejection. Natural results are separate. |
| Malformed save / topology mismatch | `AtomicAdversarialRestore`: rejects malformed state and 9-to-36 or reverse profile mismatch without mutation. |
| Cross-domain restore failure | `RBSaveCrossDomainRollback`: campaign and settlement domains roll back together. |
| Six factions without autonomous strategy | `CanonicalOwnershipAndControlledMovement`, `ControlledActionAdmission`: ordinary moves and neutral captures; enemy AI does not mutate this opt-in profile. Rendered turn receipts are separate. |

Rejection atomicity means exact serialized campaign-domain equality around the action, plus pending-bridge identity where applicable. Cross-domain tests separately check settlement preservation. It does not mean arbitrary engine memory is byte-identical.

The executor conservatively requires a living exact defending army and a geographically matching playable recipe for hostile actions. Pair admission alone does not authorize every hostile region. No automatic war declaration, target scoring, recruitment or autonomous scheduler was added.
