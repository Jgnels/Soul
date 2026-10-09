# Soul playable four-faction campaign alpha

Branch: `codex/soul-bannerlord-campaign-map-20260929`.
Starting HEAD: `ba4d03a00394d81eb0338353f1497c10b0588c1a`.
Implementation commit: `6d4887808dfabdb32414f27f3204011569676e3d` — Add opt-in playable four-faction campaign alpha.
Final evidence commit/ending HEAD: see `final-head.txt` (written after the final commit).
Remote remains `4842eeb403f6b9be6effdf9fe18fc5c24b499887`. No push or merge.
Started 2026-10-09 16:01:06 UTC. All requested phases completed, including the second seed and extended cold continuation; no extra faction/matrix/art work was added to fill time.

## Product result

| Gate | Result |
|---|---|
| PLAYABLE FOUR-FACTION CAMPAIGN ALPHA | PASS |
| HUMAN DEFENSE AGAINST AI | PASS |
| AUTONOMOUS DWARF/ORC/VIKING TURNS | PASS |
| AI-vs-AI BATTLES | PASS |
| AI RECRUITMENT / CAMPAIGN SUSTAINABILITY | PASS |
| 20-TURN CAMPAIGN | PASS — two seeds |
| MID-CAMPAIGN F5/F9 | PASS — exact day-10 restoration after continuing to day 13 |
| COLD RESTORE | PASS — exact restore plus continued gameplay |
| COOKED PLAYABLE ALPHA | PASS |

These are functional alpha gates, not final balance, final art, hardware-input certification or sustained-performance qualification. Human control was exercised through actual Unreal input/controller handlers, possession, HUD commands and magic authority, with native rendered evidence. The normal interactive launcher also passed a separate 60-second startup observation with no qualification automation. Desktop mouse/gamepad replay was unavailable because the computer-use kernel failed initialization; Nwiro was not available from the game session. Neither is claimed as a successful tool proof.

The Human player controls the campaign and tactical Human side. Dwarves, Orcs and Vikings independently select and execute at most one action after each new day. Nature/Dark remain persistent passive owners and are excluded from hostile selection. The retained/default map and older profiles are unchanged.

## Play it

From the worktree, using Python:

```powershell
python Tools/ProductionContinuation/play_candidate.py --four-faction-alpha --stage-receipt Evidence/FourFactionAlpha-20261009/Local/stage-alpha-r2/Diagnostics/receipt.json --evidence-root Evidence/FourFactionAlpha-20261009 --minutes 20
```

This is normal interactive play, not an observer fixture. Home selects the army; click a legal destination; Space ends the day; T opens town; B attacks a selected encounter; F5/F9 save/restore; WASD and wheel control campaign camera. Tactical controls are shown by the existing HUD. Save before the bounded launcher closes. The launcher retains a 20 FPS functional cap and the absolute 85°C guard.

Persistent play directory: `Saved/CompositionPlaytest/FourFactionAlpha`.
Save slot: `Soul.Composition3500.FourFactionAlpha`.
Candidate: `/Game/SoulCampaignComposition/L_Composition_3500_r2`, still isolated/opt-in.
Cooked stage: `C:/Users/Jeff/AppData/Local/Soul/CampaignAlphas/FourFactionAlpha-cztry6zj/Stage`.
Executable SHA-256: `e142da67cae5a024a0553df2119a528ce0347468e0cbf323adde9bbed350b6f7`.

## What changed

Existing controlled admission, SoulCore movement/economy, RBSave and RBCombat remain authoritative. The alpha adds the bounded selector/scheduler, paid finite AI recruitment, active/passive profile policy, Human tactical side selection, AI-only battle observation and a short activity recap. No free AP/troops/gold, teleports, forced winners or roster substitutions.

Scoring uses visible opportunities and explored routes: urgent recruitment 350; immediate capital defense 300; weak-army return to supply 220 minus distance; hostile opportunity 160; routine recruitment 130; neutral capture 100; frontier movement 30 minus distance. Attacks require at least 85% of the visible defender count. Stable seed/day/faction/destination tie-breaking; physics-based combat is not promised bit-identical. Recruitment uses real gold (140 per core infantry), pool stock and one AP, up to four troops per action. Starting pool/income values are provisional and shared with existing Human economy rules.

