# Heartland depth playtest

Launch `Tools/ProductionContinuation/play_heartland.cmd` from this worktree. **N** creates a fresh persistent session without deleting older saves. **C** restores the current session's last **F5** checkpoint. This remains an opt-in Heartland build; FourFactionAlpha and the retained default are separate.

The launcher verifies the selected stage and executable. Keep its console open: **30 FPS default, 85 C hard cutoff**. Short capped tests do not establish sustained thermal safety.

## Try first

1. At Human Capital, press **T / Manage**. Recruit **Infantry (140 gold)** and **Archers (180)**. Upgrade Barracks over two days to unlock **Veteran Guard (220)**. Stock and movement costs are real; days also give the AI its turns.
2. Build **Arcane Hall** (one day), **Veteran Barracks** and **Expanded Market** (two days each). Visit with **V** to see the corresponding authored buildings appear. Academy takes two days; Conclave three. Frost is Aurora's primary school; Fire/Lightning remain forbidden.
3. Build the Tavern, hire **Rowan (1200 gold)**, then use the assignment button. Rowan is physically present near the city arrival and listed as Companion or Unassigned. He does **not** yet follow you into battle.
4. Visit Crossroads Windmill, Old Quarry and Ancient Shrine. Their landmark art now matches existing site interactions. Windmill use grants 100 gold, separately from any first-capture reward. Daily use/cooldown persists; a full-mana shrine still rejects unnecessary use.
5. Press **L** for diplomacy with Dwarves, Orcs or Vikings. Gifts transfer 250 gold; accepted actions cost one movement. Acceptance reasons are visible. Peace blocks attacks and occupation in both directions, without granting military access. A pact lasts through its displayed end day, then remains at peace. Breaking it records betrayal. Wounded/captured Aurora cannot negotiate.
6. Fight at Forest Edge or Southern Crossing. Woodland has real trees/terrain; the river battle forces passage over the bridge. Human Capital combat remains the authored outer approach, **not a siege**.

## Controls

- Campaign: WASD pan; Q/E orbit; wheel zoom; Page Up/Down tilt; Home focus company; F focus selected place. Select your company, then an adjacent legal region. Space ends the day. **F5 saves; F9 restores**, including travel since the save being undone.
- City: WASD walk, Shift run, RMB + mouse look, Tab manage, Esc return.
- Battle: WASD hero movement, LMB melee, RMB guard, C commander view, J hero-only selection. In commander view, formation cards / F1-F5 select soldiers separately; F5 is not campaign save during battle. Use **Move, Hold, Follow, Charge** on the command bar. Hold/Move persist; Charge explicitly permits pursuit; **R** releases formations back to AI. K opens learned spells.

## Known limits

- Three honest roles: infantry, ranged archers and heavy shield infantry. No qualified cavalry yet.
- Company losses return by exact unit type. Archers and guards are scarce and vulnerable; balance is provisional.
- New city groups reuse authored facade buildings; specialized interiors and a complete city building tree remain unfinished.
- Dedicated site art is provisional, especially the modular quarry. Some donor materials, city collision and campaign-ground tiling still need art work.
- Field battles outside the new mapped locations retain prior coverage. Bridge crowding and broader formation pathfinding need human playtesting.
- No live siege is implemented. Diplomacy is a small Human-facing V0, not autonomous diplomatic AI. Hero capture/recovery follows the existing rules; captivity is not an automatic heal.

## Saves and exit

Human sessions persist under `Saved/CompositionPlaytest/HeartlandAlpha/HumanSessions/<session>/Saved/RBSave/Domains/`. Press F5 and wait for confirmation, then Alt+F4. Continue loads the last successful F5, not unsaved travel. New company pools in older saves begin at zero and grow under the existing weekly rules; a NEW session exposes their starting stock. Use this new launcher for saves containing diplomacy; old executables do not understand treaties. Thermal shutdown cannot preserve unsaved actions.
