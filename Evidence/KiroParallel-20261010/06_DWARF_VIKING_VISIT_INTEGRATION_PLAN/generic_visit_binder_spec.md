# Generic Visit Binder Spec — Removing Human-Only Visitation

Lane 6 — NEXT WALKABLE SETTLEMENTS: DWARF HOLD + VIKING HARBOUR
Source snapshot: `Jgnels/Soul` `handoff/soul-kiro-20261010` = `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c`
Noncanonical branch: `kiro/dwarf-viking-visit-integration-plan-20261010`

Evidence labels used throughout:
- **[VCS]** VERIFIED FROM CURRENT SNAPSHOT (read directly from the frozen commit)
- **[VOA]** VERIFIED OWNED ASSET METADATA (from `Jgnels/Copperlight-Asset-Catalog`)
- **[LRC]** LOCAL RUNTIME/ASSET CHECK REQUIRED (cannot be verified from cloud; needs a local Unreal editor/PIE pass — local Unreal evidence outranks any cloud guess here)
- **[DP]** DESIGN PROPOSAL (new; not present in the snapshot)

This spec is a *plan*. It touches **no** runtime `Source/`, `Config/`, `Content/`, `Soul.uproject`, maps or saves. All code excerpts are current-snapshot reads or proposed additive changes for the Codex/Unreal integration lane to make later.

---

## 1. What "visitation" is today (ground truth)

### 1.1 The transition into a visit is already faction-generic [VCS]

`ASoulFounderPlaytestCampaignActor::VisitSettlement()`
(`Source/Soul/Private/SoulFounderPlaytestCampaignActor.cpp:331-339`):

```cpp
void ASoulFounderPlaytestCampaignActor::VisitSettlement()
{
    if(bDiplomacyPanel)return;
    if (!State || !State->IsSettlementDevelopmentEnabled()) return;
    if (!ASoulSettlementVisitGameMode::CanVisit(State, LastMessage)) return;
    const FString Map = State->GetSettlementScenario()->OwnedEnvironmentMap.ToSoftObjectPath().GetLongPackageName();
    UGameplayStatics::OpenLevel(this, FName(*Map), true, TEXT("game=/Script/Soul.SoulSettlementVisitGameMode"));
}
```

This reads the **bound scenario's** `OwnedEnvironmentMap` and opens it with the visit GameMode.
It contains **no faction/settlement string literal**. It is already generic. The visit *entry* does not
need refactoring; the visit *gate* and the *walking embodiment* do.

### 1.2 The gate `CanVisit` is already faction-generic [VCS]

`ASoulSettlementVisitGameMode::CanVisit`
(`Source/Soul/Private/SoulSettlementVisitGameMode.cpp:44-67`) checks, in order:
settlement development ready; hero not Captured (only when `IsHeartlandEnabled()`); not mid-alpha-turn;
player is in the scenario region; region owner == player faction; settlement exists and is owned by the
player faction and in the player region; no pending battle / save busy; and the owned map starts with
`/Game/Soul/` and `FPackageName::DoesPackageExist(Map)`.

It references the scenario (`Scenario->SettlementId`, `Scenario->RegionId`) and the settlement authority,
**never** a hardcoded `human_capital`. **This gate is already generic.** [VCS]

### 1.3 The Human-only coupling is isolated to three code sites [VCS]

Every remaining Human-specific assumption lives in `SoulSettlementVisitGameMode.cpp`
and in scenario selection inside `SoulFounderPlaytestStateSubsystem.cpp`. They are:

| # | File:Line | Human-only coupling |
|---|-----------|---------------------|
| B1 | `SoulSettlementVisitGameMode.cpp:113-115` | Walk auto-start fallback timer gated on `State->GetDevelopmentRegion()==TEXT("human_capital")` |
| B2 | `SoulSettlementVisitGameMode.cpp:122-123` | On camera ready, `StartWalking()` is called only `if (SettlementId==TEXT("human_capital"))` |
| B3 | `SoulSettlementVisitGameMode.cpp:245-279` `StartWalking()` | Hardcoded Human player mesh (`ParagonAurora`), hardcoded tavern-forecourt entry probes `(-4350,2700,0)`, `(-3500,2700,0)`, `(-5000,1800,0)`, `(-9000,21000,0)` |
| B4 | `SoulSettlementVisitGameMode.cpp:144-220` `RefreshCompanion()` | Companion is hardcoded Rowan (Knight mesh `/Game/Knights_Pack/.../SK_Knight_04_Full_01`, idle `ThirdPersonIdle`), gated on `IsHeartlandEnabled()`, with street-sweep offsets tuned to the Human city layout |
| B5 | `SoulFounderPlaytestStateSubsystem.cpp:341-344` | Development scenario default is `DA_Soul_HumanCapital_DevelopmentProof` unless `-SoulDwarfSettlementProof` is passed; Heartland walk path only activates when `bHeartlandEnabled` (= `-SoulFourFactionAlpha -SoulHeartland`) and that path loads the Human proof |
| B6 | `SoulSettlementVisitGameMode.cpp:252` | `WalkIdle`/`WalkJog`/mesh are loaded via `LoadObject` on fixed `/Game/ParagonAurora/...` paths with no per-faction override |

