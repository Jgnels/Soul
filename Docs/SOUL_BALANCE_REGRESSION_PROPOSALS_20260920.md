# Soul Campaign Balance Regression Proposals — 2026-09-20

These are proposed production regressions discovered by the non-UE balance lab. They are not runtime changes in this branch.

## High-priority mechanics regressions

1. **Repair has a real campaign cost.** A ruined recruitment building must not return to operational state through a zero-cost, zero-time call. A zero repair amount must leave a ruined building ruined.
2. **Campaign day advances once globally.** Advancing two or more settlements for one campaign day must increment `Economy.Day`, daily income, and weekly boundary state exactly once.
3. **Recruitment pools are local.** Two settlements with the same unit family must retain independent availability, growth, damage gating, and recruitment consumption.
4. **Destroyed recruitment dwelling stops both growth and recruiting.** Existing stock must not bypass a ruined required building; growth resumes only after the intended operational repair threshold.
5. **Capstone prerequisites must be operational.** A prerequisite at effectively zero integrity must not unlock a capstone merely because its condition enum is `Damaged`.
6. **Town construction concurrency is bounded by an explicit rule.** Starting several expensive prerequisite buildings on the same day should either be intentionally legal and tested or prevented by a slot/commitment rule.

## Strategic AI regressions

7. **Low readiness affects exposed-region seizure.** At very low readiness, `SEIZE_EXPOSED_REGION` must include the same recovery tradeoff expected by the campaign AI design; a trivial exposed target must not categorically outrank recovery at zero readiness.
8. **Memory changes but does not dictate decisions.** Repeated grounded defeats against the same rival should saturate at the configured caution cap; a sufficiently favorable force edge should still allow attack.
9. **Memory decays and is rival-specific.** Old losses and losses against another commander must not permanently depress unrelated attack candidates.

## Siege regressions

10. **Starvation changes meaningful state.** Repeated encirclement must eventually change surrender pressure, combat effectiveness, readiness, attrition, or another player-visible state; supply may not sit at a floor forever with no consequence.
11. **Waiting costs the attacker too.** Encirclement must consume attacker supply/readiness/resources or strategic opportunity in a deterministic way.
12. **Aftermath has economic consequences.** Persisted wall/building damage must feed repair cost/time and any intended temporary income/recruitment disruption.
13. **Immediate assault remains a real alternative.** The AI should sometimes assault before starvation when force edge, time pressure, or attacker sustainment makes waiting worse.

## Snowball / recovery regressions

14. **Veterancy remains challengeable.** A Legendary regiment must have a bounded power premium over an otherwise identical Recruit formation; morale extra-action effects must be included in the bound, not only the direct damage bonus.
15. **Major-army loss has an explicit recovery path.** Define and test the replacement/retinue/recruitment rule rather than relying on an implicit respawn assumption.
16. **Resource lead does not compound without counterplay.** Capturing two income regions early should create an advantage, not an automatic irreversible campaign state.
17. **Map scale changes time-to-contact, not whether the AI can function.** Small, medium, and large maps must all produce legal movement/commitment choices and eventual strategic contact under representative layouts.

## Determinism

18. Same seed + same initial state + same commands/candidate inputs produces identical action sequence, battles, memories, recruitment, and final digest.
19. Stable tie-breaks remain action-order then target-id deterministic.
20. Balance evidence must record seed, parameter set, source commit, donor reference commit, and lab/runtime authority boundary.

## Recovery exploit regressions

21. **Annihilation is never a beneficial reset.** Losing the best army must not improve expected strategic position by refreshing logistics, preserving destroyed-regiment veterancy, or enabling a same-day full recruitment refill.
22. **Replacement-army identity is explicit.** If a regiment is annihilated, define whether its veterancy dies with it; do not accidentally carry persistent regiment XP into a newly created replacement formation.
23. **Replacement logistics are explicit.** A replacement retinue's supply/readiness/fatigue state must be part of the rule and deterministic.
24. **Recovery timing is measurable.** Track days to 70% of pre-loss field strength plus territory/economic position; do not use final-day strength alone as the comeback metric.
