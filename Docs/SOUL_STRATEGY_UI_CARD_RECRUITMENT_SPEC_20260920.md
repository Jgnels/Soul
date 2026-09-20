# Soul Strategy UI / Card / Recruitment Specification — 2026-09-20

## Authority and scope

This is the implementation-ready non-UE specification for Soul strategy UI, regiment cards, hero cards, town information, and in-world hero recruitment.

Inputs inspected: AGENTS.md; MECHANICS_V01; FACTION_CITY_AND_SIEGE_PLAN; SETTLEMENT_RECRUITMENT_MATRIX; STACK_DECISIONS; settlement_blueprints; environment_asset_bindings; and RefinedBadger asset-catalog records for Games By Hyper Hyper Mesh to Icon Creator v4.

Protected implementation areas were not touched: Source/, Content/, and active Soul maps.

## Locked product intent

Soul should read like a serious late-1990s/early-2000s PC fantasy strategy game rebuilt with realistic modern character sources. HOMM3 is a readability and portrait-identity reference, not an art-copy target.

A regiment card must keep three concepts separate: unit family/roster identity, persistent veterancy rank, and current tactical state.

Important authority boundary: the current Soul matrix defines seven roster families but does not authoritatively assign HOMM-style numeric tiers. The schema therefore uses stable family_slot now and exposes numeric_tier only when roster authority assigns it.

## Visual grammar

Portrait is identity. Frame is persistent rank. Temporary battle state is a compact overlay. Never bake rank, faction, text, or status into the portrait source.

Use blackened iron, aged wood, muted bronze, silvered steel, restrained heraldic engraving, and antique gilt. Avoid Total War chevrons, modern mobile rarity gradients, neon legendary glows, and anime/JRPG portrait language.

Faction identity belongs in a small heraldic inlay/sigil and nameplate geometry, not a full-card recolor.

## Small battlefield card — 192x256 design target

Always visible:
- portrait/silhouette;
- regiment count upper-left as the largest numeric element;
- aggregate HP/state bar along the bottom;
- initiative token upper-right, shared with the initiative timeline;
- veterancy through frame evolution;
- role glyph lower-left; range pip only for ranged/caster when useful;
- retaliation ready/spent lower-right.

Conditional:
- morale only when non-neutral, changed, or event-relevant;
- luck only when non-neutral, changed, or event-relevant;
- buffs/debuffs: maximum three icons plus +N overflow;
- at most one signature/passive ability marker.

Never place exact attack, defense, damage range, resistances, movement, full ability prose, or long status strings on the small card.

Selection/current-actor states should use a restrained inner keyline/notch or pulse. Exhausted/waited should change tactical overlay treatment, not destroy portrait legibility. Retaliation spent must be a shape/state change, not color-only.

## Hover / expanded card — 360x480

Show unit name, faction, family slot, numeric tier if canonical, veterancy rank, count, exact total HP and HP-per-member model, initiative, morale, luck, retaliation state, movement mode, role/range, compact attack/defense summary, up to four named major abilities, all active buffs/debuffs, and terrain/context modifier only when it currently matters.

Hover is the first place where exact numbers become explicit; it still must read in one glance.

## Regiment detail — 720x900

Show all hover data plus regiment name, veterancy XP/progress, bounded rank bonuses, casualty/replenishment history, upgrade/equipment state, recruitment/growth source, home settlement, notable battles, linked commander, and full ability text.

Ordinary soldiers do not gain subjective memory. Regiment history is factual. Subjective memory remains commander-only per Soul authority.

## Veterancy presentation

| Rank | Frame treatment |
|---|---|
| Recruit | dark iron/aged wood; plain inner bevel |
| Seasoned | cleaner iron/bronze edge; one engraved corner motif |
| Veteran | reinforced bronze/brass corners; small heraldic badge |
| Elite | silvered/blackened steel with faction inlay; restrained crest |
| Legendary | antique gilt over dark base; full restrained heraldic treatment; no glow spam |

