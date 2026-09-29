#include "SoulFounderPlaytestGameMode.h"
#include "Engine/World.h"
#include "SoulCampaignCamera.h"
#include "SoulCampaignWorldActor.h"
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
#include "InputKeyEventArgs.h"
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
    if(Sun) { Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->SetForwardShadingPriority(1); Sun->GetLightComponent()->SetIntensity(1.6f); Sun->GetLightComponent()->SetLightColor(FLinearColor(1.f,.89f,.71f)); }
    // A restrained, shadowless sky fill keeps miniature silhouettes legible
    // in the asset-free campaign map, whose captured sky otherwise contains no sky dome.
    auto* Fill=GetWorld()->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(),FVector(0,0,4500),FRotator(-55,145,0));
    if(Fill) { Fill->GetLightComponent()->SetIntensity(.48f); Fill->GetLightComponent()->SetLightColor(FLinearColor(.64f,.76f,1.f)); Fill->GetLightComponent()->SetCastShadows(false); }
    auto* Sky=GetWorld()->SpawnActor<ASkyLight>();
    if(Sky) { Sky->GetLightComponent()->SetIntensity(.65f); Sky->GetLightComponent()->SetLightColor(FLinearColor(.65f,.77f,1.f)); }
    auto* Fog=GetWorld()->SpawnActor<AExponentialHeightFog>();
    if(Fog) { Fog->GetComponent()->SetFogDensity(.007f); Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(.38f,.48f,.56f)); Fog->GetComponent()->SetStartDistance(2000.f); }
    auto* Camera=GetWorld()->SpawnActor<ASoulCampaignCamera>();
    if(Camera)
    {
        if(State && State->PlayerRegion!=TEXT("human_capital")) Camera->Focus(ASoulCampaignWorldActor::Locations().FindRef(State->PlayerRegion));
        if(auto* PC=GetWorld()->GetFirstPlayerController()) PC->SetViewTarget(Camera);
    }
    bVisualQualification=FParse::Param(FCommandLine::Get(),TEXT("SoulCampaignVisualProof"));
    FParse::Value(FCommandLine::Get(),TEXT("SoulCampaignCapturePrefix="),CapturePrefix);
    CapturePrefix=FPaths::MakeValidFileName(CapturePrefix.IsEmpty()?TEXT("World"):CapturePrefix);
    bQualification=FParse::Param(FCommandLine::Get(),TEXT("SoulCampaignQualification"));
    bRecoveryQualification=bQualification && FParse::Param(FCommandLine::Get(),TEXT("SoulCampaignRetryQualification"));
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
        // Inspect the newly revealed destination after a real conquest, without revealing unknown state.
        if(!bRecoveryQualification && FSoulWorldRules::IsExplored(State->World,State->PlayerFaction,TEXT("orc_camp")))
        {
            auto* PC=GetWorld()->GetFirstPlayerController();
            auto* Camera=PC?Cast<ASoulCampaignCamera>(PC->GetViewTarget()):nullptr;
            if(Camera && PreviousHold<20.f && ReturnHoldSeconds>=20.f) Camera->Focus(ASoulCampaignWorldActor::Locations().FindRef(TEXT("orc_camp")));
            if(PreviousHold<25.f && ReturnHoldSeconds>=25.f)
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Campaign_Stronghold.png"),true,false);
            if(Camera && PreviousHold<35.f && ReturnHoldSeconds>=35.f) Camera->Focus(ASoulCampaignWorldActor::Locations().FindRef(State->PlayerRegion));
        }
        if(bRecoveryQualification && State->ResolvedEncounters.Num()==1 && ReturnHoldSeconds>=10.f && !bRecoveryAttempted)
        {
            bRecoveryAttempted=true;
            const FName FirstEncounter=State->LastBattleResult.EncounterId;
            bool Correct=!State->LastBattleResult.bPlayerWon && State->PlayerArmy.FindRef(State->PlayerUnitId)==0;
            Campaign->EndDay();Campaign->HandleRegionClicked(TEXT("crossroads"));Campaign->HandleRegionClicked(TEXT("human_capital"));
            const auto* Pool=State->Economy.RecruitmentPools.Find(State->PlayerUnitId);
            const int32 Available=Pool?Pool->Available:0;
            const int32 Gold=State->Economy.Resources.FindRef(TEXT("gold"));
            const int32 Cost=Pool?Pool->CostPerUnit.FindRef(TEXT("gold")):0;
            Campaign->ToggleTownPanel();
            for(int32 I=0;I<3;++I)Campaign->HandleNumberKey(1);
            Correct=Correct && State->PlayerRegion==TEXT("human_capital") && State->PlayerArmy.FindRef(State->PlayerUnitId)==3
                && Pool && Pool->Available==Available-3 && State->Economy.Resources.FindRef(TEXT("gold"))==Gold-3*Cost;
            Campaign->CancelPanel();Campaign->EndDay();
            Campaign->HandleRegionClicked(TEXT("crossroads"));Campaign->HandleRegionClicked(TEXT("river_ford"));Campaign->HandleRegionClicked(TEXT("orc_watch"));
            Campaign->StartBattle();
            Correct=Correct && State->HasPendingBattle() && State->PendingBattle.EncounterId!=FirstEncounter;
            if(!Correct)
            {
                UE_LOG(LogTemp,Error,TEXT("SOUL_CAMPAIGN_RECOVERY_FAIL defeat/recruit/retry invariant"));
                bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);return;
            }
            UE_LOG(LogTemp,Display,TEXT("SOUL_CAMPAIGN_RETRY_STARTED previous=%s next=%s recruits=3 finite_pool_spent=3"),*FirstEncounter.ToString(),*State->PendingBattle.EncounterId.ToString());
            return;
        }
        if (ReturnHoldSeconds >= 70.0f)
        {
            UE_LOG(LogTemp,Display,TEXT("SOUL_CAMPAIGN_ROUNDTRIP_PASS id=%s target=%s victory=%d survivors=%d/%d persistence=RBSave returnHoldSeconds=%.2f"),
                *State->LastBattleResult.EncounterId.ToString(),*State->LastBattleResult.TargetRegion.ToString(),
                State->LastBattleResult.bPlayerWon,State->LastBattleResult.PlayerSurvivors,State->LastBattleResult.EnemySurvivors,
                ReturnHoldSeconds);
            if(bRecoveryQualification)
            {
                if(State->ResolvedEncounters.Num()!=2)
                { UE_LOG(LogTemp,Error,TEXT("SOUL_CAMPAIGN_RECOVERY_FAIL second encounter did not resolve"));bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);return; }
                UE_LOG(LogTemp,Display,TEXT("SOUL_CAMPAIGN_RECOVERY_PASS resolved=2 second=%s survivors=%d/%d"),*State->LastBattleResult.EncounterId.ToString(),State->LastBattleResult.PlayerSurvivors,State->LastBattleResult.EnemySurvivors);
            }
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
        if(auto* PC=GetWorld()->GetFirstPlayerController())
        {
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::B,IE_Pressed,1));
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::B,IE_Released,0));
        }
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
