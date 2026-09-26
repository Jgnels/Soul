# Soul RB Save scoped checkpoint correction — 2026-09-25

Implemented and qualified in `5802f5311d15bf8171f5262d72f34cf81ea335aa` on
`astra/soul-pro-vertical-20260925`; actual rendered disk persistence PASS.

## Observed failure
The first real headless campaign round trip resolved Dragon Graveyard with player victory, survivors 4/0, reinforcement waves 3/3, one RB Magic cast, and PBIL queries/success/orders 16/16/7. Campaign target orc_watch was applied correctly. Disk save then failed:
`capture failed for 'RBItemEconomy': Server economy required`.

Source inspection established that RBFoundationLegacyAdapters registers all legacy save providers unconditionally. The current Soul campaign does not initialize an RB Item Economy catalog. Its provider therefore cannot capture an economy; the weather provider similarly requires a registered director. Delaying the save does not create these unused domains.

## Correction
The existing RB Save authority now offers explicit SaveSelectedDomainsAsync / LoadSelectedDomainsAsync checkpoint APIs. They use the existing atomic domain file backend and unchanged format. The original all-domain APIs remain strict.

The Soul checkpoint explicitly selects:
- Soul.Campaign: region ownership/location, real army pools, casualties, progression, economy, explored geography, resolved encounter identities and last battle result.
- Soul.Settlements: existing settlement-state provider, preserving any current settlement consequences.

Other registered authorities are neither unregistered nor given fabricated empty payloads. They are outside this explicit checkpoint scope and remain untouched on scoped restore.

Empty, unnamed, duplicate and unregistered selections fail. A selected provider failure fails the operation. A selected domain missing from the snapshot is rejected before any provider is restored.

## Qualification boundary
Soul.Integration.Vertical.RBSaveScopedDomains exercises real campaign and settlement providers: selected capture/restore; an unselected failing provider; unchanged strict all-domain behavior; selected-provider failure; invalid selections; missing selected payload before mutation.

The exact final source, including strengthened scoped-selection validation, passed **52/52 Soul tests**, with zero warnings, failures or not-run tests in 4.357649 seconds (`Evidence/automation_soul_qualified_final.log`; `Evidence/AutomationSoulQualifiedFinal/index.json`).

Actual disk persistence passed in Evidence/headless_campaign_saved_roundtrip.log:

- Normal campaign actions committed river_ford to orc_watch, then entered real Dragon Graveyard.
- Strategic pools 12/10, active cap 5; victory survivors 5/0; reinforcement waves 3/3; one RB Magic cast; 146 contacts; PBIL query/success/order counts 16/16/7; battle time 24.28 simulation seconds.
- Scoped RB Save wrote the disk file. Deliberately changed live location, army and XP were then restored by disk reload.
- Reload restored orc_watch, five knights and 500 XP; full campaign snapshot equality passed.
- Returned campaign held for 70.00 simulation seconds before SOUL_CAMPAIGN_ROUNDTRIP_PASS and requested exit status 0.

This first execution used NullRHI. Rendered G6 subsequently passed, as recorded below.

## Rendered G6 — PASS

After maintenance was paused, the same campaign action path and authoritative
save scope passed on default D3D12 SM5, 1280×720, development 30 FPS. Run:
`Evidence/VerticalRuns/G6_D3D12_campaign_roundtrip`.

Real Dragon Graveyard battle used pools 12/10, cap 5 per side, three waves per
side and one RB Magic cast. Victory survivors 5/0, 145 contacts, PBIL query /
success / order counts 17/17/8, battle 24.05 simulation seconds. The correct
target `orc_watch` returned as Dwarf Watch with five knights and 500 XP.

Disk save succeeded. Deliberately mutated live location, army and XP were
restored from the actual file; full snapshot equality passed. Native returned
campaign HUD was inspected. The campaign held 70.03 simulation seconds before
`SOUL_CAMPAIGN_ROUNDTRIP_PASS` and exit 0. Runner measured 139.23 seconds total,
71.72 seconds confirmed live after VERIFIED, with no GPU crash.

Preserved `Evidence/Persistence/rendered_victory.domain.rbsave`: 1,504 bytes,
SHA-256 `2981D715C93BB9E25A5DBA42FE99B1777791005DEEE1F11B88A611D0E2D612AF`.

Final post-strengthening regression is complete: **52/52 PASS**, including the
real-provider scoped capture/restore, selected failures and invalid-selection cases.

## Evidence
- Initial failure: Evidence/headless_campaign_roundtrip.log
- Caller selection log: SOUL_CAMPAIGN_SAVE_SCOPE domains=Soul.Campaign,Soul.Settlements
- Domain file: Saved/RBSave/Domains/Soul.VerticalCampaign.domain.rbsave

- Successful headless round trip: Evidence/headless_campaign_saved_roundtrip.log
- Preserved checkpoint: Evidence/Persistence/headless_victory.domain.rbsave (1504 bytes)
- SHA-256: D230DD0C38A3551F8DE2B90D4AC52233A76CDEB23B7B31FC61E6812629D2A931
