# Six-faction state increment — implementation boundary

Starting HEAD: `41f604475e94905ff9afc2cb0e55148e0412618b`. Start: 2026-10-08 22:36:18 UTC. Native, rendered state, cooked save/restore and ordinary Viking battle evidence now pass; final binary/stage closeout is recorded separately.

The existing `USoulFounderPlaytestStateSubsystem` remains the only campaign adapter and `Soul.Campaign` RBSave provider. `FSoulWorldState` owns region ownership/knowledge; `FSoulWorldRules` owns graph legality/capture; `FSoulCampaignRules` owns spending/day/economy. The new strategic army value type lives in that existing SoulCore campaign domain. No AI scheduler is called.

Human army/economy remains in the existing founder fields to preserve construction, recruitment and battle behavior. Five other faction records are held by the same subsystem. The old `EnemyArmies` ledger is empty/inactive only in the six-faction profile; presentation and descriptor queries read the actual faction army. No mirrored Human state or second save authority is introduced.

The opt-in `-SoulComposition -SoulSixFactionProof` configuration directly reads the existing `six_faction_sandbox_candidate` ownership/spawn/knowledge overlay. It does not rewrite the canonical start JSON. Initial ownership stays 12 owned / 24 neutral; all 36 IDs and 51 legal pairs stay unchanged.

Starting force balance remains unresolved. For this state qualification only, Human count 45 and other faction count 30 reuse prior test magnitudes. Other faction gold starts at the same 3000 test balance; no non-Human income/recruitment is invented. These are test fixtures, not approved starting balance. All armies begin with the existing 3 AP rule. A missing physical roster is explicitly `None`, never a borrowed unit.

Save domain/schema remains `Soul.Campaign`/1. The new isolated slot adds a required profile tag, extension version, explicit unique canonical region rows, and five exact faction army/economy/knowledge rows. Human state uses the pre-existing fields. Temporary parsing validates all rows, owner identities, allowed exact unit bindings, integer bounds, region membership, AP/day, and ownership consistency before any live mutation. Legacy profiles reject the new tagged payload; six-faction rejects untagged legacy payloads. RBSave's existing selected-domain rollback remains responsible for cross-domain transactions.

The I-key inspection UI is read-only and opt-in. It shows each faction's own knowledge and field army; Home returns to the Human company. The explicit qualification observer moves armies only through existing canonical legal edges and spending/capture rules. No autonomous faction action, diplomacy or war policy is added.

Acceptance uses native tests, rendered runtime receipts and separate-process exact save restoration, documented in the handoff. Terrain, roads, art, donors, default shipping policy and other save slots remain outside this increment.
