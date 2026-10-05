# Next integration pass: campaign and battle handoff

This pilot proves Clifftop import, streamed spawn, physical terrace traversal, Soul navigation, and existing RBSave restoration. It does not certify Dragon Graveyard travel, battle, reinforcement or return. No combat/campaign rule code was changed.

Use the existing path, not a second battle bridge:

1. Add one Soul-owned encounter marker on the qualified terrace. Associate it with an existing admissible campaign region rather than inventing a separate encounter/save schema.
2. Delegate admission to `USoulFounderPlaytestStateSubsystem::BuildBattleDescriptor` / `BeginBattle`. These already select the admitted battlefield recipe, reserve campaign action/state, and call `USoulCampaignBattleBridge::BeginEncounter`.
3. Preserve the configured `dragon_graveyard` fallback and `SoulRealtimeArenaGameMode`. The current scenario uses `/Game/Dragon_graveyard/Level/L_showcase_level` and returns to `/Engine/Maps/Entry` (`Data/soul_vertical_scenario_20260925.json`). Follow `ASoulFounderPlaytestCampaignActor::StartBattle` for the current `OpenLevel` options. Do not move Dragon Graveyard assets into the pilot as part of marker wiring.
4. Decide the return destination explicitly. The existing result path reads `ReturnMapPackage` but forces `SoulFounderPlaytestGameMode`; pointing that map at this traversal pilot would not restore its current pawn/controller automatically. First exercise return to the existing canonical campaign world, or make a narrowly reviewed Soul-owned return-mode change with tests. Do not hide this mismatch behind a new gameplay authority.
5. Exercise both outcomes with reinforcement participation. Confirm `USoulCampaignBattleBridge::ResolveEncounter`, `ApplyBattleResult`, exactly-once survivor/mana/action consequences, and RBSave reload after returning.

Before treating that travel loop as accepted, cover the known reliability concerns in their dedicated lane: failed travel can leave `PendingBattle` set, and interactive `FinishProof` currently requests process exit. Neither path was entered by the Titan traversal pilot, so this pass intentionally does not rewrite them.

The donor remains a raised architectural town grid without Titan landscape. The qualified connected terrace is around Z=2250 cm. Other terraces and the staging floor are not certified as one connected campaign traversal network. Add only needed Soul terrain/ramp connections and re-run navigation/collision tests before attaching broader campaign geography.

Do not widen to Castle/Grassland/Arctic wholesale. Reuse the strict native closure pipeline for one bounded candidate at a time after the Clifftop campaign return decision. Sulfur remains stopped on its documented character dependencies and missing VFX texture; it cannot be unlocked by importing those character branches.
