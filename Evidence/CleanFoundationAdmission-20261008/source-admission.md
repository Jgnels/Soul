# Clean Composition source admission

Starting HEAD: `10aa400d75c6a4bf5db2d7f403122a6e3e17b323`.

The retained tracked terrain implementation is the base. Only the previously qualified Composition loader, native height sampling, dense route interpolation, ferry visibility, miniature transforms, camera framing and state/save-slot hooks are added. Baked candidate scenery remains owned by its existing map; no terrain, route, settlement or gameplay schema was authored in this separation.

The inherited working terrain diff was 828 changed lines. The admitted terrain diff is 122 changed lines. Default retained terrain loading, dressing, roads and settlement population return to the tracked implementation. The camera keeps the qualified Composition bounds and the existing retained controls. Its explicit rendered-frustum acceptance check remains available.

## Excluded drafts

`excluded-drafts.json` records byte hashes and local preservation paths for the former untracked Expansion implementation/header and mixed World/retained capture translation unit. Their complete original bytes also remain in `Local/Before/Source`. They are outside the Source compile tree. No R10 height, material, water, settlement, forest or capture implementation is admitted. Unrelated WorldTerrain tools/data, machine configuration, local mounts and licensed content remain inherited and uncommitted.

`Soul.Build.cs` is exactly its already-admitted HEAD version: candidate-only four data dependencies plus the HDRIBackdrop content descriptor. The three inherited R10 dependencies and their temporary skip guard are excluded. No default target/map promotion occurs.

## Validation status

Source reconciliation and fresh-source runtime reproduction PASS. Fresh SoulEditor, Soul and SoulComposition builds pass; default and Composition native CampaignWorld tests each pass 2/2. The freshly staged Composition executable passed input/save, separate-process restoration, 40 legal moves and the Human authored construction/visit/battle/return proof. See clean-source-build-inputs.json, fresh-builds-r1.json, link-boundary-r1.json and clean-runtime-results.json. Existing unchanged cooked assets were reused: this is a fresh code qualification, not a new asset cook. Existing save schemas and isolated slots remain unchanged.
`/Game/SoulCampaignWorld/SM_WorldBroadleaf` is one historical mesh reference already present in the tracked retained Crownstead dressing. It remains byte-identical to starting HEAD for default compatibility. This does not admit a World map, loader, data dependency or implementation; Composition returns before retained dressing.

The shared qualification runner admits only Composition process-name guarding. The inherited 20-second World benchmark exception is preserved outside admission; the 60-second minimum and 85 C hard cutoff remain. The default packaging-script/config drafts are untouched and excluded.