Rank must remain legible in grayscale. Focus/hover exposes the rank name as text.

## Hero / Paragon cards

Heroes should not look like oversized regiment cards. Use a taller 420x560 card, larger chest-up commander portrait, dedicated nameplate, optional epithet, command-role crest, 0-3 spell-school seals, signature traits/abilities, equipment summary, and recruitment cost/conditions where relevant. Do not show a regiment-count badge in the hero identity area.

### Portrait capture contract

- real owned Paragon/character model after casting is authoritative;
- 70mm equivalent lens;
- chest-up framing; direct or no more than about 5 degrees three-quarter;
- eyes near upper third; eye-level camera;
- large soft 45-degree key, low neutral fill, subtle rim;
- locked exposure and white balance across the whole batch;
- neutral charcoal/brown studio matte or alpha; never environment scenery.

Cleanup before capture: remove non-canon modern gear, fix weapon/scabbard clipping, suppress oversized VFX, remove debug accessories, and keep at most one iconic weapon when it improves identity without obscuring face/torso.

One raw hero portrait should feed hero card, tavern inspect panel, diplomacy/commander UI, and notifications. Do not re-render a different face/camera for each surface.

## Bannerlord-style in-world hero recruitment

Primary recruitment is physical rather than a HOMM3 two-portrait modal.

Flow:
1. Enter the faction venue.
2. See 2-4 currently available recruitable heroes physically present.
3. Approach/interact with one.
4. Inspect identity, history, traits, command role, spell schools, cost, and conditions.
5. Recruit or decline.
6. On success, canonical availability changes and the hero leaves the venue/pool.

Optimization:
- recruitable heroes are real NPC actors;
- ordinary clutter remains static/merged/instanced;
- background patrons may use low-cost representation;
- no full civilian schedule or RPG simulation is required;
- only hero interaction, readable placement, and basic pathing need high fidelity.

Faction venues:
- Humans — Tavern / Inn: timber/stone common room, hearth, notice board, mercenary corner.
- Vikings — Mead Hall: long hearth hall, static benches/shields, recruits near fire/high-seat edge.
- Dwarves — Ale & Caravan Hall: stone trading/tap hall, casks and caravan ledgers, contract table.
- Orcs — War & Spoils Hall: occupied hall/camp pavilion, trophies/spoils static, challenge/contract space.
- Dark — Court of Oaths: crypt/sanctum antechamber, oath-bound champions, restrained ritual dressing.

Town view should show venue health/availability, candidate count, new-candidate timing if known, and a Visit Venue shortcut. Full candidate portraits should not be the primary recruitment experience.

## Town-view information architecture

The fixed-camera town view is a fast strategy surface over the same persistent settlement object.

One-click/focus access should include: settlement name/faction; relevant income/resources; construction/repair state; seven recruitment dwellings with growth/stock/disabled state; hero venue candidate count; walls/gates/siege damage; garrison summary; active settlement effects.

Each recruitment dwelling tile shows the family portrait/icon, available stock/growth timer, disabled/ruined state, and upgrade state if any. Do not show regiment veterancy on recruit stock.

## Tactical readability rules

1. Count and HP are separate: count tells bodies/creatures; HP tells condition.
2. Initiative uses the same token in card and timeline.
3. Neutral morale/luck are suppressed so deviations become salient.
4. Retaliation is always visible because it changes immediate choices.
5. Statuses cap at three icons plus overflow on the small card.
6. Exact math belongs on hover/detail.
7. Range appears only when it changes targeting expectations.
8. One signature ability marker may live on the small card; hover supports up to four named major abilities.
9. Flying/multi-hex use compact movement/role glyphs, not prose.
10. Status and rank cannot rely on hue alone.

## Existing mesh-to-icon/card work

Owned catalog evidence confirms Games By Hyper Hyper Mesh to Icon Creator v4. It is the preferred existing specialist path once locally available and qualified, before bespoke capture tooling.

