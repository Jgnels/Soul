# Mountain05 plains workflow

These scripts target the upstairs local preview at `D:/RefinedBadger/AssetLibraries/SoulTerrainPreview`. They operate on licensed local inputs and are not a distributable terrain asset pack.

The editable result is `/Game/SoulCampaignMountain/L_mountain_campaign` in SoulTerrainPreview. The original `/Game/LandscapePackOne/Maps/Mountain_05` must never be saved or edited. Height inputs, generated PNG/NPY files, Unreal assets, screenshots and caches stay outside this repository.

Run `bake_plains.py` with Python containing numpy and Pillow, then `validate_plains.py`. The bake reads `TerrainWork/mountain05-height.npy` (2041 square, RG16 decoded from the source Landscape export) and writes before/plains_v2 heightmaps and metrics. It changes only Jeff's human polygon, preserves submerged ground and shoreline, and retains 20% of smoothed source relief in the inner plains.

Unreal scripts require UE5.8 and Nwiro listening on loopback port5353. Do not launch a second editor or interrupt another capture. `import_plains.py plains_v2` creates a separate derived map; `dress_plains.py` requires the two project-owned materials to exist, created through Nwiro's dedicated create_material tool. It never calls AssetTools factories.

`start_capture.py` schedules five actual Unreal screenshots and pumps viewport draws through Nwiro. In rendered-offscreen editor sessions an Automation screenshot can remain pending without those draws. Inspect files and the capture receipt before changing levels. `set_final_view.py` leaves the final map camera on the human plains.

The final map uses the verified plains_v2 geometry, copied to mountain_campaign.png for its persistent name, plus the final ground/water material palette. `build_plains_gallery.py` publishes the local comparison. Source baseline final-material screenshots are `L_before__final_*`; final screenshots are `L_mountain_campaign_*`.

Collision qualification uses a 0.25cm sweep at121 sample vertices. All121 hit, max height error0.0889cm. Zero-width rays at40 exact outer-component vertices miss while nearby sub-centimeter samples hit; do not describe those rays as passing. Recreating collision did not remove that precision caveat after reopening.

These tools do not replace campaign simulation or establish campaign/battle integration. See Evidence/MountainPlains-20260930/HANDOFF.md for qualification and remaining work.
