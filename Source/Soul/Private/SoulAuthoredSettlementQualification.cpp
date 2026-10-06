#include "SoulAuthoredSettlementQualification.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#endif
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulFounderPlaytestCampaignActor.h"
#include "SoulFounderPlaytestGameMode.h"
#include "SoulSettlementVisitGameMode.h"
#include "SoulSettlementScenarioData.h"
#include "SoulSettlementStateSubsystem.h"
#include "SoulSettlementBuildingActor.h"
#include "SoulCampaignCamera.h"
#include "SoulCampaignTerrain.h"
#include "RBSaveSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "UObject/Package.h"

bool USoulAuthoredSettlementQualification::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    return FParse::Param(FCommandLine::Get(), TEXT("SoulAuthoredSettlementQualification"));
#endif
}

bool USoulAuthoredSettlementQualification::Check(bool Condition, const FString& Label)
{
    UE_LOG(LogTemp, Display, TEXT("SOUL_AUTHORED_CHECK step=%d pass=%d %s"), Step, Condition, *Label);
    if (!Condition)
    {
        UE_LOG(LogTemp, Error, TEXT("SOUL_AUTHORED_SETTLEMENT_FAIL step=%d %s"), Step, *Label);
        bDone = true;
        FPlatformMisc::RequestExitWithStatus(false, 1);
    }
    return Condition;
}

bool USoulAuthoredSettlementQualification::Snapshots(FString& Campaign, FString& Settlement)
{
    auto* C = GetGameInstance()->GetSubsystem<USoulFounderPlaytestStateSubsystem>();
    auto* S = GetGameInstance()->GetSubsystem<USoulSettlementStateSubsystem>();
    FRBSaveDomainState CD, SD;
    FString Error;
    if (!Check(C && S && C->CaptureRBSaveDomain_Implementation(CD, Error)
        && S->CaptureRBSaveDomain_Implementation(SD, Error) && CD.DomainId == TEXT("Soul.Campaign")
        && SD.DomainId == TEXT("Soul.Settlements") && CD.Fields.Num() == 1 && SD.Fields.Num() == 1,
        TEXT("capture both existing RBSave domains: ") + Error)) return false;
    Campaign = CD.Fields[0].StringValue;
    Settlement = SD.Fields[0].StringValue;
    return true;
}

bool USoulAuthoredSettlementQualification::Remember(const TCHAR* Label)
{
    if (!Snapshots(ExpectedCampaign, ExpectedSettlement)) return false;
    const FString Base = FPaths::ProjectSavedDir() / (Prefix + TEXT("_") + Label);
    return Check(FFileHelper::SaveStringToFile(ExpectedCampaign, *(Base + TEXT("_Soul.Campaign.json")))
        && FFileHelper::SaveStringToFile(ExpectedSettlement, *(Base + TEXT("_Soul.Settlements.json"))), TEXT("expected domain snapshots recorded"));
}

bool USoulAuthoredSettlementQualification::Matches()
{
    FString C, S;
    return Snapshots(C, S) && Check(C == ExpectedCampaign && S == ExpectedSettlement, TEXT("both restored domains exactly match checkpoint"));
}

