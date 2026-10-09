#include "SoulFounderPlaytestGameMode.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "SoulCampaignCamera.h"
#include "SoulCampaignWorldActor.h"
#include "SoulCampaignTerrain.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/TextureCube.h"
#include "Engine/GameInstance.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
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
    if(!SoulCampaignTerrain::Composition())
    {
    auto* Sun=GetWorld()->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(),FVector(0,0,5000),FRotator(-42,-35,0));
    if(Sun) { Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->SetForwardShadingPriority(1); Sun->GetLightComponent()->SetIntensity(1.6f); Sun->GetLightComponent()->SetLightColor(FLinearColor(1.f,.89f,.71f)); }
    if(Sun&&SoulCampaignTerrain::Enabled())
    {
        auto* Light=Cast<UDirectionalLightComponent>(Sun->GetLightComponent());Light->SetMobility(EComponentMobility::Movable);
        Light->SetIntensity(3.2f);Light->DynamicShadowDistanceMovableLight=220000.f;Light->DynamicShadowCascades=3;
        Light->SetAtmosphereSunLight(true);
        // Static campaign lighting presentation; RB Weather remains the weather authority.
        GetWorld()->SpawnActor<ASkyAtmosphere>();
    }
    // A restrained, shadowless sky fill keeps miniature silhouettes legible
    // in the asset-free campaign map, whose captured sky otherwise contains no sky dome.
    auto* Fill=GetWorld()->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(),FVector(0,0,4500),FRotator(-55,145,0));
    if(Fill) { Fill->GetLightComponent()->SetIntensity(.48f); Fill->GetLightComponent()->SetLightColor(FLinearColor(.64f,.76f,1.f)); Fill->GetLightComponent()->SetCastShadows(false); }
    if(Fill&&SoulCampaignTerrain::Enabled()){Fill->GetLightComponent()->SetMobility(EComponentMobility::Movable);Fill->GetLightComponent()->SetIntensity(1.4f);}
    auto* Sky=GetWorld()->SpawnActor<ASkyLight>();
    if(Sky) { Sky->GetLightComponent()->SetIntensity(.65f); Sky->GetLightComponent()->SetLightColor(FLinearColor(.65f,.77f,1.f)); }
    if(Sky&&SoulCampaignTerrain::Enabled())
    {
        auto* Light=Sky->GetLightComponent();Light->SetMobility(EComponentMobility::Movable);
        Light->SourceType=SLS_SpecifiedCubemap;
        Light->SetCubemap(LoadObject<UTextureCube>(nullptr,TEXT("/Engine/MapTemplates/Sky/DaylightAmbientCubemap")));
        Light->SetIntensity(.55f);Light->SetLowerHemisphereColor(FLinearColor(.10f,.12f,.09f));
        Light->RecaptureSky();
    }
    auto* Fog=GetWorld()->SpawnActor<AExponentialHeightFog>();
    if(Fog) { Fog->GetComponent()->SetFogDensity(SoulCampaignTerrain::Enabled()?.0012f:.007f); Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(.38f,.48f,.56f)); Fog->GetComponent()->SetStartDistance(2000.f*SoulCampaignTerrain::Scale()); }
    }
    auto* Camera=GetWorld()->SpawnActor<ASoulCampaignCamera>();
    if(Camera)
    {
        if(State && (SoulCampaignTerrain::Composition()||SoulCampaignTerrain::Mesa()||State->PlayerRegion!=TEXT("human_capital"))) Camera->Focus(ASoulCampaignWorldActor::Locations().FindRef(State->PlayerRegion));
        if(FParse::Param(FCommandLine::Get(),TEXT("SoulTerrainBenchmark")))
        {
            FParse::Value(FCommandLine::Get(),TEXT("SoulTerrainBenchmarkSeconds="),BenchmarkSampleSeconds);
            BenchmarkSampleSeconds=FMath::Clamp(BenchmarkSampleSeconds,20,60);
            FString View;float Zoom=0;
            if(FParse::Value(FCommandLine::Get(),TEXT("SoulTerrainBenchmarkFocus="),View))
            {
                if(const FVector* P=ASoulCampaignWorldActor::Locations().Find(FName(*View)))Camera->Focus(*P);
            }
            if(FParse::Value(FCommandLine::Get(),TEXT("SoulTerrainBenchmarkZoom="),Zoom))Camera->Zoom(Zoom);
        }
        if(auto* PC=GetWorld()->GetFirstPlayerController()) PC->SetViewTarget(Camera);
    }
    bVisualQualification=FParse::Param(FCommandLine::Get(),TEXT("SoulCampaignVisualProof"));
    bSettlementDevelopmentQualification=FParse::Param(FCommandLine::Get(),TEXT("SoulSettlementDevelopmentQualification"));
    FParse::Value(FCommandLine::Get(),TEXT("SoulCampaignCapturePrefix="),CapturePrefix);
    CapturePrefix=FPaths::MakeValidFileName(CapturePrefix.IsEmpty()?TEXT("World"):CapturePrefix);
    bQualification=FParse::Param(FCommandLine::Get(),TEXT("SoulCampaignQualification"));
    if(FParse::Param(FCommandLine::Get(),TEXT("SoulCampaignMouseRoundtrip")))
    {
        bQualification=State&&!State->LastBattleResult.EncounterId.IsNone();
        bVisualQualification=!bQualification;
    }
    bRecoveryQualification=bQualification && FParse::Param(FCommandLine::Get(),TEXT("SoulCampaignRetryQualification"));
}
void ASoulFounderPlaytestGameMode::Tick(float Seconds)
{
    Super::Tick(Seconds);
    if(State && State->IsFourFactionAlpha() && !State->bPersistenceBusy && FPlatformTime::Seconds()>=AlphaNextActionTime)
    {
        State->RunNextAlphaAction();AlphaNextActionTime=FPlatformTime::Seconds()+0.75;
        if(State->HasPendingBattle() && !bAlphaBattleTravel)
        {
            bAlphaBattleTravel=true;
            UGameplayStatics::OpenLevel(this,State->PendingBattle.MapPackage,true,TEXT("game=/Script/SoulRealtimeBattle.SoulRealtimeArenaGameMode"));return;
        }
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("SoulAlphaQualification"))){if(!bDone)TickFourFactionAlphaQualification(Seconds);return;}
    FString ControlledAttacker;
    if(FParse::Value(FCommandLine::Get(),TEXT("SoulControlledBattle="),ControlledAttacker)
        && !FParse::Param(FCommandLine::Get(),TEXT("SoulCampaignLoadProof")))
    {if(!bDone)TickControlledBattleQualification(Seconds);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("SoulSixFactionQualification")))
    {if(!bDone)TickSixFactionQualification(Seconds);return;}
    if(FParse::Param(FCommandLine::Get(),TEXT("SoulCompositionTraversal")))
    {if(!bDone)TickCompositionTraversal(Seconds);return;}
    if(bSettlementDevelopmentQualification)
    {if(!bDone)TickSettlementDevelopmentQualification(Seconds);return;}
    // Separate-process F9 proof: a fresh game must restore the preceding F5 snapshot.
    if(FParse::Param(FCommandLine::Get(),TEXT("SoulCampaignLoadProof"))&&!bDone&&State&&Campaign)
    {
        Elapsed+=Seconds;
        if(bRoundTripVerified)
        {
            if(Elapsed>=75){bDone=true;FPlatformMisc::RequestExitWithStatus(false,0);}
            return;
        }
        auto* PC=GetWorld()->GetFirstPlayerController();
        if(!bStarted&&Elapsed>=4&&PC)
        {
            bStarted=true;
            if(!FFileHelper::LoadFileToString(ExpectedSnapshot,*(FPaths::ProjectSavedDir()/TEXT("CampaignInputExpectedSnapshot.json"))))
            {UE_LOG(LogTemp,Error,TEXT("SOUL_CAMPAIGN_COLD_LOAD_FAIL missing expected snapshot"));bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);return;}
            UE_LOG(LogTemp,Display,TEXT("SOUL_CAMPAIGN_COLD_LOAD_BEGIN fresh_region=%s"),*State->PlayerRegion.ToString());
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::F9,IE_Pressed,1));
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::F9,IE_Released,0));
        }
        if(Elapsed>=10&&!State->bPersistenceBusy)
        {
            FRBSaveDomainState Restored;FString Error;
            auto* Camera=PC?Cast<ASoulCampaignCamera>(PC->GetViewTarget()):nullptr;
            const bool Pass=State->bLastLoadSucceeded&&State->CaptureRBSaveDomain_Implementation(Restored,Error)
                &&Restored.Fields[0].StringValue==ExpectedSnapshot&&Campaign->GetSelectedRegion()==State->PlayerRegion
                &&!Campaign->IsTownPanelOpen()&&!Campaign->IsBattleAvailable()&&Camera
                &&FVector::DistXY(Camera->GetFocus(),ASoulCampaignWorldActor::Locations().FindRef(State->PlayerRegion))<3;
            if(Pass){UE_LOG(LogTemp,Display,TEXT("SOUL_CAMPAIGN_COLD_LOAD_PASS region=%s exact_snapshot=1 controller_F9=1"),*State->PlayerRegion.ToString());}
            else {UE_LOG(LogTemp,Error,TEXT("SOUL_CAMPAIGN_COLD_LOAD_FAIL snapshot or presentation mismatch"));}
            if(Pass)
            {
                bRoundTripVerified=true;
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Campaign_Cold_Load.png"),true,false);
            }
            else {bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);}
        }
        if(Elapsed>30){UE_LOG(LogTemp,Error,TEXT("SOUL_CAMPAIGN_COLD_LOAD_FAIL timeout"));bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);}
        return;
    }
    // Opt-in benchmark of the actual rendered campaign; excludes warmup and captures.
    if(FParse::Param(FCommandLine::Get(),TEXT("SoulTerrainBenchmark")))
    {
        static double Started=FPlatformTime::Seconds(),Previous=Started;
        static TArray<double> Samples;
        const double Now=FPlatformTime::Seconds(),Age=Now-Started,Frame=(Now-Previous)*1000;Previous=Now;
        const double SampleEnd=20.+BenchmarkSampleSeconds;
        if(Age>=20&&Age<SampleEnd)Samples.Add(Frame);
        if(Age>=SampleEnd&&!bDone)
        {
            bDone=true;double Sum=0;int32 Below30=0,Below40=0,Below60=0;
            FString CSV=TEXT("frame_ms\n");
            for(double V:Samples){Sum+=V;Below30+=V>1000./30;Below40+=V>25;Below60+=V>1000./60;CSV+=FString::Printf(TEXT("%.5f\n"),V);}
            Samples.Sort();const int32 N=Samples.Num();
            const FString Receipt=FString::Printf(TEXT("{\"frames\":%d,\"warmup_seconds\":20,\"sample_seconds\":%.5f,\"requested_sample_seconds\":%d,\"mean_ms\":%.4f,\"p95_ms\":%.4f,\"p99_ms\":%.4f,\"below_30\":%d,\"below_40\":%d,\"below_60\":%d,\"v2\":%s}"),N,Sum/1000.,BenchmarkSampleSeconds,Sum/FMath::Max(N,1),N?Samples[FMath::Min(N-1,FMath::FloorToInt(N*.95))]:0,N?Samples[FMath::Min(N-1,FMath::FloorToInt(N*.99))]:0,Below30,Below40,Below60,SoulCampaignTerrain::Enabled()?TEXT("true"):TEXT("false"));
            FFileHelper::SaveStringToFile(Receipt,*(FPaths::ProjectSavedDir()/TEXT("TerrainBenchmark.json")));
            FFileHelper::SaveStringToFile(CSV,*(FPaths::ProjectSavedDir()/TEXT("TerrainBenchmark.csv")));
            UE_LOG(LogTemp,Display,TEXT("SOUL_TERRAIN_BENCHMARK_COMPLETE %s"),*Receipt);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/TerrainBenchmark.png"),true,false);
        }
        if(Age>SampleEnd+3)FPlatformMisc::RequestExitWithStatus(false,0);
        return;
    }
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
            if(PC && PreviousHold<24.f && ReturnHoldSeconds>=24.f && FParse::Param(FCommandLine::Get(),TEXT("SoulCampaignMouseRoundtrip")))
            {
                FVector2D Screen;
                if(PC->ProjectWorldLocationToScreen(ASoulCampaignWorldActor::Locations().FindRef(TEXT("orc_camp"))+FVector(0,0,100*SoulCampaignTerrain::RegionScale()),Screen))
                {
                    PC->SetMouseLocation(FMath::RoundToInt(Screen.X),FMath::RoundToInt(Screen.Y));
                    PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton,IE_Pressed,1));
                    PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton,IE_Released,0));
                }
            }
            if(PreviousHold<25.f && ReturnHoldSeconds>=25.f)
            {
                if(FParse::Param(FCommandLine::Get(),TEXT("SoulCampaignMouseRoundtrip")))
                {
                    if(!Campaign->IsBattleAvailable()||Campaign->GetSelectedRegion()!=TEXT("orc_camp"))
                    {UE_LOG(LogTemp,Error,TEXT("SOUL_CAMPAIGN_RETURN_CLICK_FAIL fortress selection"));bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);return;}
                    if(SoulCampaignTerrain::EvilCorridor()&&Camera&&FVector::Dist2D(Camera->GetFocus(),ASoulCampaignWorldActor::Locations().FindRef(TEXT("orc_camp")))>100.f)
                    {UE_LOG(LogTemp,Error,TEXT("SOUL_CORRIDOR_FOCUS_FAIL fortress is not centered"));bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);return;}
                    UE_LOG(LogTemp,Display,TEXT("SOUL_CAMPAIGN_RETURN_CLICK_PASS fortress=orc_camp battle_prompt=1"));
                }
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Campaign_Stronghold.png"),true,false);
            }
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
        if(SoulCampaignTerrain::EvilCorridor()&&State->LastBattleResult.bPlayerWon&&!bRecoveryQualification)
        {
            auto* PC=GetWorld()->GetFirstPlayerController();auto* Camera=PC?Cast<ASoulCampaignCamera>(PC->GetViewTarget()):nullptr;
            auto At=[&](float T){return PreviousHold<T&&ReturnHoldSeconds>=T;};
            auto Click=[&](FName Id)
            {
                FVector2D Screen;
                if(!PC||!PC->ProjectWorldLocationToScreen(ASoulCampaignWorldActor::Locations().FindRef(Id)+FVector(0,0,100*SoulCampaignTerrain::RegionScale()),Screen))return;
                PC->SetMouseLocation(FMath::RoundToInt(Screen.X),FMath::RoundToInt(Screen.Y));
                PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton,IE_Pressed,1));
                PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton,IE_Released,0));
            };
            if(Camera&&At(40))Camera->Focus(ASoulCampaignWorldActor::Locations().FindRef(TEXT("river_ford")));
            if(At(45))Click(TEXT("river_ford"));
            if(Camera&&At(48))Camera->Focus(FVector(-19867,3389,400));
            if(At(51))FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Corridor_Bridge_Travel.png"),true,false);
            if(At(59))
            {
                if(State->PlayerRegion!=TEXT("river_ford")||State->Economy.ActionPoints!=1)
                {UE_LOG(LogTemp,Error,TEXT("SOUL_CORRIDOR_TRAVEL_FAIL outbound"));bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);return;}
                UE_LOG(LogTemp,Display,TEXT("SOUL_CORRIDOR_TRAVEL outbound=river_ford actions=1"));
            }
            if(Camera&&At(60))Camera->Focus(ASoulCampaignWorldActor::Locations().FindRef(TEXT("orc_watch")));
            if(At(64))Click(TEXT("orc_watch"));
            if(At(78))
            {
                if(State->PlayerRegion!=TEXT("orc_watch")||State->Economy.ActionPoints!=0)
                {UE_LOG(LogTemp,Error,TEXT("SOUL_CORRIDOR_TRAVEL_FAIL inbound"));bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);return;}
                UE_LOG(LogTemp,Display,TEXT("SOUL_CORRIDOR_TRAVEL_PASS roundtrip=Ashport_Bridgeward_Ashport actions_spent=2 controller_clicks=2"));
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Corridor_Ashport_Return.png"),true,false);
            }
        }
        if (ReturnHoldSeconds >= (SoulCampaignTerrain::EvilCorridor()?85.0f:70.0f))
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
            State->PlayerArmy.FindRef(State->PlayerUnitId),State->EnemyArmies.FindRef(FParse::Param(FCommandLine::Get(),TEXT("SoulVikingMatchupProof"))?FName(TEXT("viking_snow_pass")):FName(TEXT("orc_watch"))),State->ActiveCapPerSide);
        if(FParse::Param(FCommandLine::Get(),TEXT("SoulVikingMatchupProof")))
            Campaign->HandleRegionClicked(TEXT("viking_snow_pass"));
        else
        {
            Campaign->HandleRegionClicked(TEXT("crossroads"));
            Campaign->HandleRegionClicked(TEXT("river_ford"));
            Campaign->HandleRegionClicked(TEXT("orc_watch"));
        }
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
        const bool ExpectDefeat=FParse::Param(FCommandLine::Get(),TEXT("SoulCampaignDefeatProof"));
        // A loss does not imply enemy casualties or a need for enemy reserves.
        // Wave qualification is explicit and independent of defeat/recruit/retry.
        const bool RequireEnemyWave=FParse::Param(FCommandLine::Get(),TEXT("SoulCampaignRequireEnemyWave"));
        if((ExpectDefeat && (Result.bPlayerWon || Result.PlayerSurvivors!=0))
            || (RequireEnemyWave && Result.EnemyReinforcements<1))
        {
            UE_LOG(LogTemp,Error,TEXT("SOUL_CAMPAIGN_ROUNDTRIP_FAIL expected defeat or explicitly requested enemy wave"));
            bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);return;
        }
        const bool bVikingProof=FParse::Param(FCommandLine::Get(),TEXT("SoulVikingMatchupProof"));
        const FName Target=bVikingProof?TEXT("viking_snow_pass"):TEXT("orc_watch");
        const FName ExpectedRegion=Result.bPlayerWon?Target:FName(bVikingProof?TEXT("mountain_shrine"):TEXT("river_ford"));
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
        bLoading=true;
        if(auto* PC=GetWorld()->GetFirstPlayerController())
        {
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::F9,IE_Pressed,1));
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::F9,IE_Released,0));
        }
        return;
    }
    FRBSaveDomainState Restored;FString Error;
    const bool Passed=State->bLastLoadSucceeded&&State->CaptureRBSaveDomain_Implementation(Restored,Error)
        &&Restored.Fields[0].StringValue==ExpectedSnapshot;
    if(Passed)
    {
        UE_LOG(LogTemp,Display,TEXT("SOUL_CAMPAIGN_ROUNDTRIP_VERIFIED id=%s target=%s victory=%d survivors=%d/%d persistence=RBSave holdSeconds=70"),
            *State->LastBattleResult.EncounterId.ToString(),*State->LastBattleResult.TargetRegion.ToString(),
            State->LastBattleResult.bPlayerWon,State->LastBattleResult.PlayerSurvivors,State->LastBattleResult.EnemySurvivors);
        if(FParse::Param(FCommandLine::Get(),TEXT("SoulOrcMatchupProof"))||FParse::Param(FCommandLine::Get(),TEXT("SoulVikingMatchupProof")))
        {
            if(!FFileHelper::SaveStringToFile(ExpectedSnapshot,*(FPaths::ProjectSavedDir()/TEXT("CampaignInputExpectedSnapshot.json"))))
            {UE_LOG(LogTemp,Error,TEXT("SOUL_ORC_PROOF_FAIL cannot preserve fresh-load checkpoint"));bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);return;}
        }
        bRoundTripVerified=true;
        ReturnHoldSeconds=0.0f;
        FScreenshotRequest::RequestScreenshot(
            FPaths::ProjectSavedDir() / TEXT("Screenshots/Vertical_Campaign_Return.png"),false,false);
        return;
    }
    UE_LOG(LogTemp,Error,TEXT("SOUL_CAMPAIGN_ROUNDTRIP_FAIL persistence mismatch %s"),*Error);
    bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);
}