No Hyper plugin is present in the current Soul Plugins tree, so ownership is not being mistaken for installation. No prior Soul unit-card or hero-card render set was found in the Soul checkout or targeted RefinedBadger card/portrait filename scan.

Generic Kenney/game-icons and Tiny Swords Tactics icons exist, but their mobile visual language is unsuitable for Soul. They may inform semantic glyph meaning only. Catalog evidence also shows an owned fantasy spell icon pack that may be useful for spell-school/ability glyphs after local/visual qualification.

| Existing work | Reuse score | Decision |
|---|---:|---|
| Hyper Mesh to Icon Creator v4 | 4/5 | preferred production path after local qualification |
| Existing Soul card renders | n/a | none found; no legacy dependency |
| Generic Kenney/game-icons glyphs | 2/5 | prototype semantics only |
| Tiny Swords Tactics UI icons | 1/5 | reject visual style for Soul |
| Owned fantasy spell-icon pack | 3/5 | candidate spell/ability glyph source |

## Exact render manifest

Machine-readable authority: Data/soul_card_render_manifest.json.

Core unit portraits: exactly 35 jobs = 5 enabled factions x 7 roster families. Raw unit output is 1024x1024 RGBA PNG. No frame, text, rank ornament, status, or faction UI is baked into the portrait. Humanoids use head/torso 3/4 framing at 55-70mm equivalent; beasts prioritize recognizable silhouette/face rather than a forced humanoid crop.

All 35 jobs intentionally leave numeric_tier and source asset binding unresolved until the roster/casting lane becomes authoritative.

Rank-frame art is five transparent layers: Recruit, Seasoned, Veteran, Elite, Legendary. Faction identity is five separate inlay/sigil layers. Runtime composition combines portrait + rank frame + faction inlay, avoiding 25 duplicated baked combinations.

Hero production is an exact template rather than invented names: every authoritative hero gets a 1536x2048 RGBA chest-up commander capture using hero_commander_v1. The hero job list remains blocked until final hero IDs/source models/roles are authoritative.

## Assets still needed / blockers

1. Final roster/casting bindings for all 35 unit portrait sources.
2. Final hero pool IDs, owned source models, costs/conditions, command roles, and spell schools.
3. Local qualification/install evidence for Hyper Mesh to Icon Creator v4 in a future UE lane.
4. Five faction sigil/inlay assets that read at small-card scale.
5. Five rank frame art layers following this spec.
6. Tactical glyph set: initiative, morale, luck, retaliation ready/spent, melee/ranged/support/beast, flying, multi-hex, range, and overflow.
7. Spell-school seals after Soul school taxonomy is locked.
8. Typography choice/license check for the serious late-90s/early-2000s PC fantasy direction.
9. Low-cost background-patron representation if live venues need population beyond recruitable heroes.

## Acceptance criteria

- exactly 35 stable unit render jobs, seven per enabled faction;
- family_slot and numeric_tier are separate;
- rank order is Recruit, Seasoned, Veteran, Elite, Legendary;
- small card includes count, HP/state, initiative, morale/luck behavior, retaliation, role/range, rank, and bounded status display;
- hover/detail contain the exact deeper fields omitted from the small card;
- hero card is structurally distinct from regiment card;
- tavern recruitment uses physical hero actors while clutter/background population stays cheap;
- town view exposes recruitment, repair/damage, garrison, hero-service availability, and settlement effects;
- render capture contract is deterministic enough for future batch production;
- no new render was performed in this non-UE lane;
- Source/, Content/, and active maps remain untouched.

## Implementation authority

RB UI/Input 1.0 is already admitted for controller-first CommonUI/Enhanced Input, remapping, focus, and accessibility. Future implementation should consume that authority for card/town/tavern navigation rather than creating a bespoke Soul input/focus stack.

STACK_DECISIONS also records that no Hyper runtime/tool product was discoverable on the upstairs machine during that pass. Therefore Hyper Mesh to Icon Creator v4 is an owned preferred specialist candidate, not a currently qualified local dependency.