Real campaigns exposed and fixed: occupied-capital day advancement freezing; frontier oscillation around inaccessible stronger/passive targets; Human commands accidentally reaching AI-only battles; tactical controls assuming attacker side; and city visits suspending unfinished AI turns. Alpha-only Dragon Graveyard field fallback now admits the already-qualified configured field environment when no regional recipe matches. Earlier strict profiles remain strict; no approach-table entries were invented.

Defeated army records can withdraw only through legal owned destinations and replenish only at owned settlements using real resources. They cannot capture/attack at zero troops. Capital occupation preserves development identity, pauses construction and denies hostile services. No new official campaign victory/defeat condition was invented.

Save changes are isolated alpha fields inside the existing campaign domain: seed, turn cursor/day, recap and non-Human finite recruitment pools. RBSave authority/schema infrastructure and older save slots are preserved; no 9-to-36 migration.

## Observed campaigns

Final executable, seed 1701, 20 turns/day 21:

| AI | Captures* | Moves | Attacks | Withdrawals | Paid recruits | Holds | Final troops / gold |
|---|---:|---:|---:|---:|---:|---:|---|
| Dwarves | 11 | 5 | 1 | 0 | 12 | 0 | 18 / 10,320 |
| Orcs | 18 | 2 | 0 | 0 | 0 | 0 | 30 / 12,000 |
| Vikings | 4 | 1 | 0 | 3 | 16 | 8 | 16 / 9,760 |

*Capture actions include recaptures, not unique regions. Recruitment consumes a turn action; each faction has exactly 20 retained actions. F9 replayed days 11–13, so raw logs contain 23 observations per faction.

Dwarves naturally defeated Vikings at Mountain Shrine with 6/0 effective survivors; rebuilding continued afterward. Final ownership: Human 1, Dwarf 5, Orc 12, Viking 3, Nature 2, Dark 2, neutral 11. Human: 45 troops, 12,500 gold, Southern Crossing. Nature/Dark remain 30 troops/3,000 gold each, with original ownership intact.

Seed 1702 also completed 20 turns. Vikings instead won the Dwarf attack with 0/9 effective survivors. Dwarves withdrew and recruited 16; Vikings recruited 12. Final troops: Human 45, Dwarf 16, Orc 30, Viking 21. Final ownership: Human 1, Dwarf 5, Orc 13, Viking 8, Nature 2, Dark 2, neutral 5. No rejected candidates in either retained run. Outcomes were not forced.

A separate final-executable process restored the earlier cooked day-21 checkpoint exactly and continued to day 41 (40 total campaign turns). Orcs naturally defeated Dwarves at Ruined Causeway with 25/0 effective survivors using the admitted field fallback. Final troops: Human 45, Dwarf 0, Orc 25, Viking 20; final ownership Human 1, Dwarf 2, Orc 8, Viking 19, Nature 2, Dark 2, neutral 2. The destroyed Dwarf commander remains stranded at the lost site; no free recovery was invented.

Human attack: ordinary 45-troop start, legal moves and actual B input; natural victory at Orc Badlands, 22/0 effective survivors, correct capture/return and F5/F9.

Human defense: explicit isolated 24-troop starting-force fixture, three ordinary moves to North Pass, then the real Orc selector initiated the attack. Aurora was possessed on defending side 1; three Human formations and zero enemy formations accepted player orders; Human mana 80→68. Natural Orc victory 24/0, correct ownership and return, exact F5/F9.

A fresh process restored that defeat exactly. Three legal withdrawal moves reached home; four recruits spent 560 gold and four stock. Continued play produced Dwarf-vs-Viking combat and a second Orc attack on the Human capital. Defensive control passed again, Orcs won naturally, and the occupied-capital campaign continued to day 12 with exact F5/F9. This demonstrates persistence and progression after losses, not a balanced recovery guarantee.

