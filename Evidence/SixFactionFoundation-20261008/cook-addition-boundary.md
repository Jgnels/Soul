# Bounded Viking cook addition

The previously verified world cook is preserved. The new Viking axe, its material and four textures require a fresh cook; existing loose-fixture compatibility checks correctly reject pretending this is an unchanged cook.

The narrow cook uses Unreal's documented local `bShareMaterialShaderCode=False` packaging setting so the newly cooked material carries its own shader code. The override is command-line-only. It does not change project configuration or overwrite the world's shared shader libraries.

A fresh local loose stage receives only these six new packages and their sidecars. Existing packages cannot be overwritten. Its base asset registry is retained: the axe is an explicit native LoadObject dependency, not an AssetManager/discovery feature. The original UAT manifests are preserved; the addition has a separate hash manifest, checked before and after runtime.

This is candidate-specific functional cook/stage qualification. It is not a fresh full-world cook, a distributable archive or shipping promotion. Packaged rendering and exact weapon loads must pass before this boundary is accepted. A future distribution build should perform the normal complete clean cook with adequate disk space.
