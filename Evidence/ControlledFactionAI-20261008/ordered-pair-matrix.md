# Ordered encounter matrix

Attacker rows; defender columns. PASS evidence scopes are explicit in the JSON receipt.

| Attacker | humans | dwarves | orcs | vikings | nature | dark |
|---|---|---|---|---|---|---|
| humans | - | PASS | PASS | PASS | PASS | REJECT_UNSUPPORTED_ROSTER |
| dwarves | PASS | - | PASS | PASS | REJECT_OTHER_EXPLICIT_REASON | REJECT_UNSUPPORTED_ROSTER |
| orcs | PASS | PASS | - | PASS | REJECT_OTHER_EXPLICIT_REASON | REJECT_UNSUPPORTED_ROSTER |
| vikings | PASS | PASS | PASS | - | REJECT_OTHER_EXPLICIT_REASON | REJECT_UNSUPPORTED_ROSTER |
| nature | PASS | REJECT_OTHER_EXPLICIT_REASON | REJECT_OTHER_EXPLICIT_REASON | REJECT_OTHER_EXPLICIT_REASON | - | REJECT_UNSUPPORTED_ROSTER |
| dark | REJECT_UNSUPPORTED_ROSTER | REJECT_UNSUPPORTED_ROSTER | REJECT_UNSUPPORTED_ROSTER | REJECT_UNSUPPORTED_ROSTER | REJECT_UNSUPPORTED_ROSTER | - |

14 / 30 explicitly admitted. Full autonomous AI remains OFF.

Pair admission is necessary, not sufficient: no-approach or other action-specific rejection can still prevent a particular encounter.
