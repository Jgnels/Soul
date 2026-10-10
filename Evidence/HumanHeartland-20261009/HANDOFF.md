# Human Heartland integrated slice

The opt-in Heartland build now connects paid development, a real walkable Human Capital, a useful campaign site, authored-city approach combat, enemy commander capture and exact cold restoration. This is a materially deeper playable slice, but not the complete requested Heartland feature set.

Start: `8b2ce9c9bf3a1c0cf58773cc12fa3a85b3e6facb`.
Implementation commit: `ef7a909ba7e43675585e488e26a71811d54b48bb`.
Branch: `codex/soul-bannerlord-campaign-map-20260929`.
Final ending HEAD is in [final-head.txt](final-head.txt), written after the evidence commit. No push, merge or default promotion.

## Play this build

Launch `Tools/ProductionContinuation/play_heartland.cmd`: **N** creates a separate session; **C** restores its last F5. Existing FourFactionAlpha launcher, founder session and qualification saves remain unchanged. Heartland uses `Soul.Composition3500.HeartlandAlpha`, under `Saved/CompositionPlaytest/HeartlandAlpha/HumanSessions`. No normal human session was created by the dry-run.

Final stage: `C:\Users\Jeff\AppData\Local\Soul\CampaignAlphas\FourFactionAlpha-nfj9niis\Stage`.
Executable SHA256: `57072718b033eaa119ee5c5a48bb9b8b1a558811247194e19cf9ce1565a6aef9`.
Receipt: `Local/stage-heartland-r3/Diagnostics/receipt.json`.
The front door verifies executable, stage manifest, 94 candidate cooked files and admitted Heartland JSON; 30 FPS default, 85 C hard cutoff, no automated gameplay input. [Controls and limitations](PLAY_HEARTLAND.md).

## Product acceptance

| Requested result | Status | Actual limit/evidence |
|---|---|---|
| Remote save location | PASS | Crossroads F5, separate process F9, exact full campaign snapshot; both RBSave domains used. |
| Capital orbit/inspection | PASS | Full yaw, bounded pitch, zoom, army/selection focus; reviewed cooked capture. |
| Capital walkable visit | PASS | Actual authored map, 56,448 loaded actors, 19.6 m physical walking and clean return. One quay route proved; all interiors/collision not exhaustively explored. |
| Capital correct battle environment | PASS | Actual capital outer approach, natural Human defense and campaign return; no siege claim. |
| Hero normal attack | PASS | Explicit LMB sword/RMB guard guidance; actual accepted RBCombat melee damage. |
| Hero/formation selection separation | PASS | Hero-only selection is separate; formation/all-troop selections exclude hero. Native selection test passed. |
| Enemy hero | PASS | Exact owned Dwarf King commander participates and can be captured; visual distinction still provisional. |
| Hero wound/capture state | PASS | Final natural battle wounded Aurora for three days and captured the Dwarf commander. Both survived cold restore. Recovery/restrictions also native-tested. |
| Mage Guild progression foundation | PASS | Paid one-day Arcane Hall learned Frost in cooked play. Two-/three-day definitions and locked spell admission native-tested; their complete live progression not exercised. |
| Hero affinity | PASS | Frost primary; Water/Air secondary; Fire/Lightning forbidden. Locked casts rejected by actual combat admission. |
| Human building tree | PARTIAL | Six real build/upgrade options; several requested categories and new visual actor groups missing. |
| Multi-role Human recruitment | FAIL / not implemented | Only `human_knight` infantry; no fake ranged/cavalry entries. |
| Heartland sites | 3 implemented / 0 newly visually placed | Crossroads Windmill, Old Quarry, Ancient Shrine; dedicated site art remains absent. |
| Location-aware battlefields | PARTIAL | Capital exact binding plus existing field/pass coverage. Bridge, ford and forest-specific entries remain unset. |
| Formation-order reliability | PARTIAL | Selection isolation and existing persistent manual orders retained. No complete HOLD/MOVE tactical-navigation qualification in city clutter. |

Closer to Bannerlord embodiment + Heroes-style depth: **PARTIAL**. City walking and meaningful progression are real gains; recruitment variety, visible sites, companion embodiment and battlefield variety still limit the experience.

## What changed

