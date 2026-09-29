#include "SoulFounderPlaytestGameMode.h"
#include "Engine/World.h"
#include "SoulCampaignCamera.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/GameInstance.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "SoulFounderPlaytestCampaignActor.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulFounderPlaytestHUD.h"
#include "SoulFounderPlaytestPlayerController.h"

ASoulFounderPlaytestGameMode::ASoulFounderPlaytestGameMode()
{
    PlayerControllerClass=ASoulFounderPlaytestPlayerController::StaticClass();
    HUDClass=ASoulFounderPlaytestHUD::StaticClass();DefaultPawnClass=nullptr;
    PrimaryActorTick.bCanEverTick=true;
}
void ASoulFounderPlaytestGameMode::BeginPlay()
{
    Super::BeginPlay();if(!GetWorld())return;
    State=GetGameInstance()->GetSubsystem<USoulFounderPlaytestStateSubsystem>();
    if(State)State->InitializeScenario();
    Campaign=GetWorld()->SpawnActor<ASoulFounderPlaytestCampaignActor>();
    auto* Sun=GetWorld()->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(),FVector(0,0,5000),FRotator(-42,-35,0));
    if(Sun) { Sun->GetLightComponent()->SetIntensity(1.6f); Sun->GetLightComponent()->SetLightColor(FLinearColor(1.f,.89f,.71f)); }
    auto* Sky=GetWorld()->SpawnActor<ASkyLight>();
    if(Sky) { Sky->GetLightComponent()->SetIntensity(.65f); Sky->GetLightComponent()->SetLightColor(FLinearColor(.65f,.77f,1.f)); }
    auto* Fog=GetWorld()->SpawnActor<AExponentialHeightFog>();
    if(Fog) { Fog->GetComponent()->SetFogDensity(.007f); Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(.38f,.48f,.56f)); Fog->GetComponent()->SetStartDistance(2000.f); }
    auto* Camera=GetWorld()->SpawnActor<ASoulCampaignCamera>();
    if(Camera) if(auto* PC=GetWorld()->GetFirstPlayerController()) PC->SetViewTarget(Camera);
    bVisualQualification=FParse::Param(FCommandLine::Get(),TEXT("SoulCampaignVisualProof"));
    FParse::Value(FCommandLine::Get(),TEXT("SoulCampaignCapturePrefix="),CapturePrefix);
    CapturePrefix=FPaths::MakeValidFileName(CapturePrefix.IsEmpty()?TEXT("World"):CapturePrefix);
    bQualification=FParse::Param(FCommandLine::Get(),TEXT("SoulCampaignQualification"));
}
void ASoulFounderPlaytestGameMode::Tick(float Seconds)
{
    Super::Tick(Seconds);
    if(bVisualQualification && !bDone) TickVisualQualification(Seconds);
    if(!bQualification||bDone||!State||!Campaign)return;
    if (bRoundTripVerified)
    {
        const float PreviousHold=ReturnHoldSeconds;
        ReturnHoldSeconds += Seconds;
        if(PreviousHold<4.f && ReturnHoldSeconds>=4.f)
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Vertical_Campaign_Return.png"),true,false);
        if (ReturnHoldSeconds >= 70.0f)
        {
            UE_LOG(LogTemp,Display,TEXT("SOUL_CAMPAIGN_ROUNDTRIP_PASS id=%s target=%s victory=%d survivors=%d/%d persistence=RBSave returnHoldSeconds=%.2f"),
                *State->LastBattleResult.EncounterId.ToString(),*State->LastBattleResult.TargetRegion.ToString(),
                State->LastBattleResult.bPlayerWon,State->LastBattleResult.PlayerSurvivors,State->LastBattleResult.EnemySurvivors,
                ReturnHoldSeconds);
            bDone=true;
            FPlatformMisc::RequestExitWithStatus(false,0);
        }
        return;
    }
    Elapsed+=Seconds;
    if(Elapsed>90)
    {
        UE_LOG(LogTemp,Error,TEXT("SOUL_CAMPAIGN_ROUNDTRIP_FAIL campaign phase timed out"));
        bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);return;
    }
    if(State->LastBattleResult.EncounterId.IsNone())
    {
        if(bStarted||Elapsed<2)return;
        bStarted=true;
        // Explicit qualification inputs update real strategic pools before the normal action path.
        int32 Count;
        if(FParse::Value(FCommandLine::Get(),TEXT("SoulPlayerPool="),Count))State->PlayerArmy[State->PlayerUnitId]=FMath::Max(1,Count);
        if(FParse::Value(FCommandLine::Get(),TEXT("SoulEnemyPool="),Count))
            for(auto& Pair:State->EnemyArmies)Pair.Value=FMath::Max(1,Count);
        if(FParse::Value(FCommandLine::Get(),TEXT("SoulActivePerSide="),Count))State->ActiveCapPerSide=FMath::Clamp(Count,1,35);
        UE_LOG(LogTemp,Display,TEXT("SOUL_CAMPAIGN_QUALIFICATION_SETUP pools=%d/%d cap=%d"),
            State->PlayerArmy.FindRef(State->PlayerUnitId),State->EnemyArmies.FindRef(TEXT("orc_watch")),State->ActiveCapPerSide);
        Campaign->HandleRegionClicked(TEXT("crossroads"));
        Campaign->HandleRegionClicked(TEXT("river_ford"));
        Campaign->HandleRegionClicked(TEXT("orc_watch"));
        Campaign->StartBattle();
        return;
    }
    if(State->bPersistenceBusy)return;
    if(!bLoading)
    {
        const auto& Result=State->LastBattleResult;
        const FName Target=TEXT("orc_watch");
        const FName ExpectedRegion=Result.bPlayerWon?Target:FName(TEXT("river_ford"));
        const auto* Territory=State->World.Regions.Find(Target);
        const bool Correct=Result.TargetRegion==Target && !State->HasPendingBattle()
            && State->ResolvedEncounters.Contains(Result.EncounterId)
            && State->PlayerRegion==ExpectedRegion && Territory
            && Territory->OwnerFactionId==(Result.bPlayerWon?State->PlayerFaction:State->EnemyFaction)
            && State->PlayerArmy.FindRef(State->PlayerUnitId)==Result.PlayerSurvivors
            && State->EnemyArmies.FindRef(Target)==Result.EnemySurvivors
            && State->Hero.Mana==Result.PlayerManaRemaining;
        if(!Correct)
        {
            UE_LOG(LogTemp,Error,TEXT("SOUL_CAMPAIGN_ROUNDTRIP_FAIL strategic consequence mismatch"));
            bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);return;
        }
        UE_LOG(LogTemp,Display,TEXT("SOUL_CAMPAIGN_STRATEGIC_RETURN_PASS region=%s owner=%s allied=%d hostile=%d mana=%d encounter=%s"),
            *State->PlayerRegion.ToString(),*Territory->OwnerFactionId.ToString(),Result.PlayerSurvivors,Result.EnemySurvivors,State->Hero.Mana,*Result.EncounterId.ToString());
        if(!State->bLastSaveSucceeded)
        {
            UE_LOG(LogTemp,Error,TEXT("SOUL_CAMPAIGN_ROUNDTRIP_FAIL save did not succeed"));
            bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);return;
        }
        FRBSaveDomainState Snapshot;FString Error;
        if(!State->CaptureRBSaveDomain_Implementation(Snapshot,Error))return;
        ExpectedSnapshot=Snapshot.Fields[0].StringValue;
        // Change live values so the subsequent assertion proves restoration, not retained memory.
        State->PlayerRegion=TEXT("human_capital");
        State->PlayerArmy.FindOrAdd(State->PlayerUnitId)+=7;
        State->Hero.Experience+=17;
        bLoading=true;State->LoadCampaign();return;
    }
    FRBSaveDomainState Restored;FString Error;
    const bool Passed=State->bLastLoadSucceeded&&State->CaptureRBSaveDomain_Implementation(Restored,Error)
        &&Restored.Fields[0].StringValue==ExpectedSnapshot;
    if(Passed)
    {
        UE_LOG(LogTemp,Display,TEXT("SOUL_CAMPAIGN_ROUNDTRIP_VERIFIED id=%s target=%s victory=%d survivors=%d/%d persistence=RBSave holdSeconds=70"),
            *State->LastBattleResult.EncounterId.ToString(),*State->LastBattleResult.TargetRegion.ToString(),
            State->LastBattleResult.bPlayerWon,State->LastBattleResult.PlayerSurvivors,State->LastBattleResult.EnemySurvivors);
        bRoundTripVerified=true;
        ReturnHoldSeconds=0.0f;
        FScreenshotRequest::RequestScreenshot(
            FPaths::ProjectSavedDir() / TEXT("Screenshots/Vertical_Campaign_Return.png"),false,false);
        return;
    }
    UE_LOG(LogTemp,Error,TEXT("SOUL_CAMPAIGN_ROUNDTRIP_FAIL persistence mismatch %s"),*Error);
    bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);
}
