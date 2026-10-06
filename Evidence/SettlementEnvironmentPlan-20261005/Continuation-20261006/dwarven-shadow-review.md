# Owned Citadel lighting budget — native 1080p review

The map retains the original 289 local lights, authored intensity/color/attenuation and all existing shadows. `prepare_dwarven_shadow_budget.py` sets native `ShadowResolutionScale=0.5` on 205 shadow-casting fixtures whose radius is at most 30m. Larger architectural lights retain full resolution. The exact previous map is preserved at `Local/DwarfHold_before_shadow_budget.umap`; the mutation receipt contains its SHA256 and every affected actor.

The before/after runtime views are `Local/authored-fresh-r5-native1080/..._fresh_city.png` and `Local/authored-fresh-r6-shadows-native1080/..._fresh_city.png` under their `User/Saved/Screenshots` folders. Both were inspected at native 1920x1080. Masonry, pillars, floor relief, furnishings and the hall's light balance remain recognizable and closely matched. No replacement geometry, textures, lighting palette or missing shadow group was introduced. This is accepted as a bounded proof-map optimization, subject to broader moving-camera review before production admission.

| Native 1080p city window | Before | After |
|---|---:|---:|
| Mean ms | 30.123 | 27.070 |
| P95 ms | 34.935 | 32.876 |
| P99 ms | 38.663 | 36.941 |
| Average FPS | 33.20 | 36.94 |
| Frames below 30 / 40 / 60 | 83 / 995 / 996 | 49 / 815 / 1108 |
| Total sampled frames | 996 | 1108 |

Both windows use 20s uncapped warmup plus 30s sample, 100% primary/secondary scale, dynamic resolution off, VSync off. GPU captures independently show 1920x1080 scene input and output. This is a roughly 10% reduction in observed mean frame time, not a controlled laboratory guarantee. **The 40+ FPS city target remains unmet.**

The before run hit 85 C after the campaign sample and is thermally stopped overall. The after run completed cleanly at peak 84 C (city window peak 80 C); fresh restoration, physical/miniature state and visit/return pass. Its campaign proof view measures 8.781 ms mean, 9.811 P95, 10.386 P99. Peak device-wide VRAM is 2,551 MiB; process working set 4,227 MiB, private commit 5,805 MiB. City native LoadMap is 5.54s, excluding subsequent asset preparation.

Authoritative profiles: `dwarven-performance-r5-native1080-guarded.json` and `dwarven-performance-r6-shadows-native1080.json`. Older r2/r3/r4 profiles rendered 1400x788 into a 1080p output and must not be substituted for these native measurements. Fixed-view editor-game qualification is not cooked-package or whole-world performance acceptance.