## Validation and evidence

- Fresh final `SoulEditor` r13, `Soul` and `SoulComposition` builds PASS. Final game builds took 576.73/561.43 seconds.
- Final alpha native tests: 7 PASS, including turn resume/visit admission, paid recruitment, passive/malformed restore, occupied capital, zero-army withdrawal, frontier routing and field fallback.
- Unchanged-boundary regressions: strict SixFaction 8; default CampaignWorld 2; Composition CampaignWorld 2; Vertical combat/save 19; physical combat 12. These ran during implementation, not all again on r13. Final r13 alpha tests and all six final cooked runs cover the resulting binary.
- Source/tool tests 11, launcher tests 8, runner safety tests 12 PASS. Staged source diff passes whitespace checks.
- Stage update uses the verified existing asset cook plus the freshly built executable; no new visual dependencies or fresh asset recook are claimed. Default target/cook behavior unchanged. Isolation PASS; final integrity verified all 10,754 base cooked hashes and executable, zero mismatches. 12,761 manifest entries present.
- Executable compression was lossless NTFS compression of the new private copy, with unchanged SHA-256 and the 8 GiB staging reserve maintained. Prior stages remain intact. Temp-file aging/removal is still undiagnosed; locating this stage outside Temp is mitigation only.
- All 585 recorded preservation files PASS: donors, frozen/reference terrain, authored environments, prior saves and inherited dirty/config state. No licensed payload/config admitted. `admitted-files.json` lists the 31 owned source/tool files; `preservation-final.json` contains exact hashes and intentionally retained state.
- Functional runs capped at 10 FPS; normal startup at 20 FPS. Final functional peak 70°C (normal startup 67°C), VRAM up to 6,601 MiB, process peak working set up to 2,769 MiB. These are bounded functional observations, NOT FPS/performance acceptance. Prior uncapped 85°C sustained-performance failure remains open.

Compact evidence: `campaign-outcomes.json` (including final armies, resources, battle/save receipts), `action-log.jsonl` (219 retained observed actions across independent final runs), `final-closeout.json`, `stage-integrity-after-final.json`, `visual-review.html`.
Raw canonical snapshots/action timelines, F5 saves, native screenshots and guarded runtime receipts remain under `Local/final-*`. Gallery captures were inspected directly. Build/test logs stay at this evidence root; large assets/logs/screenshots remain local-only.

Rejected iterations retained: first 20-turn run exposed occupied-capital freeze; initial cold-recovery driver issued recruitment inputs in one frame; first Human-attack driver checked before queued input dispatch; first short cold-defense run exited before the required 60-second live window. Those are not acceptance runs. Fixes and longer final runs supply acceptance without weakening the runner.

## Remaining gameplay limits and next action

- AI-only embodied battles take about 1–2 minutes plus loading. Player observes; no instant mathematical substitute was added.
- One field army per faction, no invented garrisons; undefended recaptures can be frequent. Income is generous, recruitment growth can become the limiting resource, and basic scoring can hold at a blocked frontier. No balance-complete claim.
- A commander with no troops and no owned escape/supply can be trapped. No new official loss condition/restart UX was invented. The inherited zero-army HUD still says to return to/recruit at the capital even when it is occupied; authority correctly rejects that recruitment, but the guidance is misleading.
- Field fallback/ground tiling/material warnings remain provisional, including the unresolved modeling-material dependency, Human algae/shader and WebBrowser warnings inherited from cook. No terrain/road/Dwarf art changes.
- Normal human desktop play and sustained thermal/performance qualification remain important follow-up checks; neither is replaced by the automated functional proof.

Immediate next action: Jeff plays the isolated alpha using the launcher above and reviews turn pace, retreat/recovery and frontier behavior. Toward five factions, reuse the already-admitted Nature bear roster, establish the needed Nature-versus-active-faction battle admission, then enable Nature only in a separate five-faction profile. Dark still needs an exact defensible roster. Do not treat passive Nature in this alpha as absence of its earlier roster proof.
