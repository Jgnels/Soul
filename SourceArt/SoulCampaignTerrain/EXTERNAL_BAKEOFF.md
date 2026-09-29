# External terrain bakeoff / import contract

No commercial license was verified on this machine. Native project-owned heightfield is the current candidate. No free/community tool output is used in game content. The candidates and prices in the lane prompt are research inputs, not newly verified purchasing advice; no install, account, paid terms or purchase is authorized by this pipeline.

For an already owned commercial-capable Gaea, World Creator or World Machine license, record application/version, edition, evidence of commercial export entitlement (without license keys), source project hash and export hash before admission. Evaluation exports belong only under an ignored `EvaluationOnly/<tool>/<run>/` folder outside Content. Do not import those into shippable TerrainV2 assets.

## Shared input

Use `FounderHeight.png`, presentation.json and the canonical topology solely as constraints. Seed 29092026. Cover 5600 x 5600 meters with center at UE (0,0); playable places remain at presentation.json coordinates. North corresponds to decreasing Y in the campaign view. The source PNG top row is world Y=-280000 cm; column zero is X=-280000 cm. No implicit Y flip.

UE import: 1009 x 1009 unsigned 16-bit grayscale PNG (or little-endian RAW16), eight components on each axis, two subsections of 63 quads. XY spacing 555.5555556 cm. Landscape location (-280000,-280000,0); Z scale 320. Height in UE cm = (sample-32768)*2.5. No clipping to 0/65535, color management, alpha, gamma transform or lossy compression. This is a heightfield presentation contract, not geography/gameplay authority.

Keep existing place terraces, protected road corridors, drainage outlet and the river-ford crossing fixed. Do not infer new strategic edges. Export rock/slope, deposition, wetness, flow accumulation and vegetation suitability masks as aligned linear grayscale maps. Export route constraints separately; road visibility still obeys both endpoint exploration gates. Record min/max elevations, histograms, no-data count and maximum grade along every route.

## Repeatable trials

1. Gaea: geological macro forms, erosion/deposition; compare one 15-minute and one 60-minute authored pass. Export through the verified licensed bridge or the common height/mask contract.
2. World Creator: same timed passes and inputs; compare direct shaping speed, erosion and mask fidelity. Any bridge/PCG object transfer remains decorative, with no gameplay authority.
3. World Machine: same passes; focus on catchment/drainage restructuring, channel continuity, flow and deposition masks.

Retain tool graph/project, settings, seeds and timings. Import each candidate into an isolated level, never overwrite accepted source or donor packages. Rebuild the runtime height payload from that exact export, verify nine landmarks and ten routes, then capture the same 1920x1080 overview/capital/ford/forest/quarry/shrine/pass/stronghold/ground views. Compare river continuity, unnatural terraces, silhouette readability, road grade, biome transitions and accepted bounds. Run the same uncapped 20-second warmup / 60-second measurement with editor closed; report mean/P95/P99 and counts over 33.333/25/16.667 ms. Admit only an improvement supported by rendered evidence and unchanged gameplay tests.