bool USoulAuthoredSettlementQualification::Presentation(bool bBuilt, bool bFullEnvironment)
{
    auto* C = GetGameInstance()->GetSubsystem<USoulFounderPlaytestStateSubsystem>();
    const auto* Scenario = C->GetSettlementScenario();
    if (bFullEnvironment)
    {
        int32 AuthoredActors = 0;
        for (TActorIterator<AActor> It(GetWorld()); It; ++It) ++AuthoredActors;
        if (!Check(AuthoredActors >= 3000, FString::Printf(TEXT("substantial authored city loaded: %d actors"), AuthoredActors))) return false;
    }
    int32 Groups = 0;
    for (TActorIterator<ASoulSettlementBuildingActor> It(GetWorld()); It; ++It)
    {
        if (It->SettlementId != Scenario->SettlementId || It->BuildingId != C->GetTavernBuildingId()) continue;
        ++Groups;
        if (!Check(!It->IsHidden() && It->bFollowSettlementState, TEXT("state adapter visible and bound to existing authority"))) return false;
        if (bFullEnvironment)
        {
            if (!Check(It->IntactActors.Num() == 56, TEXT("reviewed 56 persistent authored roots loaded"))) return false;
            TArray<AActor*> Pending;
            TSet<AActor*> Visited;
            for (AActor* Actor : It->IntactActors) Pending.Add(Actor);
            while (!Pending.IsEmpty())
            {
                AActor* Actor = Pending.Pop(EAllowShrinking::No);
                if (Visited.Contains(Actor)) continue;
                Visited.Add(Actor);
                if (!Check(IsValid(Actor) && Actor->IsHidden() == !bBuilt, TEXT("physical root/child visibility: ") + GetNameSafe(Actor))) return false;
                TArray<AActor*> Children;
                Actor->GetAttachedActors(Children);
                Pending.Append(Children);
            }
        }
        else
        {
            int32 Base = 0, Upgrade = 0;
            TArray<UStaticMeshComponent*> Components;
            It->GetComponents(Components);
            for (auto* Component : Components)
            {
                if (Component->GetStaticMesh() == Scenario->MiniatureBaseMesh.Get())
                {
                    ++Base;
                    if (!Check(Component->IsVisible() && !Component->bHiddenInGame, TEXT("starting miniature base visible"))) return false;
                }
                if (Component->GetStaticMesh() == Scenario->MiniatureUpgradeMesh.Get())
                {
                    ++Upgrade;
                    if (!Check((Component->IsVisible() && !Component->bHiddenInGame) == bBuilt, TEXT("miniature upgrade visibility matches authority"))) return false;
                }
            }
            if (!Check(Base == 1 && Upgrade == 1, TEXT("exactly one owned base and upgrade derivative"))) return false;
        }
    }
    return Check(Groups == 1, TEXT("exactly one settlement representation in the loaded world"));
}

void USoulAuthoredSettlementQualification::Key(FKey K)
{
    if (auto* PC = GetWorld()->GetFirstPlayerController())
    {
        UE_LOG(LogTemp, Display, TEXT("SOUL_AUTHORED_INPUT step=%d key=%s"), Step, *K.ToString());
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, IE_Pressed, 1));
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, IE_Released, 0));
    }
}

void USoulAuthoredSettlementQualification::Capture(const TCHAR* Label)
{
    const FString Path = FPaths::ProjectSavedDir() / TEXT("Screenshots") / (Prefix + TEXT("_") + Label + TEXT(".png"));
    Captures.Add(Path);
    FScreenshotRequest::RequestScreenshot(Path, true, false);
}

void USoulAuthoredSettlementQualification::Next(double Delay)
{
    ++Step;
    AssetsQuietSince = 0;
    StepStarted = FPlatformTime::Seconds();
    NextTime = StepStarted + Delay;
}

bool USoulAuthoredSettlementQualification::CaptureWithoutHUD(const TCHAR* Label, double Now)
{
    auto* HUD = GetWorld()->GetFirstPlayerController()->GetHUD();
    if (!Check(HUD != nullptr, TEXT("existing HUD available for unobstructed evidence"))) return false;
    if (!bCleanCapturePending)
    {
        bSavedHUDVisible = HUD->bShowHUD;
        HUD->bShowHUD = false;
        Capture(Label);
        bCleanCapturePending = true;
        NextTime = Now + 1;
        return false;
    }
    HUD->bShowHUD = bSavedHUDVisible;
    bCleanCapturePending = false;
    return true;
}

bool USoulAuthoredSettlementQualification::AwaitVisualAssets(double Now)
{
#if WITH_EDITOR
    const int32 Pending = FAssetCompilingManager::Get().GetNumRemainingAssets();
    if (Pending > 0)
    {
        AssetsQuietSince = 0;
        if (Now >= NextAssetWaitLog)
        {
            UE_LOG(LogTemp, Display, TEXT("SOUL_AUTHORED_ASSET_WAIT step=%d remaining=%d"), Step, Pending);
            NextAssetWaitLog = Now + 10;
        }
        if (Now - StepStarted > 300) Check(false, TEXT("asset compilation did not finish within the visual evidence deadline"));
        NextTime = Now + .5;
        return false;
    }
#endif
    if (AssetsQuietSince == 0) AssetsQuietSince = Now;
    if (Now - AssetsQuietSince < 2) { NextTime = Now + .25; return false; }
    return true;
}

