# Soul Founder Import Bundle v2 — 2026-09-22

Status: **FOUNDER_IMPORT_READY_NON_UE**

This consolidates the founder geography, settlement, surface, visual-anchor, route, directed-approach, battle-handoff, and presentation-state contracts into one versioned import package.

- Regions: 9 / 9.
- Routes: 10 / 10.
- Directed approaches: 20 / 20.
- Presentation states: 30.
- Settlement slots carried into founder import: 3.

## Import order

1. Regions.
2. Routes/splines.
3. Directed approach and battle-launch metadata.
4. Presentation-state fixtures.

The JSON manifest embeds the same enriched region/route/approach data and SHA-256 pins every source and CSV output. Unreal remains presentation/import authority only; strategic rules stay in SoulCore and weather stays in RBWeather.
