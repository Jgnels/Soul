# RB AI 1.0.0

RB AI is a modular Unreal Engine 5.8 decision framework for tactical enemies, creatures, coordinated groups and authored bosses.

It owns **behavior selection**, not gameplay truth. RB AI scores bounded contexts, chooses an action deterministically, and emits a request for the host game to execute or reject.

## Included modules

- `RBAICore` — deterministic utility/context/consideration selection and budgeted world scheduler.
- `RBAICombat` — UE AI Perception/threat bridge and host combat request/result seam.
- `RBAICreatures` — reusable creature archetypes and needs/territory/pack signals.
- `RBAIGroups` — event-driven group support and token-based combat-pressure arbitration.
- `RBAIBoss` — deterministic boss phases, telegraphs, action sequencing and snapshots.
- `AITokenCore` / `AITokenCoreEditor` — MIT-licensed token arbitration foundation, ported for UE5.8.

RB AI does not require RB Routine, RB Combat, RB Save, Foundation, or another RefinedBadger product.
