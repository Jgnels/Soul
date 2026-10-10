# Human Capital Siege V0 — local playtest handoff

**HUMAN CAPITAL SIEGE V0: PASS. PLAYTEST BUILD READY: YES.** This is an opt-in assault at the actual Hivemind capital gate. It does not replace the retained/default campaign or the previous Heartland stage.

## Launch

Run `Tools/HumanCapitalSiege/play_siege.cmd`. Choose **N** for a new isolated siege session; **C** continues its latest F5 checkpoint. The launcher verifies the pinned R6 stage/executable/candidate assets, uses 1280×800 at a 30-FPS cap, preserves the 85°C guard, and sends **no automated gameplay input**. The final launch plan was dry-run checked; functional siege qualification ran at 10 FPS, not 30-FPS performance acceptance.

Save with **F5 before leaving Crossroads** for a retry checkpoint. Select your Human army, select the enemy-held Human Capital, and Attack. Use P to pause, C for hero/commander camera, X for hero view, WASD/LMB/RMB for movement/melee/guard. All/H/G/V select/hold/charge/follow; use Move for a ground destination and R to restore formation AI. Breach the portcullis through melee, then take the inner ring or defeat/rout the defenders. A broken gate alone does not win. F5/F9 work in campaign after return. Exit with Alt+F4 after saving; keep the guarded launcher console open.

Save root: `Saved/CompositionPlaytest/HeartlandAlpha/HumanSessions/SiegeV0`.
Slot: `Soul.Composition3500.HeartlandAlpha.SiegeV0Playtest`.
The isolated start repositions existing armies at fixture initialization and splits the existing 45 Human troops into 37 infantry / 4 archers / 4 guards. It grants no extra army and never scripts the battle outcome.

## Exact boundary

