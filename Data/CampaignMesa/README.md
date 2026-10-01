# Approved terrain campaign profile

Soul's default campaign streams `/Game/SoulCampaignMountain/L_evil_waterfront`
from the approved external SoulTerrainPreview project. The campaign actor is the
game-owned wrapper: it removes study cameras/lights from the transient instance,
keeps the terrain and water, and creates Soul's existing regions, roads, army,
camera and HUD. It never saves the referenced level or materials.

`presentation.json` owns the nine region placements and ten route polylines.
Canonical movement, visibility, armies, encounters and saves still belong to the
existing Soul/RB systems. Routes occupy the human peninsula and connected dwarf
foothills. These are dry routes; the retained `river_ford` ID does not create an
artificial river or bridge. The western mesa remains visible geography, without
invented regions or lava hazard rules.

The 2041-square heightfield uses `z_cm=(u16-32768)*0.5`, XY extent 150000 cm,
origin (-75000,-75000). Runtime interpolation uses Unreal's Landscape triangle
diagonal. Camera scale is 10; location miniatures use 5. No old procedural
terrain or old fixed forest scatter is built for this profile.

Local setup (licensed content required):

1. Run `Tools/integrate_soul_mesa.py --preview <SoulTerrainPreview>` with NumPy
   and Pillow. It reads the approved PNG, writes the external heightfield, and
   regenerates this game-owned placement profile. It does not write Unreal assets.
   The `CampaignMesa` directory and `MesaHeight.r16` names are retained for mount
   compatibility; the profile records the current waterfront source and hash.
2. Run `Tools/Setup_Soul_Mesa.ps1 -Preview <SoulTerrainPreview>` to verify/create
   ignored content/data junctions and check the height hash.
3. Build SoulEditor and run `Tools/Open_Soul_Campaign_Mesa.bat`.

The binary heightfield resides in the preview's `SoulIntegration` folder. The
three content mounts and data mount are Git-ignored. Do not add their contents.
Another machine must supply its admitted local asset library. Missing terrain
or a collision/alignment failure causes an explicit error, not placeholder proof.

`-SoulTerrainV2` explicitly selects the earlier owned terrain. `-SoulLegacyTerrain`
explicitly selects the original procedural presentation. `-SoulMesaTerrain`
explicitly selects the new default and identifies qualification receipts.

Qualification: `Tools/qualify_soul_mesa.ps1 -Mode Input`, then `-Mode Load`
for a fresh-process F9 check of the preceding F5 snapshot, then `-Mode Roundtrip`
and `-Mode Defeat`. Defeat uses bounded 12-versus-70 canonical force pools before
the ordinary B action; the physical battle decides the result.
These tests issue simulated keys and cursor events through the actual player
controller; they are not an OS-level manual playtest. The runner keeps the existing
85 C cutoff. It uses the normal RB Save slot: preserve an existing player checkpoint
before tests. Evidence and acceptance status are in
`Evidence/CampaignIntegration-20260930/HANDOFF.md`.

Click the army summary at the top or press Home to select/focus your company.
Gamepad: left stick pans, right stick moves the cursor, bottom face button
selects through the same mouse/HUD path, right face button closes panels,
left/top face buttons open town/commit battle. Left/right triggers zoom out/in,
shoulders orbit, Back selects the company, and Start ends the day.
Gamepad qualification injects controller events; physical hardware feel requires
an attached controller.