`StartWalking()` returns `false` cleanly if the probe points miss or the mesh fails to load; in that
case the GameMode falls back to the authored **overview camera** via `ASoulTownViewAnchor` and keeps
management + return available. **So a Dwarf/Viking visit already works today as a camera overview** —
only *walking embodiment* is Human-only. [VCS]

---

## 2. Design goal

Remove the Human-only coupling by introducing a small, **data-driven per-settlement "Visit Profile"**
that the existing generic transition and gate consume. No second authority: the Visit Profile is an
immutable presentation input (exactly like `FSoulSettlementEnvironmentBinding` and
`USoulSettlementScenarioData`), never a save/encounter/ownership authority.

The binder must:
1. Let any registered, owned settlement open its authored map and present an overview camera (works today for Dwarf Hold). [VCS]
2. Make the *walking avatar*, *entry point(s)*, and *optional companion* data-driven per settlement/faction. [DP]
3. Degrade gracefully: if walk data is missing or probes fail → overview camera (current behaviour). [VCS]
4. Keep the siege/battle paths untouched (Human Capital Siege V0 is frozen per shared rules).

---

## 3. Proposed architecture: `FSoulSettlementVisitProfile` [DP]

### 3.1 Data home — reuse an existing immutable input, do not add a new mutable store

Two viable homes; recommended is **(A)** because it keeps presentation beside the environment binding
that the GameMode already loads for Heartland.

**(A) Extend the Environment Registry row** (`Data/SettlementEnvironments/EnvironmentRegistry.json`
+ `FSoulSettlementEnvironmentBinding` in `Source/Soul/Public/SoulSettlementEnvironmentRegistry.h`).
Add an optional `visit_profile` object per entry. The registry is already "immutable content bindings;
no ownership, save, or encounter authority" (header comment, `SoulSettlementEnvironmentRegistry.h:17`). [VCS]

**(B) Extend `USoulSettlementScenarioData`** (`Source/Soul/Public/SoulSettlementScenarioData.h`) with a
`FSoulSettlementVisitProfile VisitProfile` UPROPERTY. The scenario already carries `OwnedEnvironmentMap`
and miniature presentation fields, so a walk profile is thematically consistent there. [VCS]

Recommendation: **(A) for the per-map walk data** (entry points are map-space facts that belong with the
map binding), and a thin accessor so the GameMode can read it without a new subsystem. See
`visit_registry_extensions.json` for the exact proposed JSON shape and parse rules.

### 3.2 Proposed struct (additive; mirrors existing terse Soul style) [DP]

```cpp
// Immutable presentation input. No ownership, save, or encounter authority.
struct FSoulSettlementVisitAvatar
{
    FString Mesh;        // /Game/... skeletal mesh; empty => overview-only visit
    FString IdleAnim;    // /Game/... UAnimationAsset
    FString LocomotionAnim; // /Game/... forward jog/walk
    float   MaxWalkSpeed = 300.f;
    float   RunWalkSpeed = 500.f;
};

struct FSoulSettlementVisitCompanion
{
    FName  CompanionId;      // e.g. dwarf.<name>, viking.<name>; None => no companion
    FString Mesh;            // /Game/... skeletal mesh
    FString IdleAnim;        // /Game/...
    FString Label;           // on-world nameplate text, faction-flavoured
    bool    bRequiresHeartland = false; // keep Rowan behind Heartland; new factions default false
};

struct FSoulSettlementVisitProfile
{
    bool bWalkable = false;                 // false => overview camera only (safe default)
    TArray<FVector> EntryProbePoints;       // measured LOCAL authored-map ground candidates
    FRotator EntryFacingYaw = FRotator::ZeroRotator;
    FSoulSettlementVisitAvatar Avatar;
    FSoulSettlementVisitCompanion Companion; // optional
};
```