bool USoulAuthoredSettlementQualification::MeasureView(const TCHAR* Label)
{
    const double Now = FPlatformTime::Seconds();
    if (MeasureStarted == 0)
    {
        auto* Cap = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"));
        auto* VSync = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync"));
        // Loading/input checks use the safe functional cap. Only the entire
        // 20-second warmup and 30-second measured window run uncapped.
        if (Cap) Cap->Set(0.f, ECVF_SetByConsole);
        int32 Width = 0, Height = 0;
        GetWorld()->GetFirstPlayerController()->GetViewportSize(Width, Height);
        if (!Check(Cap && VSync && Cap->GetFloat() == 0 && VSync->GetInt() == 0
            && Width == 1920 && Height == 1080, TEXT("performance run is uncapped, VSync off, actual 1920x1080"))) return false;
        MeasureStarted = Now;
        MeasureStartedUTC = FDateTime::UtcNow().ToIso8601();
        FrameSamples.Reset();
        UE_LOG(LogTemp, Display, TEXT("SOUL_AUTHORED_PERFORMANCE_BEGIN view=%s warmup=20 sample=30"), Label);
        return false;
    }
    const double Age = Now - MeasureStarted;
    if (Age < 20) return false;
#if WITH_EDITOR
    if (FAssetCompilingManager::Get().GetNumRemainingAssets() != 0)
    { Check(false, TEXT("asset compilation resumed during the measured window")); return false; }
#endif
    if (Age < 50) { FrameSamples.Add(FrameMs); return false; }
    if (!Check(FrameSamples.Num() >= 30, TEXT("performance window contains real frame samples"))) return false;
    FString CSV(TEXT("frame,wall_ms\n"));
    double Sum = 0;
    int32 Below30 = 0, Below40 = 0, Below60 = 0;
    for (int32 I = 0; I < FrameSamples.Num(); ++I)
    {
        const double Frame = FrameSamples[I];
        CSV += FString::Printf(TEXT("%d,%.6f\n"), I, Frame);
        Sum += Frame; Below30 += Frame > 1000. / 30.; Below40 += Frame > 25.; Below60 += Frame > 1000. / 60.;
    }
    FrameSamples.Sort();
    const int32 N = FrameSamples.Num();
    const FString Report = FString::Printf(TEXT("{\"view\":\"%s\",\"started_utc\":\"%s\",\"finished_utc\":\"%s\",\"warmup_seconds\":20,\"requested_sample_seconds\":30,\"sample_seconds\":%.6f,\"frames\":%d,\"mean_ms\":%.6f,\"p95_ms\":%.6f,\"p99_ms\":%.6f,\"below_30\":%d,\"below_40\":%d,\"below_60\":%d,\"viewport\":[1920,1080],\"max_fps\":0,\"vsync\":0,\"scope\":\"game-thread wall-clock frame intervals; separate GPU telemetry in runner\"}\n"),
        Label, *MeasureStartedUTC, *FDateTime::UtcNow().ToIso8601(), Sum / 1000., N, Sum / N,
        FrameSamples[FMath::Min(N-1, FMath::FloorToInt(N*.95))], FrameSamples[FMath::Min(N-1, FMath::FloorToInt(N*.99))], Below30, Below40, Below60);
    const FString Base = FPaths::ProjectSavedDir() / (Prefix + TEXT("_performance_") + Label);
    if (!Check(FFileHelper::SaveStringToFile(CSV, *(Base+TEXT(".csv")))
        && FFileHelper::SaveStringToFile(Report, *(Base+TEXT(".json"))), TEXT("uncapped performance raw samples and receipt written"))) return false;
    UE_LOG(LogTemp, Display, TEXT("SOUL_AUTHORED_PERFORMANCE_COMPLETE view=%s frames=%d mean_ms=%g"), Label, N, Sum/N);
    IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"))->Set(20.f, ECVF_SetByConsole);
    MeasureStarted = 0;
    return true;
}

