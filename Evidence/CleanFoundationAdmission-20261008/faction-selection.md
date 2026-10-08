# Bounded next-faction selection

Read-only selection began 2026-10-08 19:44:18 UTC, while clean-source builds occupied the sole UE lane. No faction implementation is enabled before the clean-source runtime gate.

**Selected candidate: Orcs, one explicit hammer-infantry unit.** The primary Soul donor library contains `Fantasy_Pack/Characters/Orc_Hummer`; all 21 packages are already present in Soul and byte-identical to AoEAssetRenderLab. This includes the actual skeletal mesh and rig, separate hammer, idle/run/walk, three attacks, hit reaction and death. Nine direct roots are already in the qualified isolated cook. Existing native animation tests exercise this rig and its `CATRigRArmPalm` hammer attachment. These are strong readiness signals, not a new faction runtime PASS.

The current renderer also references Viking Ulf and other Fantasy characters, and the primary library contains Nature-oriented Paragon families. They have no already-admitted exact campaign matchup. Orc wins because Jeff explicitly prefers it when ready, its full dedicated combat set is local, its attachment path already exists, and it can be bound without retargeting or a new combat system. Dark/Nature/Viking broader rosters remain unimplemented rather than inferred from an environment or a hero asset.

## Exact identity boundary

Do not simply set the old Evil roster flag. That branch mixes Orc, Barbarian, Viking, Troll and other presentation roles, and can be selected from a region name. The new proof must bind the exact Orc faction/unit pair to the actual Orc mesh, hammer and native clips for initial troops and reserves. Unsupported faction/unit pairs remain rejected. RBCombat continues to own accepted health/damage; existing formations, reinforcement and campaign-result handling remain the authorities.

## Environment scope

Available approved local candidates include the WarCamp Collection overview, Ravenhold fortress kit demo and the already-qualified Dragon Graveyard field. The first two are recorded as camp/occupied-fortress candidates, not proved battle-ready environments. Prefer the existing Dragon Graveyard hostile field for this narrow roster/bridge proof unless bounded inspection identifies a better already-qualified location. This does not assign Dragon Graveyard to every Orc encounter or promote a new capital. No third authored-city integration is authorized here.

`orc-source-inventory.json` records exact donor/local hashes and already-cooked roots. Native asset compatibility, actual roster rendering, natural result, return and persistence must still be qualified after implementation.

## Narrow regression boundary

The proposed Orc override applies only to the new exact campaign pair and side. The historical mixed Evil/region heuristic for existing fixtures is not globally rewritten in this mission: doing so would change previously qualified default battles. It remains presentation debt for a separate roster audit. The new Orc proof bypasses that fallback for every requested formation role, native animation and weapon, and normalizes its deployments/reserves to the one admitted infantry role. No claim of a complete seven-family Orc roster is made.