- **Save diagnosis:** the founder's preserved log shows the successful F5 at the capital before later travel. The restart loaded that older checkpoint. The reported remote teleport could not be reproduced, so this is evidence for an older save, not a claimed universal corruption fix. F5/F9 now report saved/loaded day and region. A separate reproduced presentation defect was fixed: remote F9 inside a city now exits the invalid visit and returns to the restored campaign location, including during streaming.
- **Visit:** `/Game/Soul/Maps/Settlements/L_HumanCapital_Authored`, complete existing authored setup. V enters, T manages/recruits; WASD/Shift walk/run, RMB look, Tab management, Esc return. Full source/editor city streaming was slow; cooked city battle streaming was roughly 72 seconds in the first complete proof. No donor map saves.
- **Camera:** Q/E orbit, Page Up/Down pitch, wheel zoom, Home army, F selected location. Movement controls preserved.
- **Battle:** Human Capital is explicitly bound to its owned city at approach origin `(-9000, 21000, 400)`. Final fresh-binary natural result: Humans defended successfully, 23 survivors; Dwarves 0. Aurora wounded, Dwarf commander captured by Humans at Human Capital. F5/F9 and cold restore preserved this. Earlier complete run yielded 29 Human survivors; outcomes were never forced. Post-return text now labels attacker/defender survivors and the Human company correctly.
- **Commander:** `dwarf_king_commander`, `/Game/Dwarf_Pack/King/Mesh/SK_Dwarf_King_Full`, existing King axe/qualified Dwarf animation adapter. Replaces one existing Dwarf soldier slot, adds no free troop, and does not respawn once captured.
- **Hero rules:** wounded with surviving company means three-day recovery; wounded with destroyed company means capture. Captor and region persist. Unavailable heroes grant no existing travel bonus, cannot be embodied/cast in battle, and fail siege/diplomacy eligibility hooks. Rescue/ransom, actual siege and diplomacy systems are not implemented here.
- **Development:** Tavern 200g/1d; Arcane Hall 300g/1d; Mage Academy 600g/2d; High Conclave 900g/3d; Expanded Market 400g/2d (+100 daily gold at level 2); Veteran Barracks 500g/2d (+2 weekly recruit stock, cap 24). Prerequisites, costs and one construction decision per day use existing authority. Guild unlocks existing Frost Blizzard, Water Tidal Ward and Air Tailwind. New physical building groups are explicitly pending; existing tavern visual behavior retained.
- **Sites:** owned living company spends 1 movement once/day for Windmill +100 gold, Quarry +140 gold, Shrine +20 mana. Full mana/unavailable hero rejects shrine without spending. Cooldowns persist; UI explains collected state. Two naturally neutral site regions require legal capture. A bounded search of existing medieval donors did not establish a ready, reviewed windmill/shrine mesh; no substitute art was invented.
- **Registry:** `Data/SettlementEnvironments/EnvironmentRegistry.json` drives the new exact city battle override. Visit/development still use the existing settlement scenario asset, whose Human/Dwarf mappings are recorded in the registry; this is not yet a generic multi-settlement visit binder. Dwarf battle origin/admission, sieges and unknown geographic categories remain unset. `HeartlandDevelopment.json` supplies building, spell-affinity and site data.

## Qualification and delivery boundary

Fresh **SoulEditor r9** and **SoulComposition r3** builds passed. Generic `Soul` target was not rebuilt. Native gameplay tests passed on r8: Heartland 5, FourFactionAlpha 7, formation selection 1; unchanged camera/selection test also passed earlier. Final source/launcher suite: 19 passed. r9/r3 then changed only the post-battle text and passed the final cooked battle/cold restore.

Seven complete cooked functional receipts are summarized in [runtime-summary.json](runtime-summary.json). City walking, paid guild learning/site interaction, remote cold restore, city F9 return, city battle/save and final-binary cold restore were reviewed. Final battle confirms normal RBCombat, reinforcements, correct Human defensive control and persistent commander state. Existing AI captured Snow Basin, Badlands and Fjord Ridge during the progression chain; no old 20-turn campaign was needlessly repeated.

This is a fresh executable + two admitted data files on the previously verified cooked asset base, **not a fresh full asset cook**. Stage copied 124 private files, linked 12,643, verified 10,754 base cook hashes and 136 runtime dependencies. Isolation verification found no rejected experimental payloads or editor security token. Source/data boundary: [source-boundary.json](source-boundary.json), exactly 40 intentionally changed files. Inherited 17 tracked dirty files, local assets/configuration and existing evidence were excluded from commits. 602 preservation entries matched; full before/after hashes in `Local/preservation-final.json`. No terrain, map, donor or licensed asset edits.

Functional runs were capped at **10 FPS**, not performance tests. Highest measured GPU temperature: **84 C**; final run temperatures are in the runtime summary. The 85 C guard remains unchanged. The human launcher defaults to 30 FPS but sustained 30-FPS city safety/performance is **not qualified**. Some earlier NVML VRAM samples were invalid and are not reported as real memory peaks.

Rejected/incomplete attempts remain in Local: an uncooked Orc city run timed out during return; an early walk had pending materials; one launcher timeout-argument rejection occurred before launch; a stage command used the wrong prior receipt and was corrected. These are not counted as passes. Existing algae/material shader warnings remain; no warning cleanup is claimed.

## Remaining P0/P1 work and next lanes

No state-corruption P0 was reproduced in the final checks. Thermal headroom is still a practical blocker for long visits. P1 gaps: single-role recruitment; no visible recruited companions; missing new building/site art; incomplete battle-map variety; no rescue/ransom; existing Human defensive victories grant no hero XP; uneven donor collision/materials; tactical-order effectiveness needs direct human testing.

Next main integration task: real ranged + cavalry company support through the existing battle bridge, followed by visible companion participation. A roster lane can safely return exact owned body/weapon/animation/role mappings without changing combat authority. An environment lane can supply measured battlefield entry/bounds for the registry's unset classes. A settlement-art lane can identify authored actor groups for the admitted guild/market/barracks states and real site meshes. Diplomacy/audio design may proceed independently; do not mutate shared campaign/save/battle source or donor packages without coordinating the integration boundary.

Evidence: [review gallery](visual-review.html), [delivery receipt](final-closeout.json), [play instructions](PLAY_HEARTLAND.md). Raw logs, screenshots, saves and private stage receipts remain local under `Local/`.
