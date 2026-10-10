# AI Prisoner Behavior (V1)

**Lane:** 04_PRISONERS_RANSOM_DIPLOMACY_V1
**Source snapshot:** `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c`
**Scope:** How AI factions behave when they hold a captured hero, when their own hero is captured, and how this stays deterministic and non-degenerate.

Tags: **[VERIFIED]** VERIFIED FROM CURRENT SNAPSHOT · **[PROPOSAL]** DESIGN PROPOSAL · **[LOCAL]** LOCAL RUNTIME-ASSET CHECK REQUIRED.

---

## 1. Current AI reality (what exists)

**[VERIFIED]** `FSoulStrategyAI` (`Source/SoulCore/Public/SoulStrategyAI.h`, impl `SoulStrategyAI.cpp`) scores a fixed enum of actions: `Hold, Recover, DefendOwnedRegion, CaptureResourceRegion, AttackVisibleArmy, SeizeExposedRegion, SiegeSettlement`. It is deterministic (component sums + lexical tie-break `BetterCandidate`, `SoulStrategyAI.cpp:10-15`) and has **no awareness of heroes, captures, or diplomacy**. (Context confirmed: SoulStrategyAI contains no wounded/prisoner/diplomacy logic.)

**[VERIFIED]** In the shipped Heartland/alpha profiles the strategic AI is deliberately restrained: `AdvanceEnemyAI` reports "garrisons hold; no synthetic strategic army bypasses encounter resolution" and the six-faction profile reports strategic AI OFF (`SoulFounderPlaytestStateSubsystem.cpp` `AdvanceEnemyAI`, ~lines 657-659). So AI does not currently initiate diplomacy or ransom at all.

**[VERIFIED]** The AI captor relationship already exists in data: when the player's `Hero`/`DwarfCommander` is captured, `CaptorFaction` holds a canonical AI faction (`SoulHero.cpp:60`), persisted and validated (`SoulFounderPlaytestStateSubsystem.cpp` ~lines 858-861).

**Design consequence:** V1 AI prisoner behavior must be **reactive and preview-driven**, not a new autonomous strategic actor. The player initiates; the AI evaluates deterministically. This respects "do not create a second combat/diplomacy authority."

---

## 2. AI as captor (AI holds the player's / another faction's hero)

**[PROPOSAL]** The AI exposes exactly two passive behaviors, both pure functions, no turn-loop needed:

1. **Standing ransom demand.** For any hero it holds, the AI's demand is `FSoulRansomRules::ValueGold(level, kind, daysHeld, relation)` (see `ransom_scoring.json`). This is computed on demand when the player opens the Prisoners panel — it is NOT a stored offer the AI "sends," so it needs no AI tick and cannot desync from save state.
2. **Offer acceptance.** When the player proposes to pay, `FSoulRansomRules::EvaluateRansomOffer(...)` decides accept/reject deterministically (threshold 500, fully previewable before the player spends anything).

**[PROPOSAL]** AI captor reluctance factors (all deterministic, all in `EvaluateRansomOffer`):
- At **War** => `war_premium = -150` (AI reluctant to hand back an enemy asset cheaply).
- **Low relation** lowers acceptance (`relation = relationPermille/4`).
- **Staleness** raises acceptance over time (`+min(daysHeld,10)*10`) so a hero is never *permanently* unransomable — this is the primary anti-stalemate control.

**[PROPOSAL]** The AI never escalates, tortures, or executes (out of scope). It never raises the demand beyond `ValueGold`; the number only decays with captivity. This guarantees a monotone, finite path to release.

---

## 3. AI as owner (AI's hero captured by the player)

**[VERIFIED]** Today the only AI hero that can be captured is `DwarfCommander` (the single modeled NPC commander). Its captured state is fully persisted/validated (`SoulFounderPlaytestStateSubsystem.cpp` ~lines 718-723, 853-855).

**[PROPOSAL]** When the player holds an AI hero:
- The player may post a **ransom price to the AI owner** or offer a **prisoner exchange**. The AI *owner's* willingness to pay/trade uses the symmetric `EvaluateRansomOffer` from the owner's perspective (it values recovering its own commander highly: a captured commander is worth ~1 full re-hire, 1200g — see `ransom_scoring.json` anchors).
- **[PROPOSAL]** In V1 the AI owner does NOT spontaneously pay to recover its hero on its own turn (no autonomous spending), because the strategic AI is intentionally off in these profiles (**[VERIFIED]** §1). Instead the *player* drives the interaction and the AI responds. This is explicitly a V1 limitation recorded in Unknowns.

---

## 4. Anti-degeneracy rules (must-haves)

**[PROPOSAL]** The prisoner loop must not create the churn problems the broader AI design (Lane 2) worries about. Guardrails:

1. **No recapture churn.** A released hero returns `Wounded` (3-day clock) and cannot immediately re-enter battle (**[VERIFIED]** availability gate `FSoulHeroRules::IsAvailable`, `SoulHero.h:35`). So a hero cannot be freed, re-fought, and re-captured on the same day.
2. **Monotone decay guarantees termination.** `ValueGold` only ever decreases with days held (floor 60% after 10 days) and `EvaluateRansomOffer` staleness only increases acceptance. Therefore for any held hero there is a finite day by which a full-value offer is accepted — no infinite captivity, no infinite negotiation.
3. **One active process per hero.** `RansomProcess` is a single enum; a hero cannot be simultaneously in `OfferOpen` and `ExchangeProposed`. Opening a new process overwrites the old deterministically (preview shows this).
4. **Movement-gated like diplomacy.** Every accepted transaction costs 1 movement (`FSoulCampaignRules::SpendAction`, **[VERIFIED]** used by `ExecuteDiplomacy`), capping how many prisoner actions a player can spam per day (`MaxActionPoints=3`, **[VERIFIED]** `SoulCampaign.h`).
5. **No stranded commanders.** Because release always resolves to a valid owned/home/fallback region (`exchange_rules.md` §2, validated against `World.Regions`), a freed hero is never placed in a nonexistent or enemy-locked region.
6. **No suicide/farming.** Ransom gold flows owner->captor; a player cannot profit by losing heroes. Capturing an AI hero yields ransom income only if the AI pays, which in V1 it does not do autonomously, so there is no gold farm.

---

## 5. Determinism & test surface

**[PROPOSAL]** All AI prisoner decisions are pure functions of persisted state (hero level/kind, days held, relation, stance, treasuries) + current `Economy.Day`. No RNG, no seeds — identical to the diplomacy guarantee (**[VERIFIED]** `SoulDiplomacy.cpp:23`). This makes every behavior reproducible in a headless automation test using the existing `FHeartlandFixture` harness (**[VERIFIED]** `SoulHeartlandTests.cpp:11-16`). See acceptance tests in `HANDOFF.md`.

---

## 6. Explicit non-goals (out of scope per lane prompt)

- No torture / execution / interrogation.
- No dynasty / lineage / permadeath systems.
- No second strategic AI authority — behavior is reactive scoring attached to `FSoulRansomRules`, reusing `FSoulStrategyAI`'s component-sum + lexical-tiebreak style only as a stylistic template, NOT by modifying `FSoulStrategyAI`.
