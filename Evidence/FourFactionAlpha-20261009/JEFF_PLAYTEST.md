# Jeff's Soul playtest

## How to launch

Double-click **PLAY SOUL FOUR FACTION ALPHA** on the Desktop, or run `Tools/ProductionContinuation/play_four_faction_alpha.cmd` from this worktree.

- **N — NEW:** starts a clean Human campaign in a new session. Every older save stays intact.
- **C — CONTINUE:** restores the current Human session's last **F5** save. Save once before expecting Continue to work.
- The launcher checks the executable, stage manifest and campaign assets before starting. It sends **no gameplay inputs**.
- **Default: 30 FPS.** The single 40 FPS check hit the **85°C cutoff** after about 97 seconds. The optional `--playtest-fps 40` target is not recommended on this machine; `--playtest-fps 30` selects the fallback explicitly. Never uncapped.
- The 30 FPS startup completed three minutes at **82°C peak**. Margin remains narrow: save frequently. This does not establish safe sustained play.
- Keep its console open: it owns the thermal guard. Sessions are limited to two hours; save before that limit.

## Controls

**Campaign:** Home selects/focuses your army. Click a highlighted legal destination. WASD pans; mouse wheel zooms. Space ends the day and lets Dwarves, Orcs and Vikings act. T opens services at your owned capital; 1 recruits Knights using gold and stock. B starts the selected encounter. Escape closes panels. **F5 saves; F9 restores the last save**—later progress is discarded. Wait for save/load feedback before continuing.

**Battle:** use the tactical HUD's displayed movement, formation and spell controls. You control Humans even when defending. AI-only fights run automatically; do not mistake their loading/battle interval for a frozen campaign.

## What to test first

1. Spend 5–10 minutes moving, recruiting and ending days without developer help. Note any unclear destination, control or feedback.
2. Read the turn recap: can you tell who moved, captured, recruited or won? Is waiting for AI battles tolerable?
3. Save, take another action, restore, then continue. Later, exit and use **C** to check cold restoration.
4. Try a Human battle. Judge tactical control/readability and whether defeat explains your actual recovery options.

## Known issues

This is an alpha: AI-only battles can take 1–2 minutes plus loading; undefended territory can trade back and forth; economy/balance and several map representations remain provisional. Nature and Dark are passive. A zero-troop commander may be genuinely stranded; no free troops, teleport or guaranteed recovery are supplied. The HUD now identifies occupied-capital/unavailable-recruitment states. A sustained thermal/performance pass is **not** claimed.

## Where saves persist

`D:\RefinedBadger\Worktrees\Soul-bannerlord-campaign-map-20260929\Saved\CompositionPlaytest\FourFactionAlpha\HumanSessions\Jeff-<timestamp>-<id>\`

The launcher prints the exact active directory. `HumanSessions/current-session.json` selects Continue. Within that session, the save is `Saved/RBSave/Domains/Soul.Composition3500.FourFactionAlpha.domain.rbsave`. Prior qualification saves are separate and unchanged. New does not automatically save; press F5.

## How to exit safely

Finish the current turn/battle, press **F5**, wait for save confirmation, then **Alt+F4** the game. Keep the guard console open until the game closes. If the guard stops at 85°C, let the GPU cool; use the 30 FPS option. Only the last completed F5 save is guaranteed to survive a cutoff.
