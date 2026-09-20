# RefinedBadger Combat 1.0.0

Core: host-issued `FRBCombatantRef`, `FRBCombatHit`, bounded `FRBWeaponProfile`,
`IRBCombatConsequenceSink`, `IRBCombatWeaponProvider`, `FRBCombatRouter`.
Only Unreal Core is required. No game simulation or Actor dependency.

Expansion: `FRBWeaponDefinition` and
`FRBCombatAction` supply configurable action timing with notify-only contact;
`FRBRangedDraw` and `FRBProjectileFlight` supply bounded ballistic segments;
`FRBCombatGroup` supplies stable membership and revision-checked leader orders.
These helpers do not by themselves provide animations, AI navigation or UI.

`URBCombatRangedComponent` connects draw/release input to
`ARBCombatProjectile` physical sweeps. The host must provide an equipped Bow
profile and `IRBCombatResourceAuthority` for atomic inventory/effort spending.
Blueprint hosts can call `ConfigureBow` with an `RB Combat Bow Settings` struct
for ammunition type, draw timing, speed, gravity, lifetime and collision radius.
`IsDrawReady`, `GetDrawProgress` (0–1) and `GetAmmunitionId` are Blueprint-readable.
Configuration rejects invalid/non-finite values and changes during an active
draw without replacing the prior profile. Cancel or release before retuning.
These settings contain no ammunition quantity, fatigue or health authority.
Projectile impact uses a launch-time source/profile snapshot and native
PointDamage/AnyDamage; the source Actor need not remain alive. Callbacks report
native accepted damage; Blueprint-local damage transformations remain subject
to the guard integration limitation below. A host rejection after native damage
cannot roll native state back. The producer records attempted contact IDs to
prevent a retry from applying native damage again (128 transient IDs per victim).
The isolated host has qualified physical projectile guard/bypass/hit/miss/wall/
no-ammunition cases and a separate-process save/reload. The distribution includes an original receiving-game training fixture. Its primitive
presentation demonstrates the native impact adapter, bow, guard, group orders and
host-owned persistence; integration with your art, input and authority still needs
qualification. See the quick-start guide and the accompanying release gate report.

Variant: subclass `URBVariantCombatBindingComponent` in your game. Override
`GetConsequenceSink` and `GetWeaponProvider` with lifetime-safe host accessors.
Register one binding per Actor, then bind an existing host identity. Bindings
cannot change identity. Equipment truth remains in the game. Bind both actors
to the same consequence authority, in the same world.

For Blueprint games, create a Blueprint subclass of `RBCombatBlueprintBinding`
and add it to each participating Actor. Override `HostIdentityExists`,
`HostReadEquippedWeapon`, `HostCommitAcceptedHit`, `HostSpendResources`,
`HostCanAct`, `HostCanReceiveDamage` and (when used)
`HostHasEligibleGuardEquipment`. Call `BindExistingIdentity` with an identity
already created by the game. Every unimplemented host decision rejects by
default. The component stores no health, ammunition or identity registry.
Validate a hit/resource transaction before any host mutation, then commit once
and return true. These synchronous functions cannot provide rollback for a
partially changed host. Receiving damage is separate from acting so a game's
incapacitated body may remain damageable while unable to attack.

Author a `RBCombatWeaponData` asset with local blade endpoints, sweep radius
and attack montages. Each montage needs an `RB Combat Contact` notify state
covering the actual contact interval. Configure `RBCombatMeleeComponent` with
the data, skeletal mesh and attached weapon geometry; route input to
`RequestAttack` or `RequestThrow`. Combo entries use the same attack type and
successive ComboStage values starting at zero. A short buffered input starts
the next configured stage. Do not also run another damage trace for that swing.
Throw release consumes one owned weapon through the host resource transaction.

For groups, subclass `RBCombatGroupDriver` in the host and supply persisted
group membership/orders, participant state and opponent eligibility. Register
the current Actor bindings with `SetRepresentations`. AI pawns use their
AIController and a host navmesh. Open arenas may explicitly enable direct
steering, which was used for the 8v8 proof; that option does not qualify complex
terrain navigation. Player-controlled members are left to player input.

The observer subscribes to `OnTakeAnyDamage`, resolves a distinct bound attacker
from causer, its instigator/owner, then instigating controller pawn. A valid
weapon provider is required. It does not emit damage or own health. Native
misses do not create evidence. Environmental damage without a person source
is ignored; the host may handle it independently.

