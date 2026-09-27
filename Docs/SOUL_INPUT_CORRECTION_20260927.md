# Campaign normal-input correction

Starting HEAD: `7a6b293`, branch `astra/soul-finalproject-consolidated-20260927`.

Reported packaged candidate003 evidence: T opens Human Capital; Escape closes it;
ordinary left clicks on Human Capital/Crossroads do not select or move. This worker
has not rerun that packaged candidate and does not modify the frozen RC.

The bound controller `PrimaryClick` now traces under the cursor on Visibility with
simple collision, accepts only a valid visible region actor with an ID, and calls
the existing campaign `HandleRegionClicked`. The actor's old click override was
removed: engine actor notifications can still occur, but cannot independently
dispatch campaign actions. This avoids timing/debounce state and preserves fast
distinct clicks. Cursor, capture and GameAndUI settings are unchanged.

Executed: `python -m unittest Tools.test_soul_campaign_input -v`: 3 static checks
pass (binding/trace/authority, sole dispatch, cursor/UI/collision contracts).
These checks do not execute Unreal input or establish runtime acceptance.

Required supervisor gates: UE compile/UHT; ordinary mouse/manual confirmation in
the new built candidate. Click Human Capital to open town; Escape closes; click
Crossroads to move exactly once and spend exactly one AP. Return to capital and
verify one click moves back without also opening town; a subsequent click opens
town. Verify terrain/non-region/hidden marker clicks have no campaign effect,
town-open clicks retain existing blocking behavior, and T/Escape still work.
Verify cursor remains visible and UI-consumed clicks do not leak through to the
map. Test repeated clicks and available supported viewport resolutions. Runtime
trace occlusion and input focus remain unverified until these checks run.