void USoulAuthoredSettlementQualification::Tick(float DeltaTime)
{
    if (bDone || !GetWorld() || !GetWorld()->IsGameWorld() || !GetWorld()->HasBegunPlay()) return;
    const double Now = FPlatformTime::Seconds();
    FrameMs = PreviousFrame > 0 ? (Now - PreviousFrame) * 1000. : 0;
    PreviousFrame = Now;
    if (Started == 0) { Started = StepStarted = Now; NextTime = Now + 25; }
    if (Now - Started >= 1500) { Check(false, TEXT("bounded 25-minute qualification expired")); return; }
    if (Now < NextTime) return;
    auto* State = GetGameInstance()->GetSubsystem<USoulFounderPlaytestStateSubsystem>();
    auto* Authority = GetGameInstance()->GetSubsystem<USoulSettlementStateSubsystem>();
    auto* Save = GetGameInstance()->GetSubsystem<URBSaveSubsystem>();
    if (!(State && Authority && Save && State->bInitialized && State->IsSettlementDevelopmentReady()))
    { Check(false, TEXT("existing authorities initialized")); return; }
    if (State->bPersistenceBusy) return;
    auto* Visit = GetWorld()->GetAuthGameMode<ASoulSettlementVisitGameMode>();
    const bool Campaign = GetWorld()->GetAuthGameMode<ASoulFounderPlaytestGameMode>() != nullptr;
    const auto* Scenario = State->GetSettlementScenario();
    const FName Condition = Authority->GetBuildingConditionName(Scenario->SettlementId, State->GetTavernBuildingId());
    auto BeginSave = [&]()
    {
        SaveBefore.Reset();
        if (IFileManager::Get().FileExists(*SavePath)) FFileHelper::LoadFileToArray(SaveBefore, *SavePath);
        Key(EKeys::F5);
    };
    auto Saved = [&]()
    {
        TArray<uint8> Bytes;
        if (!Check(State->bLastSaveSucceeded && FFileHelper::LoadFileToArray(Bytes, *SavePath)
            && !Bytes.IsEmpty() && Bytes != SaveBefore, TEXT("F5 completed and changed real checkpoint bytes"))) return false;
        return Check(FFileHelper::SaveArrayToFile(Bytes, *(FPaths::ProjectSavedDir() /
            FString::Printf(TEXT("%s_checkpoint_step%d.rbdomains"), *Prefix, Step))), TEXT("actual checkpoint bytes retained for fresh-process restoration"));
    };
    auto BeginLoad = [&]() { ExpectedLoadRevision = State->CampaignLoadRevision + 1; Key(EKeys::F9); };
    auto Loaded = [&]() { return Check(State->bLastLoadSucceeded && State->CampaignLoadRevision == ExpectedLoadRevision,
        TEXT("new F9 callback completed")) && Matches(); };
    auto WaitVisit = [&]()
    {
        if (Visit && Visit->IsVisitReady()) return Check(GetWorld()->GetOutermost()->GetName() == Scenario->OwnedEnvironmentMap.ToSoftObjectPath().GetLongPackageName(), TEXT("correct actual authored environment loaded"));
        if (Now - StepStarted > 180) Check(false, TEXT("visit did not become ready"));
        NextTime = Now + 1;
        return false;
    };
    auto WaitCampaign = [&]()
    {
        if (Campaign) return true;
        if (Now - StepStarted > 180) Check(false, TEXT("campaign return did not complete"));
        NextTime = Now + 1;
        return false;
    };
    if (LastLoggedStep != Step)
    {
        UE_LOG(LogTemp, Display, TEXT("SOUL_AUTHORED_STEP %d map=%s condition=%s day=%d"), Step, *GetWorld()->GetName(), *Condition.ToString(), State->Economy.Day);
        LastLoggedStep = Step;
    }
    // Editor-game can report gameplay readiness while meshes, textures and
    // shaders still show fallback surfaces. Their readiness is a separate gate.
    const bool bVisualGate = Step == 0 || Step == 1 || Step == 3 || Step == 6 || Step == 8
        || Step == 10 || Step == 15 || Step == 17 || Step == 101 || Step == 102
        || ((Step == 103 || Step == 105) && MeasureStarted == 0)
        || (Step == 23 && Campaign && bReturnedBattleViewWarmed);
    if (bVisualGate && !AwaitVisualAssets(Now)) return;
    switch (Step)
    {
    case 0:
    {
        FString UserDir;
        const TCHAR* Cmd = FCommandLine::Get();
        if (!Check(FParse::Param(Cmd, TEXT("SoulDwarfSettlementProof")) && SoulCampaignTerrain::EvilCorridor()
            && !FParse::Param(Cmd, TEXT("SoulWorldTerrain")) && !FParse::Param(Cmd, TEXT("SoulSettlementDevelopmentQualification"))
            && !FParse::Param(Cmd, TEXT("SoulVerticalQualification")), TEXT("isolated authored proof on retained terrain"))) return;
        if (!Check(FParse::Value(Cmd, TEXT("UserDir="), UserDir) && !FPaths::IsRelative(UserDir)
            && FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()), FPaths::ConvertRelativePathToFull(UserDir)), TEXT("isolated absolute UserDir protects player saves"))) return;
        if (!Check(FParse::Value(Cmd, TEXT("SoulCampaignCapturePrefix="), Prefix) && !Prefix.IsEmpty()
            && Prefix == FPaths::MakeValidFileName(Prefix), TEXT("explicit evidence prefix"))) return;
        SavePath = Save->GetDomainSlotPath(TEXT("Soul.VerticalCampaign"));
        if (FParse::Param(Cmd, TEXT("SoulAuthoredSettlementFreshLoad")))
        {
            if (!Check(IFileManager::Get().FileExists(*SavePath), TEXT("explicit fresh-load run has a copied qualification checkpoint"))
                || !Check(FFileHelper::LoadFileToString(ExpectedCampaign, *(FPaths::ProjectSavedDir()/TEXT("expected_Soul.Campaign.json")))
                    && FFileHelper::LoadFileToString(ExpectedSettlement, *(FPaths::ProjectSavedDir()/TEXT("expected_Soul.Settlements.json"))), TEXT("fresh-load expected snapshots supplied independently"))) return;
            if (!Check(Campaign && Condition == TEXT("Unbuilt") && !State->bSecondHeroHired, TEXT("new process starts with unbuilt scenario before F9"))) return;
            BeginLoad(); Step = 99; Next(); break;
        }
        if (!Check(!IFileManager::Get().FileExists(*SavePath), TEXT("fresh save slot required"))) return;
        if (!Check(Campaign && Condition == TEXT("Unbuilt") && !State->IsTavernOperational(), TEXT("campaign starts unbuilt with service locked")) || !Presentation(false, false)) return;
        InitialGold = State->Economy.Resources.FindRef(TEXT("gold"));
        Key(EKeys::Home);
        for (int32 I = 0; I < 5; ++I) Key(EKeys::MouseScrollUp);
        Next(8); break;
    }
    case 1:
        Capture(TEXT("miniature_start"));
        if (!Remember(TEXT("start"))) return;
        BeginSave(); Key(EKeys::T); Next(); break;
    case 2:
        if (!Saved()) return;
        Key(EKeys::V); Next(20); break;
    case 3:
        if (!WaitVisit() || !Presentation(false, true) || !Matches()) return;
        Capture(TEXT("city_start")); Next(); break;
    case 4:
        if (!CaptureWithoutHUD(TEXT("city_start_clean"), Now)) return;
        Key(EKeys::U); Next(); break;
    case 5:
        if (!Check(Condition == TEXT("Building") && State->Economy.Resources.FindRef(TEXT("gold")) == InitialGold - 200
            && !State->IsTavernOperational(), TEXT("actual U input purchased two-day upgrade once")) || !Presentation(false, true)) return;
        Key(EKeys::SpaceBar); Next(); break;
    case 6:
        if (!Check(Condition == TEXT("Building") && State->Economy.Day == 2 && !State->IsTavernOperational(), TEXT("one elapsed day does not complete construction"))) return;
        Capture(TEXT("city_mid"));
        if (!Remember(TEXT("mid"))) return;
        BeginSave(); Next(); break;
    case 7:
        if (!Saved()) return;
        Key(EKeys::SpaceBar); Next(); break;
    case 8:
        if (!Check(Condition == TEXT("Intact") && State->Economy.Day == 3 && State->IsTavernOperational(), TEXT("second actual day completes structure and unlocks companion service")) || !Presentation(true, true)) return;
        Capture(TEXT("city_completed")); Next(); break;
    case 9:
        if (!CaptureWithoutHUD(TEXT("city_completed_clean"), Now)) return;
        BeginLoad(); Next(); break;
    case 10:
        if (!Loaded() || !Presentation(false, true) || !Check(Condition == TEXT("Building") && !State->IsTavernOperational(), TEXT("F9 rolls physical state and service back to construction"))) return;
        Capture(TEXT("city_loaded_mid")); Next(); break;
    case 11:
        Key(EKeys::SpaceBar); Next(); break;
    case 12:
        if (!Presentation(true, true) || !Check(State->IsTavernOperational(), TEXT("restored construction completes normally")) || !Remember(TEXT("completed"))) return;
        BeginSave(); Next(); break;
    case 13:
        if (!Saved()) return;
        Key(EKeys::Escape); Next(20); break;
    case 14:
        if (!WaitCampaign() || !Matches() || !Presentation(true, false)) return;
        Key(EKeys::Home);
        for (int32 I = 0; I < 5; ++I) Key(EKeys::MouseScrollUp);
        Next(8); break;
    case 15:
        Capture(TEXT("miniature_completed")); BeginLoad(); Next(); break;
    case 16:
        if (!Loaded() || !Presentation(true, false)) return;
        // Load presentation correctly closes transient UI. Reopen it after the
        // callback, on a separate input frame, before requesting the next visit.
        if (!bVisitPanelOpenedAfterLoad)
        { Key(EKeys::T); bVisitPanelOpenedAfterLoad = true; NextTime = Now + 4; return; }
        Key(EKeys::V); Next(20); break;
    case 17:
        if (!WaitVisit() || !Matches() || !Presentation(true, true)) return;
        Capture(TEXT("city_revisited_completed")); Key(EKeys::H); Next(); break;
    case 18:
        if (!Check(State->bSecondHeroHired && State->Economy.Resources.FindRef(TEXT("gold")) == InitialGold - 200 + 2 * 450 - 1200,
            TEXT("actual H input uses unlocked service and charges existing hire price")) || !Remember(TEXT("completed_hired"))) return;
        BeginSave(); Next(); break;
    case 19:
        if (!Saved()) return;
        Key(EKeys::Escape); Next(20); break;
    case 20:
        if (!WaitCampaign() || !Matches() || !Presentation(true, false)) return;
        PreBattleSettlement = ExpectedSettlement;
        // Select through the normal region handler, then commit through actual B input.
        for (TActorIterator<ASoulFounderPlaytestCampaignActor> It(GetWorld()); It; ++It)
        {
            It->SelectCompany();
            It->HandleRegionClicked(TEXT("dwarf_forge_approach"));
        }
        Next(); break;
    case 21:
        Key(EKeys::B); Next(2); break;
    case 22:
    {
        if (Campaign) { if (Now - StepStarted > 90) Check(false, TEXT("B did not enter authored battle")); NextTime = Now + 1; return; }
        if (!Check(State->HasPendingBattle() && State->PendingBattle.MapPackage.ToString() == Scenario->OwnedEnvironmentMap.ToSoftObjectPath().GetLongPackageName()
            && GetWorld()->GetAuthGameMode()->GetClass()->GetName() == TEXT("SoulRealtimeArenaGameMode"), TEXT("existing bridge entered authored city with existing realtime combat mode"))) return;
        Encounter = State->PendingBattle.EncounterId;
        int32 Characters = 0;
        for (TActorIterator<ACharacter> It(GetWorld()); It; ++It) if (!It->IsHidden()) ++Characters;
        if (!Check(Characters >= 30, TEXT("both real physical forces spawned")) || !Presentation(true, true)) return;
        Capture(TEXT("authored_battle"));
        Next(30); break;
    }
    case 23:
    {
        if (!Campaign)
        {
            if (!bBattleWarmCapture && AwaitVisualAssets(Now))
            { Capture(TEXT("authored_battle_warm")); bBattleWarmCapture = true; }
            if (Now - StepStarted > 600) Check(false, TEXT("real battle did not resolve within ten minutes; no fabricated result"));
            NextTime = Now + 5; return;
        }
        // Travel completion precedes material/texture readiness in editor-game.
        // Preserve the early battle capture, but don't accept a checkerboard
        // campaign return frame as evidence of the completed presentation.
        if (!bReturnedBattleViewWarmed)
        {
            Key(EKeys::Home);
            for (int32 I = 0; I < 5; ++I) Key(EKeys::MouseScrollUp);
            bReturnedBattleViewWarmed = true; NextTime = Now + 30; return;
        }
        FString C, S;
        if (!Check(!State->HasPendingBattle() && State->LastBattleResult.EncounterId == Encounter && State->ResolvedEncounters.Contains(Encounter), TEXT("real battle result accepted exactly through campaign bridge"))
            || !Snapshots(C, S) || !Check(S == PreBattleSettlement, TEXT("authored battle preserves settlement domain"))) return;
        Capture(TEXT("campaign_after_battle"));
        if (!Remember(TEXT("after_battle"))) return;
        BeginSave(); Next(); break;
    }
    case 24:
        if (!Saved()) return;
        BeginLoad(); Next(); break;
    case 25:
    {
        if (!Loaded()) return;
        for (const FString& Path : Captures) if (!Check(IFileManager::Get().FileSize(*Path) > 1024, TEXT("completed screenshot file: ") + Path)) return;
        const FString Receipt = FString::Printf(TEXT("{\"status\":\"FUNCTIONAL_PASS_PENDING_VISUAL_REVIEW\",\"authored_environment\":true,\"miniature\":true,\"two_domain_restoration\":true,\"real_battle\":true,\"battle_won\":%s,\"player_survivors\":%d,\"enemy_survivors\":%d,\"screenshot_count\":%d,\"seconds\":%.2f}\n"),
            State->LastBattleResult.bPlayerWon ? TEXT("true") : TEXT("false"), State->LastBattleResult.PlayerSurvivors,
            State->LastBattleResult.EnemySurvivors, Captures.Num(), Now - Started);
        if (!Check(FFileHelper::SaveStringToFile(Receipt, *(FPaths::ProjectSavedDir() / (Prefix + TEXT("_authored.json")))), TEXT("functional receipt saved"))) return;
        UE_LOG(LogTemp, Display, TEXT("SOUL_AUTHORED_SETTLEMENT_PASS physical=1 miniature=1 visit=1 save=1 battle=1 visual_review_required=1"));
        bDone = true; FPlatformMisc::RequestExitWithStatus(false, 0); break;
    }
    case 100:
        if (!Loaded() || !Presentation(true, false)
            || !Check(State->bSecondHeroHired && State->IsTavernOperational(), TEXT("fresh process restored completed structure and purchased companion"))) return;
        Key(EKeys::Home);
        for (int32 I = 0; I < 5; ++I) Key(EKeys::MouseScrollUp);
        Key(EKeys::T);
        Next(8); break;
    case 101:
        Capture(TEXT("fresh_miniature")); Key(EKeys::V); Next(20); break;
    case 102:
        if (!WaitVisit() || !Matches() || !Presentation(true, true)) return;
        Capture(TEXT("fresh_city")); Next(); break;
    case 103:
        if (!MeasureView(TEXT("city"))) return;
        Key(EKeys::Escape); Next(20); break;
    case 104:
        if (!WaitCampaign() || !Matches() || !Presentation(true, false)) return;
        Key(EKeys::Home);
        for (int32 I = 0; I < 5; ++I) Key(EKeys::MouseScrollUp);
        Next(8); break;
    case 105:
        if (!MeasureView(TEXT("campaign"))) return;
        Capture(TEXT("fresh_return")); Next(); break;
    case 106:
        for (const FString& Path : Captures) if (!Check(IFileManager::Get().FileSize(*Path) > 1024, TEXT("fresh-load screenshot completed"))) return;
        UE_LOG(LogTemp, Display, TEXT("SOUL_AUTHORED_FRESH_LOAD_PASS two_domains=1 miniature=1 city=1 return=1 uncapped_profiles=2"));
        bDone = true; FPlatformMisc::RequestExitWithStatus(false, 0); break;
    }
}
