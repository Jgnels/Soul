# Actual local cooked-stage footprint

12,658 files; apparent file sizes total 17.56 GiB. Cooked assets share hardlinks with this run's cook and earlier stage, so these are not additional independently allocated bytes.

| Family | Files | Apparent GiB |
|---|---:|---:|
| Soul/Content/CastleTown | 2106 | 3.329 |
| Soul/Content/DwarvenCitadel | 2802 | 2.462 |
| Soul/Content/Knights_Pack | 322 | 2.022 |
| Soul/Content/Dwarf_Pack | 320 | 1.621 |
| Soul/Content/ParagonSparrow | 699 | 1.345 |
| Soul/Content/ParagonAurora | 645 | 0.794 |
| Soul/Content/Soul | 172 | 0.735 |
| Soul/Content/Dragon_graveyard | 210 | 0.599 |
| Soul/Content/Forest_village | 215 | 0.481 |
| Soul/Content/SoulCampaignProxies | 30 | 0.448 |
| Soul/Content/Kingdom_Capital | 134 | 0.396 |
| Engine/Binaries | 93 | 0.380 |
| Soul/Content/Fantasy_Pack | 216 | 0.365 |
| Soul/Binaries | 18 | 0.348 |
| Engine/Plugins | 973 | 0.325 |
| Soul/Content/AlienPlanet | 144 | 0.300 |
| Soul/Content/Kraken | 50 | 0.263 |
| Soul/Content/Ravenhold | 242 | 0.248 |
| Soul/Content/Medieval_Megapack | 206 | 0.245 |
| Soul/Content/Medieval_Warzone | 315 | 0.168 |
| Soul/Content/QuadrapedCreatures | 75 | 0.123 |
| Soul/Content/SoulCampaignComposition | 94 | 0.121 |

All 95 staged INI files were checked for nonempty sensitive-key names, Nwiro sections and absolute Windows paths without printing values. Raw sensitive-key findings: 1; any matching installed-engine source values are classified separately in the JSON. UAT already removed the local Android editor token. This bounded check is not a general security audit.

The temporary loose stage is qualified locally and is not a distributable archive. It remains dependent on these recorded local paths; do not advertise it as an uploaded build. No donor or licensed package is committed.

No content pruning is performed merely to lower this number. The authored Human city remains complete; a future compact distribution should use a reviewed profile and normal installer/archive packaging.
