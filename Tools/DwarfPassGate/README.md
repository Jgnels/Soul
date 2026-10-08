# Dwarf pass visual-gate recipes

This is a bounded failed visual-gate checkpoint, not a runtime adapter or terrain generator. Read Evidence/DwarfPassGate-20261007/HANDOFF.md first.

- checkpoint.py was run once at the inherited MapFinalPolish HEAD; do not rerun over the preserved Before directory.
- analyze_bypass.py is read-only analytical terrain evidence. Its hypothetical walls are not gameplay blockers. Native trace results and rendered inspection accompany the analysis.
- fit_mountain_curves.py and fit_mountain_curves_r2.py are REJECTED trials. No output from them was bound to the map. Do not promote their results.
- damage_broken_bridge.py is a one-shot live-editor mutation of the candidate only. It asserts that its new owned derivative assets do not exist; do not blindly rerun. Original licensed bridge assets remain unchanged, original collision actors remain active, and the rendered replacements are noncolliding.
- validate_routes.py copies the unchanged final5 routes from MapFinalPolish, generates native probe inputs and all 51 analytical plates. The existing Tools/MapPolish/qualify_native_routes.py ran live with its evidence root redirected to this pass and QUALIFY_REVISION=final; summarize_native.py reads that native result.
- contact_review.py produces analysis-only sheets from native screenshots. Final camera transforms and paths are in Local/captures-final/receipt.json; original capture engine is Tools/MapPolish/capture_review.py.
- verify_preservation.py hashes the baseline files and inherited tracked state, asserts only the candidate umap changed, and records native test log lines.
- closeout.py generates the handoff and paired review. It records the current HEAD as the starting HEAD, so do not regenerate it after the milestone commit without correcting that field.

Local assets/screenshots/traces are intentionally excluded from the source commit. See candidate-asset-manifest.json and preservation-final.json. To reverse only the bridge rendering change, hide PassGate_BrokenVisual_West/East and restore visibility/cast-shadow on Polish_Crossing_broken_West/East; preserve all packages and collision. Do not restore the whole worktree or overwrite unrelated state.
