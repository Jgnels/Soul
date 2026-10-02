# Soul campaign and battle experience brief

This is the reusable experience target for Jeff's Soul implementation workers. Read it before changing campaign controls, physical battles, cameras, creatures, or spells. It combines Jeff's stated direction and playtest feedback with the existing gameplay reference review. It is a design and acceptance brief, not a claim that the current build passes.

## The experience we are building

Soul should let me understand my position in a believable fantasy world, select my company, make a deliberate travel or encounter decision, then personally fight and command an organized fantasy army. At every stage I should know what I control, what it can do, what is happening, and what my last action changed.

Use Total War's readable campaign decisions, army identity, formation information, and tactical overview as reference qualities. Use Bannerlord's embodied battlefield presence, formation orders, and visible maneuver as reference qualities. Soul needs a coherent control scheme of its own. The hybrid below is our design interpretation, not a claim that either reference game works exactly this way.

The decisive test: during a 90-second battle observation, can a new player identify what each major formation is trying to do without reading diagnostic logs? Can that player intervene and see a useful response?

## Campaign selection and travel

The army is a first-class selectable object even when it occupies a capital. Its marker, hit target, selected state, strength and current location must be distinct from the settlement. Provide a persistent army button and focus control as alternatives to picking a small world marker.

Selection and action are separate, legible steps. Selecting a city should explain that city; selecting the army should reveal travel choices. Show remaining movement before travel, the cost or eligibility of a destination, and the updated remainder afterward. A blocked action explains the reason. Ending the day clearly restores the appropriate movement.

Soul currently has region adjacency and action points. Preserve that authority. Present its legal destinations spatially; do not silently introduce free travel or a second movement simulation to imitate a reference game. The party can animate along the route after authoritative movement succeeds.

Settlements, roads, rivers, passes and faction standards establish geography at normal zoom. Labels support the world. Context panels expose recruitment and battle commitment without covering most of the map. Returning from a battle visibly updates company survivors, location, ownership and the result.

## Entering a battle

An encounter must provide time to understand the forces and controls before combat starts. For this slice, enter an actual paused deployment state with separated armies and a useful initial camera. The pause/start control and current state stay visible.

While paused, the player can inspect the battlefield, move the commander camera, look around the hero, select formations and assign supported orders. Soldiers, projectiles, damage, resource spending and battle timers must remain frozen. Showing PAUSED while units attack is a failure.

Do not start normal play with proof, automated casting or autobattle flags. Qualification shortcuts must never define the player experience.

## Cameras and control

Third-person hero view provides an unobstructed view of the hero and nearby action at a useful scale. Mouse look must work, including during tactical pause. Nearby soldiers must not push the camera into the hero's torso.

First-person view must look outward from a sensible eye position with no face, chest or equipment blocking the view. Switching back restores the body correctly. Commander view shows formation relationships and supports usable pan, zoom and inspection. Switching views is explicit, discoverable and reversible.

Hero movement, camera movement and formation orders must have clearly different meanings. State which mode is active. Losing the hero must not leave the player trapped in a corpse or remove all army control.

Actual test: start paused, look both directions, switch through all views, inspect a formation, return to hero view, resume and move. Repeat pause and camera switching after combat begins. A printed key legend alone does not pass this test.

## Army organization and battle flow

Keep roles and formations separate. Several unit roles can share one tactically useful formation. Roughly three to five understandable formations per side is the initial target: front line, missile/support, strike/flank and command/reserve as appropriate to the real roster.

Formation cards persist and show identity, count, selection, current order and morale. Friendly and hostile troops are distinguishable in the world. The selected formation has restrained world feedback. Players should not need to identify every miniature individually.

Hold keeps an anchor and limits pursuit. Advance moves together toward an objective. Charge commits to contact with looser cohesion. Fall Back produces an actual withdrawal. Face changes orientation. Player orders remain authoritative until deliberately released or clearly expired; the commander must not immediately undo them.

The observable sequence should allow deployment, approach, ranged pressure, engagement, maneuver, reserve arrival and eventual collapse or recovery. It can vary with composition and player decisions. Frontline troops screen support; ranged troops seek useful firing distance; strike troops attempt a flank or exploit an opening; reserves reinforce a meaningful need.

The first useful scale is about 30-40 active combatants total. Prove organization there before increasing representation. More actors, labels or visual effects do not compensate for every group running into the same pile.

An ordinary balanced encounter should allow several decisions and several minutes of readable action. This is a proposed playtest target, not a fixed duration extracted from the videos. Do not manufacture duration through invulnerability, inert AI or inflated health. A one-sided fight can end quickly.

## Morale and reinforcement

Morale has visible behavioral consequences. A damaged or overwhelmed formation may withdraw, break, flee or rally. Its UI state and physical behavior must agree. A battle can end through organizational collapse without requiring extermination.

Reinforcements physically arrive from safe, believable approaches, then join the intended formation. The player can distinguish active troops, reserves and losses. New troops must not materialize inside enemy contact.

