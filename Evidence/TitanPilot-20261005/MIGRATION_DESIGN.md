# Clifftop migration gate

The World Partition external-actor exception remains narrow. A query gap is admitted only for a companion-covered external actor whose one concrete class is `/Game/Environment/.../Name.Name_C`. Its explicit class package must be present in the recursive closure, have complete native hard/all queries, and be a native `Engine.Blueprint` asset. Unknown, unresolved, native-only or non-environment classes do not receive this exception.

Class dependencies alone proved insufficient during runtime qualification: placed actors override materials independently of their Blueprint defaults. The native auditor therefore uses UE 5.8's exported `FPackageReader::ReadLinkerObjects` to read all external actors' serialized imports and soft package references without loading or executing donor objects. These edges join the same recursive graph and undergo the same forbidden/gameplay/plugin checks. Failed serialized reads cannot bypass the gate.

All map companion trees, nested maps, packages and sidecars remain explicit. Every source/destination file has a SHA-256 receipt. A supplement accepts previously copied files only when their verified receipt, current source hash and destination hash agree; it never rewrites those files. New files use exclusive creation. Unknown collisions, missing prior files, changed hashes, unreferenced packages and reserve violations stop transfer.

The initial class-only closure contained 343 files (152,342,330 bytes). Runtime exposed two overridden material references; native recursion identified three additional material instances totaling 42,822 bytes. The completed Clifftop closure contains 346 files (152,385,152 bytes). The original receipt and supplemental receipt are both retained.

The two foliage support exceptions remain exact native passive assets: `MPC_Player` is a MaterialParameterCollection and `RT_Player` a CanvasRenderTarget2D. The landscape allowlist admits only 35 referenced shader support assets (130,746,712 bytes); no landscape actor, streaming proxy or whole landscape directory is admitted.

Sulfur was not transferred. Its recursive environment Blueprint closure reaches `/Game/Characters/Main/Standard/oot_lostfang` and `/Game/Characters/SecondaryAnims/AS_Sitting_box`, and includes a missing `/Game/VFX/Environment/_Global/StylizedFire/Textures/T_CloudNoise`. It must be stripped in an authorized Soul copy and independently re-audited before migration; importing Characters to satisfy it is prohibited.

The reader uses a version-specific exported engine internal header. An engine upgrade requires rebuilding and rerunning native audit and regression tests before reusing this gate. A cached manifest is not authority to bypass new dependency failures.
