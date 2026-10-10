# Human Heartland playtest

Use `Tools/ProductionContinuation/play_heartland.cmd` from this worktree. Choose **N** for a new Heartland campaign or **C** to restore that session's last F5 checkpoint. This is separate from the existing FourFactionAlpha launcher and saves. The launcher validates its private stage before starting; 30 FPS default, 85 C hard cutoff, no automated play inputs.

Start by moving to Crossroads, using its site action, and pressing F5. Exit and Continue: the company should restore at Crossroads. F5 reports the saved day and region; travel after that checkpoint is not saved until another F5.

At Human Capital, use **T / Manage** for recruitment and construction, **V / Visit** to enter the authored city. Build Arcane Hall (300 gold, one day) to learn Frost magic. Mage Academy takes two days; High Conclave takes three. Buildings require their prerequisites and real resources. Only one new construction order per day. Their new physical building groups are not authored yet.

- Campaign: WASD pan; Q/E orbit; wheel zoom; Page Up/Down tilt; Home focus company; F focus selected place. Click your company, then a legal destination to travel. F5 save, F9 restore.
- City: WASD walk, Shift run, RMB + mouse look, Tab manage, Esc return. The original city has decorative clutter and imperfect collision; this is an early walking pass.
- Tactical: WASD hero movement, LMB normal melee, RMB guard; C toggles commander view; J selects the hero alone. F1-F5 select troop formations without selecting the hero. R restores autonomous orders. Use the existing HUD for movement/hold and learned spells.

A wounded hero is unavailable for three days if the army survives. A wounded hero whose army is destroyed is captured; release/rescue is not implemented. Captivity does not silently heal. Human recruitment remains one infantry type. The Dwarf commander is the first enemy-hero integration, not a complete enemy-hero roster.

Three site effects exist at Crossroads, Old Quarry and Ancient Shrine; dedicated site art is still missing. Human Capital defense uses the authored city's outer approach, not a complete siege or street battle. Other locations still use existing battlefield coverage. Hired tavern companions do not yet have a complete visible battle-participation implementation.

Saves: `Saved/CompositionPlaytest/HeartlandAlpha/HumanSessions/<session>/Saved/RBSave/Domains/`. Save with F5, then Alt+F4. Keep the launcher console open for the thermal guard. A thermal stop cannot save later unsaved travel automatically.

Functional checks ran at 10 FPS. The full-city walking run peaked at 84 C, below the unchanged 85 C cutoff; this is **not** a sustained 30-FPS performance pass. The human launcher remains capped at 30 FPS by default. Let the worker cool before playing; city visits may still reach the guard.