- Branch: `codex/soul-bannerlord-campaign-map-20260929`.
- Start: `5f212fdd9ef38b46c52fe84813ce1d069b777447`.
- Implementation commit: `61a6532cfc09bfd0086670b52293892446dab668`.
- Ending evidence commit: read the post-commit `final-head.txt` (avoids embedding a commit's own hash).
- Exact intentional source/Data hashes and excluded inherited paths: `implementation-boundary.json`. Twenty-one source/Data files and seven siege tool/launcher files were committed; unrelated dirty state was not staged.
- Stage: `C:\Users\Jeff\AppData\Local\Soul\CampaignAlphas\FourFactionAlpha-ch7rwc10\Stage`.
- Executable SHA256: `45e72066ca9dac01162effb9ff1804f8347e7f19ea767eba8b29edaaa8313eb5`.
- Receipt: `Local/stage-siege-r6/Diagnostics/receipt.json`.
- No push, merge, history rewrite, donor save, default-map promotion, or new licensed asset payload.

## Physical siege and authority

Registry recipe `human_capital_gate_siege` loads `/Game/Soul/Maps/Settlements/L_HumanCapital_Authored`. The original field approach remains available without the opt-in siege eligibility. Hostile fortified capital + valid army + eligible attacking commander is required. Wounded/captured commanders reject before mutation.

The native `SM_Portcullis` actor is the sole blocker adapted at runtime. Its authored base is approximately (5376.59,1390.95,1050) cm; the siege closes it about 261 cm to the measured floor, with final base Z 788.97. Inside direction is (0.68928,-0.72449). Walls, gatehouse, surrounding collision and donor bytes remain untouched. Ground-level capsule sweeps pass; the closed gate blocks a human capsule. The prior strip inspection sampled stacked roof/walkway geometry rather than establishing this ground-relative path.

Attackers assemble outside; defenders hold inside. Accepted RBCombat melee contacts reduce integrity. At zero only the portcullis is hidden and its collision disabled. Troops physically traverse the opening. Local reciprocal avoidance is disabled near the aperture to prevent deadlock, while native capsule/world collision remains; ordinary avoidance resumes outside. HOLD/MOVE/FOLLOW and explicit orders remain authoritative.

The inner courtyard is 9.5 m behind the gate, with a 4.2 m capture radius. Fifteen seconds of actual uncontested attacker occupancy captures it; a living defender contests; empty attacker occupancy decays progress. Capture triggers existing battle rout/result authority. Defeating/routing the enemy remains the alternate win. Routed strategic zero does **not** mean every still-living physical soldier was killed.

Aftermath uses the existing campaign and settlement domains: exact company survivors, owner, gate integrity/scar, commander condition/captor and normal AP/day. Optional backward-compatible siege result-summary fields are saved; no new save domain/schema version. Missing old fields clear stale siege summary; malformed partial/contradictory fields reject atomically. Pending-battle save/load remains blocked: no mid-battle resume is claimed.

## Qualified outcomes

See `runtime-receipt.json` for exact final run records and `visual-review.html` for actual Unreal frames.

- Final R6 natural attacker win by defender rout: 26 Human survivors (25 infantry / 0 archers / 1 guard), capital changes to Humans, Aurora wounded, Dwarf commander captured by Humans. Gate zero persists. F5/F9 and separate-process exact restore pass.
- Final R6 natural defender win from an isolated nine-infantry attacking start: Humans routed, 24 Dwarf survivors, Dwarf ownership retained, Aurora captured by Dwarves at Human Capital. Gate zero persists. F5/F9 and separate-process exact restore pass.
- Earlier R5 actual inner-objective win: 15-second courtyard capture with five physical defenders still alive; campaign result and cold restore pass. R6 changed only optional siege-result summary persistence after that gameplay proof. A single final R6 replay also resolved by defender rout, with 32 Human survivors (29 infantry / 1 archer / 2 guards); exact F5/F9 and separate-process restore passed. No reruns chased capture. The new true-valued capture-summary field has not had a final-R6 cold-load runtime proof; the earlier capture outcome and owner/gate persistence did pass. Do not call a rout victory a capture victory.
- Actual hero combat inside the city is preserved from the earlier diagnostic run in the gallery appendix. That run was rejected for troop congestion; it is not presented as the final result run. In the final standard assault Aurora was wounded at the gate and her troops completed the attack.
- Native rules cover fortified integrity 1000 vs 1300, objective contest/decay, invalid hits/results, injury admission, exact aftermath and control commands. Purchased fortification-upgrade-to-siege playthrough is **PARTIAL**, not qualified.

## Builds, preservation and limits

Fresh SoulEditor, Soul and SoulComposition builds pass. Forty-four focused native tests pass; the final save-summary change received 29 relevant reruns. Twenty-two source/tool tests and 16 launcher tests pass. Full receipts: `build-test-receipt.json`. Unchanged long Heartland visit/progression paths were not all replayed this sprint.

R6 uses the existing isolated cooked asset payload plus the fresh executable; this is an actual cooked runtime proof, not a claim of a new asset cook. Stage verification checks 13,348 manifest entries, 10,754 base cooked hashes and executable identity. The legacy verifier's supplemental-file field is a count, not an additional full supplemental rehash. External temp-file aging/removal remains undiagnosed.

`preservation.json` confirms 567 monitored assets and 17 inherited content-dirty tracked files unchanged, plus the original qualified Heartland executable. The eighteenth inherited status-only C++ path has no content diff and was untouched. `native-donor-preservation.json` compares 8,083 donor inventory entries and 6,328 map/external package hashes with zero mismatches; other donor entries use historical size/mtime, not a fresh whole-texture rehash.

Known limits: one gate/one objective; crowded and slow doorway; simple gate disappearance; no ladders/engines/wall-top combat; Human tactical defending siege not independently qualified; no new gate repair or captive rescue/ransom; no mid-battle save/resume; inherited texture-streaming/material warnings. Manual play is 30-FPS capped; functional evidence is 10-FPS capped, peak accepted temperature **70°C** (85°C cutoff), and is **not** sustained performance qualification.

**Next action:** Jeff plays the isolated gate assault, especially hero-led FOLLOW/MOVE through the aperture, clarity of the capture ring, and whether the native doorway congestion is enjoyable. Improve this one assault based on that feedback before adding ladders, engines or another city.

## Rejected iterations and qualification scope

The initial stacked-geometry survey did not prove a ground route. The accepted adapter instead measures the native floor and fails closed on capsule obstruction. A rear deployment slot at an authored walkway joint was rejected; staging moved locally to the solid approach. Earlier troop congestion was rejected and corrected through a gate-clear waypoint and bounded reciprocal-avoidance adjustment, not removal of wall collision. One full-city Editor inspection exhausted memory after exporting its measurements; subsequent physical acceptance used the isolated cooked runtime. No donor was saved.

Gate damage is melee-only in V0. Existing hero magic/guard and formation controls were preserved; no new spell-to-gate damage or artillery is claimed. This local executable is reproducible from the admitted implementation plus the preserved local licensed asset/configuration boundary, not from an asset-free clone.