All `/Game/...` strings MUST be validated the same way the registry already validates maps
(`StartsWith("/Game/")` and existence) before use. Empty/absent avatar ⇒ overview camera. [DP]

### 3.3 Proposed GameMode refactor (replaces B1–B6) [DP]

Replace the Human-only branches with profile reads. Concretely, in
`ASoulSettlementVisitGameMode`:

- **B2/B1 → generic walkable check.** Replace
  `if (SettlementId==TEXT("human_capital")) StartWalking();`
  and the `GetDevelopmentRegion()==TEXT("human_capital")` fallback timer with
  `if (ActiveVisitProfile.bWalkable) StartWalking();` where `ActiveVisitProfile` is resolved once in
  `BeginPlay`/`RefreshPresentation` from the bound settlement id. [DP]

- **B3/B6 → data-driven avatar.** `StartWalking()` loads `ActiveVisitProfile.Avatar.Mesh / IdleAnim /
  LocomotionAnim` instead of fixed Paragon Aurora paths, and iterates
  `ActiveVisitProfile.EntryProbePoints` instead of the four hardcoded Human forecourt vectors. The
  physical ground line-trace + normal check (`ImpactNormal.Z > .75f`) and
  `AdjustIfPossibleButDontSpawnIfColliding` spawn handling are **kept verbatim** — they are the
  "never teleport through a wall" guarantee and are faction-agnostic. [VCS→DP]

- **B4 → data-driven companion.** `RefreshCompanion()` reads `ActiveVisitProfile.Companion`. Keep the
  entire street-sweep/sightline validation logic (it is generic and is the anti-clipping guarantee);
  only the mesh/anim/label/`bRequiresHeartland` gate become data. Rowan stays Human-only by setting
  `bRequiresHeartland = true` in the Human profile; Dwarf/Viking profiles omit a companion in V0. [VCS→DP]

- **No change** to `CanVisit`, `HandleAction`, `IsStreamingReady`, `TickDepthVisitQualification`, the
  return-to-campaign path, or the siege descriptor. [VCS]

### 3.4 Scenario-selection generalization (B5) [DP]

Today `InitializeScenario` hardcodes the development scenario:

`SoulFounderPlaytestStateSubsystem.cpp:341-344` [VCS]:
```cpp
auto* DevelopmentScenario = LoadObject<USoulSettlementScenarioData>(nullptr,
    bDwarfEnvironmentProof ? TEXT("/Game/Soul/Data/Settlements/DA_Soul_DwarfHold_DevelopmentProof")
        : TEXT("/Game/Soul/Data/Settlements/DA_Soul_HumanCapital_DevelopmentProof"));
```

For the **four-faction Heartland playtest** to allow visiting a Dwarf Hold or Viking Harbour the player
owns, scenario selection must resolve the development scenario from the **player's current settlement**,
not a compile-time constant. Minimum change: a `TMap<FName /*settlementId*/, FSoftObjectPath>`
development-scenario registry (data-driven), with the current two literals as its first two rows. This
is additive and preserves the existing proof flags. The Lane will NOT implement this — it is the
Codex/Unreal integration lane's job — but it is the critical dependency (see `dependency_checklist.json`).

---

## 4. Backward-compatibility contract [DP]

- Human Capital keeps today's exact behaviour by shipping a Human visit profile:
  `bWalkable=true`, the four current entry probes, Paragon Aurora avatar, Rowan companion with
  `bRequiresHeartland=true`.
- Dwarf Hold / Viking Harbour ship profiles with `bWalkable` only after **[LRC]** confirms a measured,
  physically clear entry point in the authored map. Until then they ship `bWalkable=false` and present
  the overview camera — which already works for Dwarf Hold today via `ASoulTownViewAnchor`. [VCS]
- Absolutely no change to encounter legality: `battle_enabled` and the siege origin remain under the
  Environment Registry's existing strict validation (`SoulSettlementEnvironmentRegistry.cpp:33-47`). [VCS]

---

## 5. Why this is the *smallest* generic binder

- It adds **one immutable data struct** and **profile reads**, deleting six string literals.
- It reuses the already-generic `VisitSettlement()` + `CanVisit()` + `ASoulTownViewAnchor` + the
  streaming/return machinery unchanged.
- It introduces **no** new subsystem, save domain, scheduler, or encounter authority (shared-rule safe).
- Graceful degradation means a half-finished Dwarf/Viking map still yields a working, shippable overview
  visit, so the feature can land incrementally.

See `dwarf_visit_plan.md`, `viking_visit_plan.md`, `visit_registry_extensions.json`,
`dependency_checklist.json`, and `HANDOFF.md`.
