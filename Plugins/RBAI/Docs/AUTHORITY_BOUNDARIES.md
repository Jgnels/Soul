# Authority boundaries

RB AI owns tactical and behavioral **selection**. It intentionally does not become a second authority for systems the host already owns.

- **Perception:** UE AI Perception or the host owns sensory truth; RB AI consumes translated stimuli.
- **Navigation:** UE Navigation/StateTree/Behavior Tree or the host owns movement execution.
- **Combat:** the host or RB Combat owns attack feasibility, damage, guard, projectiles, death and consequences.
- **Items/equipment:** the host or RB Item Economy owns inventory and equipment truth.
- **Persistent civilian life:** RB Routine owns schedules, jobs, relationships, memory and offscreen population continuity.
- **Persistence orchestration:** the host, RB Save, or Foundation owns save ordering and durable commit policy.
- **Dialogue truth:** deterministic conversation/quest state belongs to the dialogue system; optional AI may select intent or phrasing only.

The core contract is:

`facts/context -> deterministic selection -> action request -> host execution/refusal -> result -> re-evaluate`

Predictive scores and candidate contexts are never authoritative world mutation.
