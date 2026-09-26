# Soul Founder Runtime Spatial Convergence — 2026-09-22

Status: **read-only migration analysis; accepted v2 coordinates remain authoritative.**

- Best fit: reflected 2-D similarity, uniform scale **93.800x**.
- RMS position error: **198.9 m** (3.04% of founder-map span).
- Worst region error: **337.1 m** (5.16% of span).
- Mean route-length delta: **151.3 m**.
- Worst route relative length error: **18.1%**.

## Migration conclusion

The existing founder runtime layout is structurally reusable, not disposable: it is essentially the accepted layout mirrored/scaled. However, the local deviations are too large to promote its hardcoded coordinates. Preserve the runtime interaction/camera concepts, but replace RegionPositions and route presentation geometry directly from the v2 import bundle.

## Largest position deviations

- ancient_shrine: 337.1 m after best-fit transform.
- orc_camp: 297.4 m after best-fit transform.
- river_ford: 267.4 m after best-fit transform.
- north_pass: 176.6 m after best-fit transform.

## Largest route-length deviations

- route.crossroads_old_quarry: 18.1% (+251.6 m).
- route.crossroads_river_ford: 16.3% (+243.0 m).
- route.orc_watch_river_ford: 13.9% (-280.4 m).
- route.orc_camp_orc_watch: 10.9% (-194.3 m).