The result must preserve the distinction between casualties and surviving routed troops according to the existing campaign contract. Inspect that contract before changing outcome accounting.

## Fantasy troops and spells

Creature identity must survive scale changes. Elephant, dragon, griffin and kraken can occupy a comparable elite-unit scale band while retaining their own proportions, footprint and silhouette. Comparable size does not mean identical dimensions.

Do not squash the elephant to force a bounding-box match. Inspect the complete body, feet, ground contact, animation, facing and collision. Use suitable animation for the assigned mesh. Kraken movement should read as its own locomotion; airborne creatures must read as airborne and turn plausibly. A declared movement archetype does not prove the visual behavior works.

Heroes need distinct, usable spells. Show available abilities, mana, costs, cooldowns, valid targeting and cast feedback. Firebolt, Chain Lightning, Blizzard, Tidal Ward and Tailwind are current candidates to verify against the actual assets and gameplay authority. Their names in a menu do not establish distinct effects.

The player should understand the result: a hit, chain, persistent area, protection or speed change. Match each effect to authorized RB Magic mechanics and the available presentation assets. Never fake success with a particle alone or spend mana automatically in ordinary play.

## Terrain and environment

Use the real Dragon Graveyard showcase environment for its intended proof. Place the battle and camera where the pack's distinctive geography and landmarks can be experienced, not an arbitrary patch of ground that obscures the setting. Author useful approaches and firing positions, then prove that formations can navigate them.

The campaign should read as one coherent world. Preserve the human plains, surrounding dwarven mountains, northern lake and evil region's water access from Jeff's accepted direction. Bridges should cross plausible short spans. Terrain and settlements should explain travel routes.

Licensed donor assets remain intact. Work in the authorized isolated branch and use existing approved references or mounts. No donor reserialization, source-owned asset claims, canonical merge or push is implied by this brief.

## Known failures from Jeff's playtests

These are reported failures, not completed fixes:

- Army selection competed with the capital; movement allowance was unclear.
- Kraken and griffin kept the same facing while moving.
- Elephant initially showed only tusks; a later view showed very short, squat legs moving toward combat.
- Battle was too distant to read, control was unclear and it ended before useful decisions.
- Formation selection, allegiance, spells, pause and first-person access were not discoverable.
- A later screenshot showed the camera inside a hero's chest and casualties while the HUD displayed paused deployment.
- Jeff could not move the camera in that state.

Treat these as end-to-end acceptance cases. Verify them through the normal launch and campaign encounter path, not only a special proof scenario.

## Evidence required before another ready claim

1. Normal campaign launch: select army beside its capital, inspect movement, travel, recruit and save/load.
2. Campaign encounter enters the real battlefield paused with zero unsolicited combat progression.
3. Hold deployment for at least 30 seconds; prove unchanged health, casualties, positions, resources and timers while camera inspection and selection work.
4. Exercise hero, first-person and commander views through real input in paused and running states. Inspect screenshots for clipping, scale and readability.
5. Select one formation, issue different orders and observe their distinct spatial results. Show that manual orders survive commander evaluation.
6. Play long enough to observe front line, ranged support and a maneuver element doing different jobs.
7. Inspect each large creature moving and turning at close and tactical zoom. Check the full elephant body and undistorted proportions.
8. Cast and inspect supported spells, including mana/cooldown/target validation and their distinct actual effects.
9. Observe physical reinforcement arrival, morale change and resolution when the scenario exercises them.
10. Return to campaign and verify survivors, resources, encounter identity and ownership. Run the relevant build and regression tests.

Capture a short sequence of before/action/after evidence; one attractive still cannot prove control, pace, animation or tactics. Report exactly which checks passed, failed or remain untested. Keep diagnostics available for developers without making players rely on them.

## Source and evidence limits

The existing review note records complete transcripts and storyboard sampling, with denser inspection around commands, camera changes and battle phases. This is temporal reference analysis, not literal uninterrupted human-rate viewing. This brief does not claim exact reference map dimensions, timing benchmarks or proprietary implementation details.

Reference recordings recorded in that review:
- [Karl Franz opening walkthrough](https://www.youtube.com/watch?v=exMyDXLEsN4)
- [Bannerlord campaign opening](https://www.youtube.com/watch?v=CMxEENNl6kw)
- [Bannerlord battlefield commands](https://www.youtube.com/watch?v=QTxrRzv0ncw)

Local provenance: [Video reference findings](VideoReferenceFindings.md). Current priorities come from Jeff's explicit requests and playtest reports. Future workers must inspect the live branch and implementation; this brief is not a substitute for current evidence.

## Handoff instruction for another worker

Read this brief, the repository AGENTS.md and current git state. Identify the smallest change that resolves the next failed player experience. Reuse Soul and RB authority. Exercise the actual input-to-outcome loop in Unreal, inspect the result, and preserve concise evidence. Do not mark an experience complete because a method, hotkey, label, asset reference or automation test exists.