Explicit `BeginNativeAttackContact` / `EndNativeAttackContact` adds one-victim-
per-attack semantics. Without it each accepted callback gets a new GUID, as in
the physically qualified donor. Epic must emit only one callback per physical
contact in that mode. Core deduplication covers 128 recent successful
contact/attacker/victim keys; it is transient, not a durable global event ledger.
Active contacts keep a separate bounded victim set; overflow fails closed.
Recreating an attacker binding or replay beyond retention is outside this
guarantee. A rejecting sink must leave host truth unchanged. No rollback of
arbitrary host mutations is possible inside the core.

AnyDamage supplies no exact point/normal. The observer marks location as an
estimate, supplies no witnesses, bleeding or stagger, and uses the accepted
callback amount without substituting weapon base damage. Metadata is descriptive.

Optional guard now filters plugin-produced impacts before native PointDamage,
using supplied guard intent, host equipment/participation and planar source
direction. Defaults are 80% reduction and a .5 facing dot. Hosts provide shield
eligibility and may opt into an atomic effort cost. The frozen Gladiator donor
at `74d6873` was inspected read-only; its migration still requires separate
qualification. In particular,
`OnTakeAnyDamage` receives Unreal's damage-event amount; if a Blueprint changes
damage later inside its handler, this observer cannot assume that changed amount.
Do not advertise shield compatibility until event ordering is physically checked.

## Network authority boundary

RB Combat 1.0.0 deliberately does not own transport or replication. The host invokes combat on its authoritative simulation path, supplies stable combatant/equipment/resource truth, commits accepted consequences atomically, and replicates resulting game state using the host/Foundation networking model. Plugin projectiles are local execution helpers and are not replicated actors. This keeps Combat usable in single-player, listen-server, dedicated-server, and custom networking architectures without creating a second durability or replication authority.

A multiplayer game must qualify its own RPC/authority routing, presentation replication, late-join state, and consumer animation/art integration. RB Combat does not claim those host-specific behaviors merely because the core combat contract is qualified.

No vendor content is distributed. Epic Variant template content and its
GameplayStateTree/EnhancedInput dependencies belong to the host. The generic
plugin does not force StateTree on games that do not use that template.

Install from the Lineage extraction tools:

```powershell
& .\tools\Install-RefinedBadgerCombat.ps1 -Project 'C:\isolated-host\Game.uproject' -EnableVariantDependencies -DryRun
& .\tools\Install-RefinedBadgerCombat.ps1 -Project 'C:\isolated-host\Game.uproject' -EnableVariantDependencies
& .\tools\Install-RefinedBadgerCombat.ps1 -Project 'C:\isolated-host\Game.uproject' -Uninstall -DryRun
```

The installer copies only manifest-listed code/descriptor/readme files. It hashes
each copy, records original project bytes in `.rbcombat-install.json`, preserves
unrelated JSON properties and refuses unmanaged or modified plugin files.
Identical installs are no-ops. Use `-Upgrade -DryRun` to review an explicit
managed upgrade, then `-Upgrade` to apply it. All old managed hashes must match,
project configuration must be unchanged and new paths must not collide with
unowned files. The installer stages both versions under `.rbcombat-backups`,
retains recovery copies and restores prior files/receipt if applying fails.
Only removed manifest-owned source is retired; generated/unowned files remain.
Keep the editor closed for that target while changing its installed source.
An upgrade is source deployment, not runtime acceptance: rebuild and qualify
the target afterward. Keep receipts and recovery folders local, out of Git.
Uninstall refuses if project configuration or managed source has since changed;
restore/reconcile those changes explicitly rather than overwriting other work.
Generated files are left alone. Remove empty directories manually after review
before reinstalling. Never install into another active agent's checkout.

`Tools/Test-RefinedBadgerCombatInstallation.ps1 -Project <game.uproject>` provides
a read-only audit; add `-AsJson` for a structured report. It explicitly leaves
host build, bindings, physical combat, cold reload and packaged acceptance as
not run. Matching source files alone never qualify a game integration.

Add Core/Variant module dependencies to your host module when subclassing.
Implement host-local atomic injury/bout/raid consequences and persistence.
Compile and run `RefinedBadger.Combat` automation, then a real native combo and
a separate process save/reload test. Compilation alone is not integration proof.

Bind the victim component's `OnAcceptedContact` event for cosmetic hit feedback.
It fires only after a successful host consequence transaction, with accepted
damage, impact location, weapon category, exact-location flag and contact ID.
Rejected/replayed contacts do not emit it. Do not deduct health again in this
event. Approximate native observations have an inexact location; use the flag
before placing surface effects. Full guard absorption uses `PresentGuardResult`
instead of an accepted positive-damage event. Projectile `PresentImpact` still
means a physical surface collision, including scenery, not an accepted injury.
