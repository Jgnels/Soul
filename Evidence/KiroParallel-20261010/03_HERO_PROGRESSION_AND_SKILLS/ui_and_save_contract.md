# Soul Lane 3 — UI & Save Contract

Snapshot authority: `handoff/soul-kiro-20261010` @ `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c` (VERIFIED resolves).

## 1. Save authority (VERIFIED FROM CURRENT SNAPSHOT) — DO NOT fork

There is exactly **one** campaign save authority. Lane 3 adds **no** second save authority.

- Domain id: `Soul.Campaign` (`GetRBSaveDomainId_Implementation`), schema version **1** (`GetRBSaveSchemaVersion_Implementation`).
- Implemented by `USoulFounderPlaytestStateSubsystem` via `IRBSaveDomainProvider`:
  - `CaptureRBSaveDomain_Implementation(FRBSaveDomainState& Out, FString& Error)`
  - `RestoreRBSaveDomain_Implementation(const FRBSaveDomainState& In, FString& Error)`
- Transport: a single JSON string field `CampaignJson` inside `FRBSaveDomainState`.
- Settlement state is a **separate** domain (`Soul.Settlements`, `USoulSettlementStateSubsystem`) — Governor/stewardship effects write there, not into `Soul.Campaign`.

### Hero fields currently serialized (VERIFIED)
From `CaptureRBSaveDomain_Implementation`:
```
"hero_kind" : int (ESoulHeroKind)
"hero"      : { "level", "xp", "points", "mana", "max_mana" }   // int map
"skills"    : { <skillId> : <rank> }                            // int map  (FSoulHeroState::Skills)
"spells"    : [ <spellId>, ... ]                                // name set (KnownSpells)
```
Heartland-only extra block (when `bHeartlandEnabled`):
```
"heartland_companion_assigned" : bool          // bCompanionAssigned
"heartland_hero_condition"     : int           // Hero.Condition
"heartland_recovery_days"      : int           // Hero.RecoveryDays
"heartland_captor"             : string
"heartland_capture_region"     : string
"heartland_dwarf_commander"    : { "condition","recovery","captor","region" }  // DwarfCommander
```

### Restore safety contract (VERIFIED — must be preserved)
- Restore reads into **temporary locals** and only commits if the whole snapshot validates ("invalid snapshots must not partially mutate authority"). Lane 3 additions MUST follow this all-or-nothing pattern.
- Optional numeric receipt fields (`result_mana`, `hero_kind`) are validated: a field present but out of range (negative, fractional, > INT32, or an undefined enum ordinal) **rejects the whole restore atomically**, leaving the existing checkpoint byte-identical. (VERIFIED by `SoulCampaignManaTests` → `FSoulCampaignResourceReceiptTest`.)
- A **legacy** checkpoint missing `result_mana`/`hero_kind` still loads, and the hero defaults to `Kind=Hero` (does not retain a previous paragon). (VERIFIED.)
- `ArtifactSlots` (declared on `FSoulHeroState`) is currently **NOT serialized** — see unknowns.

## 2. How Lane 3 extends the save WITHOUT a schema bump (DESIGN PROPOSAL)

Follow the exact legacy-tolerant pattern: new keys are **optional-with-default**, validated the same way, so schema stays at **v1** and old saves keep loading.

Proposed additive keys (all optional; absence => safe default):
```
"hero_assignment"      : string   // AssignmentRole; absent => "ArmyCompanion"  (only if roles ship)
"heartland_companion"  : { "level","xp","points","mana","max_mana","condition","recovery","captor","region" }
                                   // Rowan as a real FSoulHeroState; mirrors heartland_dwarf_commander exactly
```
Validation rules Codex must apply (parallels existing receipt validation):
- Each numeric is a non-negative int ≤ INT32 (level ≥ 1 on commit).
- `condition` must be a valid `ESoulHeroCondition` ordinal (0..2) or reject.
- `hero_assignment` must be one of the known role names or reject.
- All reads into temporaries; commit only on full success.

**A schema bump to v2 is required ONLY if** a new field becomes **mandatory** (i.e. an old save without it must be refused). Lane 3 recommends staying at v1 by keeping everything optional-with-default.

## 3. UI surface (VERIFIED FROM CURRENT SNAPSHOT)

Founder playtest HUD (`Source/Soul/Private/SoulFounderPlaytestHUD.cpp`) + summary text:

- `BuildSummary()` / HUD prints: `Day D | AP a/max | Gold g | Troops t | Hero L<level> XP <xp>` — hero **Level and XP are already shown**.
- Companion controls (VERIFIED): a `Hire` button (`[H] Hire companion / 1200 gold`) when tavern operational and not hired; a `CompanionAssign` button showing `CompanionStatus()` when Heartland + hired; muted text otherwise.
- Skill selection is driven by `ChooseSkill(FName)`; HUD exposes the two live skills (`Adventure`, `Magic`).
- Battle-side: `SoulRealtimeBattleArena` exposes a spell bar, `PlayerManaValue()`, spell icons, and spell block reasons (e.g. "Affinity / Mage Guild locked"). Hero mana and learned spells drive the battle UI; the campaign owns the mana balance.

## 4. UI additions (DESIGN PROPOSAL)

- A compact **hero sheet** panel: Level, XP (current / to-next from `ExperienceForLevel(Level+1)`), UnspentSkillPoints, per-domain skill ranks, Mana/MaxMana, Condition (Healthy/Wounded N days/Captured by X), Assignment role, archetype label.
- Skill spend buttons: one per unlocked skill id, enabled only when `UnspentSkillPoints>0` and rank < that skill's cap. Reuse `ChooseSkill` (widen it to the new ids + per-skill cap; keep legacy ids working).
- Companion/Governor assignment: extend the existing `CompanionAssign` button into a small role picker (ArmyCompanion / Independent / Governor) gated by `IsAvailable`.
- All UI is **presentation only** — it reads canonical `FSoulHeroState` and calls existing rules; it invents no values (AGENTS.md).
- **VERIFIED OWNED ASSET METADATA:** Kenney UI is already the project UI kit (shared baseline); reuse it for the hero sheet. Specific hero portrait art is **LOCAL RUNTIME/ASSET CHECK REQUIRED** — do not bind a `/Game` portrait path from cloud guesses.

## 5. Explicit unknowns
- `FSoulHeroState::ArtifactSlots` exists but is unserialized and unused. If artifacts are ever wired, they will need an optional `"artifacts"` array under the same v1-tolerant rules. Flagged, not designed here.
- **LOCAL RUNTIME/ASSET CHECK REQUIRED:** confirm the HUD can render a multi-row skill panel within the current founder HUD layout budget.
