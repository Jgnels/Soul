# Controlled faction qualification

This is opt-in deterministic action qualification, not autonomous strategy. `SoulControlledCampaignAction.cpp` delegates movement, AP, capture, save and battles to the existing authorities. Full AI stays off.

Use `qualify_runtime.py controlled --six-proof --controlled-attacker <key>` with an isolated evidence root. Keys are explicit fixture plans: `dwarves`, `orcs`, `vikings`, `human_nature`, `nature`, `dwarves_orcs`, `orcs_dwarves`, `dwarves_vikings`, `vikings_dwarves`, `orcs_vikings`, `vikings_orcs`. Each stages armies through canonical legal moves; no ownership overrides or forced battle result. The corresponding `load` run requires `--source` pointing to that run and the same key.

`collect_reverse.py` requires both natural battle and separate-process restoration evidence. It checks exact body identity on both sides, real RBCombat contacts, effective campaign survivors, owner/return location and identical persisted save bytes. Physical survivors are reported separately because existing morale defeat zeroes effective campaign survivors.

`pair_matrix.py` reads the explicit source whitelist and qualification receipts. Available bodies alone never imply an ordered pair is admitted. Pair admission also does not waive action-specific edge, defending-army or geographic recipe requirements. `make_review.py` displays existing Unreal captures only.

For cooked proof, pass the verified isolated stage receipt to both launches. The additive roster cook uses only the exact Viking weapon and Nature package closure. It cannot overwrite the preserved base cook, asset registry or shader libraries. This loose hardlink stage is local qualification, not a distributable package.

Use one heavy Unreal/build process at a time. Functional runs remain capped and guarded at 85 C; no performance claim. Licensed packages and local stage payloads are not committed. Existing default and authored-settlement slots remain unchanged; controlled fixtures have distinct slots.
