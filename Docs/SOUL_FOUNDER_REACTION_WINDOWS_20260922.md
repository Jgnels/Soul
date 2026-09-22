# Soul Founder Reaction Windows — 2026-09-22

Status: **policy-neutral evidence; no canonical reinforcement rule changed.**

The earlier founder assault sensitivity deliberately used a coarse day>1 reserve shortcut. This analysis resolves the actual action slots on the current graph.

| AP/day | North Pass revealed | Stronghold contact | Overnight window | Same-day reaction |
|---:|---|---|---|---|
| 2 | day 2 / action 1 | day 2 / action 2 | no | order_dependent |
| 3 | day 1 / action 3 | day 2 / action 1 | yes | yes |
| 4 | day 1 / action 3 | day 1 / action 4 | no | order_dependent |

## Findings

- **ap2_coarse_overstatement (correction)** — At 2 AP/day the attacker reveals North Pass on day 2 action 1 and reaches the Stronghold on day 2 action 2. The prior day>1 shortcut therefore overstates an overnight reinforcement window: there is no day boundary between threat reveal and contact.
- **ap3_true_overnight_window (info)** — At 3 AP/day the attacker reaches North Pass on day 1 action 3 and the Stronghold on day 2 action 1. This is the only tested AP value with an unambiguous overnight physical-reaction window.
- **ap4_same_day_rush (info)** — At 4 AP/day North Pass is reached on day 1 action 3 and the Stronghold on day 1 action 4. An Orc Watch reinforcement requires an explicit same-day reaction/interleaving rule or automatic adjacent support.
- **physical_relocation_opportunity_cost (design_test)** — If Orc Watch reinforcement is modeled as physical strategic relocation, the reserve cannot simultaneously remain at the outpost. That makes bypassing the Watch a meaningful trade: the defender may reinforce the Stronghold by vacating the forward position instead of duplicating strength.

## Candidate for the next prototype — not a product decision

Physical relocation over the real Orc Watch -> Stronghold edge; no reserve duplication. Overnight reaction is unambiguous; same-day reaction requires an explicit reaction/order rule.

It preserves graph causality and opportunity cost without granting teleport support, while making the remaining ambiguity small and directly testable in the campaign turn model.
