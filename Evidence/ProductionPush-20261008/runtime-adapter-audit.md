# Runtime integration audit / chosen boundary

Starting HEAD c610966001d9aafd739f9d49550261e1f5d4f43f. All 33 inherited tracked modifications were copied and hashed before edits.

Current implementation already owns campaign state in USoulFounderPlaytestStateSubsystem / SoulCore, persistence in RBSave with Soul.Campaign and Soul.Settlements providers, campaign presentation in SoulCampaignTerrain + SoulCampaignWorldActor, and stateful miniatures in SoulSettlementBuildingActor. Reuse these interfaces.

Inherited drafts include SoulWorldTerrain (R10), SoulCampaignExpansion (tile study), camera/world-context presentation branches and packaging-data additions. Their switches are opt-in, but they are not accepted candidate authority. The new isolated composition profile must reject conflicting experiment flags. No startup config/default-map change. Compile after source audit, not via Live Coding.

Selected minimal integration: an explicit SoulComposition command-line profile in the existing terrain adapter. Load the already-authored composition level at identity; read its frozen heightfield and final 36 anchors / 51 polylines. Preserve baked road/material and crossing art: do not generate another road ribbon/scatter layer. Reuse canonical scenario data / explicit existing qualification fixtures, not terrain-driven adjacency. Expand presentation to 36 selectable canonical nodes under the same rules. Existing Human authored proof may run against the candidate with its explicit qualification ownership overlay. Isolate UserDir/save roots for tests; no save-schema change.

Bridge travel Z comes from the existing qualified native route samples or equivalent verified surface data. Ferry segments are tagged in presentation data; hide marching figures only while traversing open water, with existing strategic move authority unchanged. No boat subsystem.

Use the existing Human base/upgrade actor binding at the measured candidate miniature transform. Hide only its baked static scale-reference duplicate in the transient runtime instance. All other baked campaign art remains unchanged. No authored settlement source edits.

Camera bounds and zoom are profile parameters, not World/R10 activation. Selection and knowledge remain controlled by existing region actors. Candidate terrain may show geographical roads regardless of exploration (cartographic scenery); ownership/forces/selection continue to obey knowledge gates.

Source modifications overlapping inherited files will have a delta against Local/Inherited, with unrelated inherited hunks left in place. Do not stage the entire dirty source as accepted work.
