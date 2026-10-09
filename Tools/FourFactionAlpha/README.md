# Playable four-faction alpha

Opt in with `-SoulComposition -SoulFourFactionAlpha`. Humans are the player;
Dwarves, Orcs and Vikings act once each after Space/Next day. Nature and Dark
remain canonical, persistent, passive owners. The retained/default campaign
and earlier qualification profiles are unchanged.

Use the guarded cooked launcher after the milestone stage passes:

```powershell
python Tools/ProductionContinuation/play_candidate.py --four-faction-alpha --stage-receipt <verified-receipt.json> --evidence-root Evidence/FourFactionAlpha-20261009 --minutes 20
```

This launches interactive play, not qualification automation. Home selects the
Human army; click a legal destination, Space ends the day, T opens the town,
B starts a selected encounter, F5 saves and F9 restores. WASD pans; wheel zooms.
The launcher keeps the 85°C cutoff and a capped, bounded session. Save with F5
before leaving. Its persistent directory is `Saved/CompositionPlaytest/FourFactionAlpha`;
the slot is `Soul.Composition3500.FourFactionAlpha`. Previous proof saves are not loaded.

## AI rules

The existing controlled-action admission API validates every selected move,
capture, encounter or recruitment immediately before execution. No direct
ownership writes, added AP, free recruits, forced results or roster substitution.

Each faction considers visible adjacent destinations and explored routes toward
a visible frontier or its own supply settlement. It excludes passive owners.
Defended attacks need at least 85% of the visible defending troop count. Priority
scores are: urgent recruitment 350, immediate capital defense 300, weak-army
return to supply 220 minus distance, hostile opportunity 160, routine recruitment
130, neutral capture 100, frontier movement 30 minus distance. A seed/day/faction/
destination tie-break is stable; physical RBCombat outcomes are not forced.

Recruitment uses the existing finite infantry pool, real unit cost and one AP,
up to four recruits per action at an owned settlement. Starting income and pool
values match the existing Human campaign economy; these are provisional balance.
The profile has one field army per faction and no invisible garrisons. An empty
hostile territory can be occupied by a legal paid move. A surviving army can
return to a settlement and replenish. A defeated army record may withdraw one
legal edge per action through owned regions, like the defeated Human commander.
It cannot capture or attack with zero troops. Rebuilding requires an owned
settlement, real resources and finite pool stock. No remote recruitment or teleport.

AI-only encounters use the existing embodied Autobattle/RBCombat path. The player
observes and cannot command either army. AI attacks on the Human field army enter
normal tactical defense with the Human hero, formations and mana. Battle return
preserves GameInstance state without overwriting the player's manual F5 checkpoint.
When no regional recipe matches, this profile explicitly permits the same qualified
Dragon Graveyard field fallback already available to Human attacks. Exact rosters,
AP and result authority are unchanged. This is provisional encounter scenery, not
a claim of regional battlefield or siege-art completion; directed approaches remain
unchanged. Earlier controlled qualification profiles retain their strict rejection.
Occupation pauses Human construction/services at the captured capital while
preserving its building state. Ending a day during a city visit returns to the
campaign for the same AI scheduler.

## Qualification

`qualify_alpha.py` runs capped editor/cooked gameplay with separate user/save
directories. `--alpha-defense --target-day 2` uses an explicitly isolated 24-troop
Human starting-force fixture, three normal Human moves, then the real AI selector.
`--alpha-attack --target-day 3` instead keeps the ordinary 45-troop Human start,
uses legal movement and the actual B-input handler to attack the Orc army, then
checks natural return and F5/F9 in its separate `.AttackProof` slot.
`cold --source <run>` restores the real save in a new process and continues play.
`summarize_campaign.py` reports observed actions, natural results, resource changes
and save continuity. It never supplies campaign state or outcomes.

`stage_binary_update.py` creates a new local stage from a verified unchanged cook
and a fresh monolithic executable. It preserves prior stages, checks dependency
and cooked hashes, privately copies mutable metadata and retains the 8 GiB staging
reserve. It is not a new asset cook, default-map promotion or distribution archive.
`--compress-executable` can losslessly NTFS-compress that new private executable
before copying the rest of the stage. The 8 GiB floor and before/after executable
hash checks remain mandatory; prior stages and linked content are not compressed.
