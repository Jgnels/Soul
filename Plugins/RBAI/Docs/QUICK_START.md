# RB AI quick start

## 1. Install

Copy the `RBAI` plugin folder into `<YourProject>/Plugins/RBAI`, regenerate project files if needed, and enable **RB AI** in Unreal Engine 5.8.

The plugin has no required RefinedBadger product dependency.

## 2. Add a brain

Add `RB AI Brain Component` to an Actor. Assign an `RB AI Profile` Data Asset or supply runtime action overrides.

A profile contains action tags, priority groups, base scores, inertia, required/blocked tags, and considerations. Contexts carry candidate Actors/locations/tags/signals.

## 3. Supply facts

Use `SetSignal`, state tags, and `UpsertContext` to supply bounded host facts. The AI Perception bridge can translate UE sight/hearing/team stimuli into tactical contexts without replacing UE perception authority.

## 4. Execute actions

Bind `OnActionRequested`. The host performs movement, combat, animation, inventory, or other consequences and calls `NotifyActionFinished` when the request completes or is rejected.

RB AI never applies damage or mutates inventory from the core decision loop.

## Scheduler

Brains do not require per-agent Tick. `RB AI World Subsystem` evaluates registered brains through a shared frame budget and distance-based intervals. Defaults are 0.5 ms and at most 64 evaluation attempts per frame; both are configurable.

## Creatures and groups

`RB AI Creature Behavior Component` maps externally owned health, hunger, fatigue, territory and pack-support facts into the standard creature action set. `RB AI Group Coordinator Component` supplies pack support and token-gated combat pressure without owning squad formation or damage.

## Bosses

Use `RB AI Boss Profile` plus `RB AI Boss Encounter Component` for authored phases and telegraphed action sequences. Snapshot structs are deliberately host-serializable; your save system owns when and where they are persisted.

## Optional RB Combat integration

`RBAICombat` emits host requests and accepts results. If your project uses RefinedBadger Combat, keep RB Combat authoritative for formation/order feasibility, hit validation, damage, guard, projectiles and consequences. RB AI decides tactical preference; RB Combat executes allowed combat behavior.
