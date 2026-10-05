# Routed survivor conservation audit

The recovered battle implementation already records physical survivors separately from effective campaign survivors. In `SoulRealtimeBattleArena.cpp`, physical inventory is living actors plus reserves; a morale-defeated side has zero effective survivors because its force loses strategic cohesion. The campaign bridge commits that existing result. No battle, bridge, state or SoulCore source was changed for this correction.

The previous log auditor ignored the paired `physical` and `routed` fields and incorrectly demanded deaths plus effective routed survivors equal the initial force. It now validates physical casualty/reserve conservation first, validates both rout flags, then requires effective survivors to be zero for a routed side or equal physical survivors otherwise. Legacy logs without rout metadata retain their original strict conservation checks. Malformed/incomplete rout metadata is rejected.

Validation: all21 analyzer tests pass, including the exact routed victory, mirrored defeat, missing deaths/waves, malformed inventory/flags and unchanged nonrouted/legacy cases. The complete Tools suite also passes90 tests in this working tree.

The actual `Local/victory-r7-20fps/runtime/unreal.log` (SHA256 `4afb22d460b922708ade242a43d2cfbb3f5b108c094ea6d6f65f9997503eec9b`) has initial46/30, physical deaths12/27, active11/3, reserves23/0 and physical survivors34/3. Routed flags0/1 produce effective34/0, matching campaign return. Mana80→18 matches five casts. The corrected official analyzer returns0 with no issues or orphan events.

The original wrapper FAIL receipt remains intact. A separate `Local/victory-r7-20fps-reaudited/qualification-reaudited.json` combines the unchanged runtime input/return/save/clean-exit checks with the corrected log audit and records functional PASS. The20fps functional cap and83°C peak do not establish campaign performance or art acceptance.
