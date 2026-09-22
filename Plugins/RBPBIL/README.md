# RB PBIL

RB PBIL is RefinedBadger Studios' stable spatial-reasoning layer.
Games and RB systems should depend on PBIL concepts rather than raw backend APIs.

Current backend: TCAT 1.0.2, source-ported and qualified on Unreal Engine 5.8.2.

Initial semantic channels:
- Threat
- Congestion
- Objective
- MagicHazard
- Retreat
- FlankOpportunity
- FriendlySupport
- TerrainValue

Initial API provides semantic influence sources, a PBIL battlefield-volume preset,
and immediate semantic queries. TCAT remains replaceable behind this facade.

First proving ground: Soul real-time fantasy battles. Copperlight is a planned
consumer for ecological pressure, migration, danger, and habitat reasoning.
