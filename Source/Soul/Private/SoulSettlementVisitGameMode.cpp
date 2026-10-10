#include "InputKeyEventArgs.h"
#include "SoulSettlementVisitGameMode.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/LevelStreaming.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "SoulFounderPlaytestHUD.h"
#include "SoulFounderPlaytestPlayerController.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulSettlementPresentationController.h"
#include "SoulSettlementScenarioData.h"
#include "SoulSettlementStateSubsystem.h"
#include "SoulTownViewAnchor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimationAsset.h"
#include "Engine/SkeletalMesh.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "UnrealClient.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#endif

ASoulSettlementVisitGameMode::ASoulSettlementVisitGameMode()
{
    PlayerControllerClass = ASoulFounderPlaytestPlayerController::StaticClass();
    HUDClass = ASoulFounderPlaytestHUD::StaticClass();
    DefaultPawnClass = nullptr;
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0;
}

bool ASoulSettlementVisitGameMode::CanVisit(const USoulFounderPlaytestStateSubsystem* Campaign, FString& OutError)
{
    auto Reject = [&OutError](const TCHAR* Reason) { OutError = Reason; return false; };
    if (!Campaign || !Campaign->IsSettlementDevelopmentReady())
        return Reject(TEXT("The settlement development environment is not ready."));
    if(Campaign->IsHeartlandEnabled()&&Campaign->Hero.Condition==ESoulHeroCondition::Captured)
        return Reject(TEXT("Your hero is captured and unavailable for a walking visit. Settlement management remains available."));
    if (Campaign->IsAlphaTurnActive())
        return Reject(TEXT("Wait for the other factions to finish their turns before visiting."));
    const auto* Scenario = Campaign->GetSettlementScenario();
    const auto* Region = Campaign->World.Regions.Find(Scenario->RegionId);
    const auto* Authority = Campaign->GetGameInstance()->GetSubsystem<USoulSettlementStateSubsystem>();
    const auto* Settlement = Authority ? Authority->FindSettlement(Scenario->SettlementId) : nullptr;
    if (Campaign->HasPendingBattle() || Campaign->bPersistenceBusy || Campaign->PlayerRegion != Scenario->RegionId
        || !Region || Region->OwnerFactionId != Campaign->PlayerFaction || !Settlement
        || Settlement->FactionId != Campaign->PlayerFaction || Settlement->RegionId != Campaign->PlayerRegion)
        return Reject(TEXT("Visit requires your current owned settlement and no active battle or save operation."));
    const FString Map = Scenario->OwnedEnvironmentMap.ToSoftObjectPath().GetLongPackageName();
    if (!Map.StartsWith(TEXT("/Game/Soul/")) || !FPackageName::DoesPackageExist(Map))
        return Reject(TEXT("The owned settlement environment map is missing or not bound."));
    OutError.Reset();
    return true;
}

void ASoulSettlementVisitGameMode::BeginPlay()
{
    Super::BeginPlay();
    State = GetGameInstance()->GetSubsystem<USoulFounderPlaytestStateSubsystem>();
    if (State) State->InitializeScenario();
    if (!CanVisit(State, LastFailure))
    { LastMessage = LastFailure; UE_LOG(LogTemp, Error, TEXT("SOUL_SETTLEMENT_VISIT_FAIL %s"), *LastFailure); return; }
    const FString ExpectedMap = State->GetSettlementScenario()->OwnedEnvironmentMap.ToSoftObjectPath().GetLongPackageName();
    if (UWorld::RemovePIEPrefix(GetWorld()->GetOutermost()->GetName()) != ExpectedMap)
    {
        LastFailure = TEXT("Loaded environment does not match the settlement's owned map binding.");
        LastMessage = LastFailure; UE_LOG(LogTemp, Error, TEXT("SOUL_SETTLEMENT_VISIT_FAIL %s"), *LastFailure); return;
    }
    LastMessage = TEXT("Loading the authored settlement view...");
    RefreshPresentation();
}

void ASoulSettlementVisitGameMode::RefreshPresentation()
{
    if (!State || !State->GetSettlementScenario()) return;
    for (TActorIterator<ASoulSettlementPresentationController> It(GetWorld()); It; ++It)
        if (It->SettlementId == State->GetSettlementScenario()->SettlementId) It->RefreshSettlementPresentation();
    ObservedDevelopmentRevision = State->SettlementDevelopmentRevision;
    ObservedLoadRevision = State->CampaignLoadRevision;
}

void ASoulSettlementVisitGameMode::Tick(float Seconds)
{
    Super::Tick(Seconds);
    if (!State) return;
    if(bReturnRequested)
    {
        ReturnWaitSeconds+=Seconds;
        if(!State->HasPendingBattle()&&!State->bPersistenceBusy&&(IsStreamingReady()||ReturnWaitSeconds>=15))
        {bReturnRequested=false;UGameplayStatics::OpenLevel(this,State->CampaignMap,true,TEXT("game=/Script/Soul.SoulFounderPlaytestGameMode"));return;}
    }
    if(!LastFailure.IsEmpty())return;
    TickWalking(Seconds);
    if(bCameraReady&&!Walker&&CameraWaitSeconds<30&&State->GetDevelopmentRegion()==TEXT("human_capital"))
    {CameraWaitSeconds+=Seconds;if(FMath::FloorToInt(CameraWaitSeconds)!=FMath::FloorToInt(CameraWaitSeconds-Seconds)&&StartWalking())LastMessage=TEXT("WASD walk | RMB + mouse look | Shift run | Tab manage | Esc return.");}
    if (!bCameraReady)
    {
        for (TActorIterator<ASoulTownViewAnchor> It(GetWorld()); It; ++It)
            if (It->SettlementId == State->GetSettlementScenario()->SettlementId
                && It->ActivateForPlayer(GetWorld()->GetFirstPlayerController(), 0))
            {
                bCameraReady = true;
                if(State->GetSettlementScenario()->SettlementId==TEXT("human_capital"))StartWalking();
                LastMessage = Walker?TEXT("WASD walk | RMB + mouse look | Shift run | Tab manage | Esc return. Explore the authored city; collision remains physical."):TEXT("Authored city overview. Walking entry unavailable; management and return remain available.");
                RefreshPresentation();
                UE_LOG(LogTemp, Display, TEXT("SOUL_SETTLEMENT_VISIT_READY settlement=%s map=%s"),
                    *It->SettlementId.ToString(), *GetWorld()->GetOutermost()->GetName());
                break;
            }
        CameraWaitSeconds += Seconds;
        if (!bCameraReady && CameraWaitSeconds >= 20)
        {
            LastFailure = TEXT("The authored settlement camera is missing. Return to the campaign.");
            LastMessage = LastFailure; UE_LOG(LogTemp, Error, TEXT("SOUL_SETTLEMENT_VISIT_FAIL %s"), *LastFailure);
        }
    }
    if (ObservedDevelopmentRevision != State->SettlementDevelopmentRevision || ObservedLoadRevision != State->CampaignLoadRevision)
    {
        const bool RestoredElsewhere=ObservedLoadRevision!=State->CampaignLoadRevision;
        RefreshPresentation();
        if (!CanVisit(State, LastFailure))
        {
            LastMessage = LastFailure + TEXT(" Return to the campaign.");
            // A remote F9 restore must not leave the player embodied in the old city.
            if(RestoredElsewhere)HandleAction(TEXT("Return"));
        }
    }
}

void ASoulSettlementVisitGameMode::HandleAction(FName Action)
{
    if (!State) return;
    if(Action.ToString().StartsWith(TEXT("Develop:"))){State->BeginSettlementConstruction(FName(*Action.ToString().Mid(8)),LastMessage);return;}
    if(Action==TEXT("Manage")){bManagePanel=!bManagePanel;return;}
    if (Action == TEXT("Return"))
    {
        if (State->HasPendingBattle() || State->bPersistenceBusy)
        { LastMessage = TEXT("Finish the current battle or save/load before returning."); return; }
        if (State->CampaignMap.IsNone() || !FPackageName::DoesPackageExist(State->CampaignMap.ToString()))
        { LastMessage = TEXT("The campaign return map is unavailable."); return; }
        if(!IsStreamingReady()){if(!bReturnRequested)ReturnWaitSeconds=0;bReturnRequested=true;LastMessage=TEXT("Finishing city streaming before returning to campaign...");return;}
        UGameplayStatics::OpenLevel(this, State->CampaignMap, true, TEXT("game=/Script/Soul.SoulFounderPlaytestGameMode"));
        return;
    }
    if (!IsVisitReady() || !CanVisit(State, LastMessage)) return;
    if (Action == TEXT("BuildTavern"))
    {
        if (State->BeginSettlementConstruction(State->GetTavernBuildingId(), LastMessage)) LastMessage = State->GetDevelopmentBuildingName() + TEXT(" construction started.");
    }
    else if (Action == TEXT("EndDay"))
    {
        State->AdvanceDay(); LastMessage = TEXT("A new day begins.");
        // AI turns run in the existing campaign mode, never a second scheduler in the city.
        if(State->IsFourFactionAlpha() && State->IsAlphaTurnActive()){HandleAction(TEXT("Return"));return;}
    }
    else if (Action == TEXT("Hire"))
        LastMessage = State->HireTavernHero() ? TEXT("Tavern companion hired.")
            : TEXT("Hiring requires an operational tavern, 1200 gold and an available companion.");
    else if(Action==TEXT("Recruit")) LastMessage=State->Recruit(TEXT("human_knight"))?TEXT("A paid Eastern Knight joined your company."):TEXT("Recruitment requires an owned capital, available pool, gold and movement.");
    else if (Action == TEXT("Save")) State->SaveCampaign();
    else if (Action == TEXT("Load")) State->LoadCampaign();
    RefreshPresentation();
}

bool ASoulSettlementVisitGameMode::IsStreamingReady() const
{
    if(IsAsyncLoading())return false;
    for(const auto* Level:GetWorld()->GetStreamingLevels())
        if(Level&&(Level->IsStreamingStatePending()||(Level->ShouldBeLoaded()&&!Level->IsLevelLoaded())
            ||(Level->ShouldBeVisible()&&!Level->IsLevelVisible())))return false;
    return true;
}

bool ASoulSettlementVisitGameMode::StartWalking()
{
    auto* PC=GetWorld()->GetFirstPlayerController();if(!PC)return false;
    auto* Mesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Meshes/Aurora.Aurora"));
    WalkIdle=LoadObject<UAnimationAsset>(nullptr,TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Idle.Idle"));
    WalkJog=LoadObject<UAnimationAsset>(nullptr,TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Jog_Fwd.Jog_Fwd"));
    if(!Mesh||!WalkIdle||!WalkJog)return false;
    FVector Entry;bool Found=false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(SoulCityEntry),true);
    FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_WorldStatic);
    // Authored tavern forecourt; probe physical ground, never teleport through a wall.
    for(const FVector& P:TArray<FVector>{FVector(-4350,2700,0),FVector(-3500,2700,0),FVector(-5000,1800,0),FVector(-9000,21000,0)})
    {
        FHitResult Hit;
        if(GetWorld()->LineTraceSingleByObjectType(Hit,P+FVector(0,0,1800),P-FVector(0,0,2000),Objects,Query)&&Hit.ImpactNormal.Z>.75f)
        {Entry=Hit.ImpactPoint+FVector(0,0,100);Found=true;break;}
    }
    if(!Found)return false;
    FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
    Walker=GetWorld()->SpawnActor<ACharacter>(ACharacter::StaticClass(),Entry,FRotator(0,90,0),Params);
    if(!Walker)return false;
    Walker->GetMesh()->SetSkeletalMeshAsset(Mesh);Walker->GetMesh()->SetRelativeLocation(FVector(0,0,-88));
    Walker->GetMesh()->SetRelativeRotation(FRotator(0,-90,0));Walker->GetMesh()->PlayAnimation(WalkIdle,true);
    Walker->bUseControllerRotationYaw=false;Walker->GetCharacterMovement()->bOrientRotationToMovement=true;
    Walker->GetCharacterMovement()->MaxWalkSpeed=300;
    auto* Arm=NewObject<USpringArmComponent>(Walker);Arm->SetupAttachment(Walker->GetRootComponent());
    Arm->TargetArmLength=340;Arm->SocketOffset=FVector(0,45,80);Arm->bUsePawnControlRotation=true;Arm->RegisterComponent();
    auto* Camera=NewObject<UCameraComponent>(Walker);Camera->SetupAttachment(Arm,USpringArmComponent::SocketName);Camera->SetFieldOfView(70);Camera->RegisterComponent();
    PC->Possess(Walker);PC->SetControlRotation(FRotator(-12,90,0));PC->SetViewTarget(Walker);
    UE_LOG(LogTemp,Display,TEXT("SOUL_CITY_WALK_READY map=%s entry=%s mesh=%s"),*GetWorld()->GetOutermost()->GetName(),*Walker->GetActorLocation().ToString(),*Mesh->GetPathName());return true;
}
void ASoulSettlementVisitGameMode::TickWalking(float Seconds)
{
    auto* PC=GetWorld()->GetFirstPlayerController();if(!Walker||!PC)return;
    if(PC->WasInputKeyJustPressed(EKeys::Tab))bManagePanel=!bManagePanel;
    if(bManagePanel||State->bPersistenceBusy)return;
    if(PC->IsInputKeyDown(EKeys::RightMouseButton))
    {float X=0,Y=0;PC->GetInputMouseDelta(X,Y);FRotator R=PC->GetControlRotation();R.Yaw+=X*.4f;R.Pitch=FMath::Clamp(FRotator::NormalizeAxis(R.Pitch)-Y*.35f,-65.f,35.f);PC->SetControlRotation(R);}
    const FRotationMatrix Basis(FRotator(0,PC->GetControlRotation().Yaw,0));
    Walker->AddMovementInput(Basis.GetUnitAxis(EAxis::X),(PC->IsInputKeyDown(EKeys::W)?1.f:0.f)-(PC->IsInputKeyDown(EKeys::S)?1.f:0.f));
    Walker->AddMovementInput(Basis.GetUnitAxis(EAxis::Y),(PC->IsInputKeyDown(EKeys::D)?1.f:0.f)-(PC->IsInputKeyDown(EKeys::A)?1.f:0.f));
    Walker->GetCharacterMovement()->MaxWalkSpeed=PC->IsInputKeyDown(EKeys::LeftShift)?500:300;
    if(FParse::Param(FCommandLine::Get(),TEXT("SoulHeartlandVisitLoadQualification")))
    {
        const float Before=WalkingProofTime;WalkingProofTime+=Seconds;
        if(Before<5&&WalkingProofTime>=5)
        {
            UE_LOG(LogTemp,Display,TEXT("SOUL_HEARTLAND_VISIT_F9 map=%s streaming_ready=%d"),*GetWorld()->GetOutermost()->GetName(),IsStreamingReady());
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::F9,IE_Pressed,1));PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::F9,IE_Released,0));
        }
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("SoulHeartlandQualification")))
    {
#if WITH_EDITOR
        if(FAssetCompilingManager::Get().GetNumRemainingAssets()>0)return;
#endif
        if(!IsStreamingReady())return;
        if(WalkingProofTime==0)WalkingProofOrigin=Walker->GetActorLocation();
        const float Before=WalkingProofTime;WalkingProofTime+=Seconds;
        if(WalkingProofTime>5&&WalkingProofTime<12)Walker->AddMovementInput(FVector(0,1,0));
        if(Before<15&&WalkingProofTime>=15)FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Heartland_City_Walk.png"),true,false);
        if(Before<20&&WalkingProofTime>=20)
        {
            int32 Actors=0;for(TActorIterator<AActor> It(GetWorld());It;++It)++Actors;
            const float Distance=FVector::Dist2D(WalkingProofOrigin,Walker->GetActorLocation());
            const FString Receipt=FString::Printf(TEXT("distance_cm=%.1f actors=%d map=%s"),Distance,Actors,*GetWorld()->GetOutermost()->GetName());
            if(Distance<150||Actors<3000){UE_LOG(LogTemp,Error,TEXT("SOUL_HEARTLAND_WALK_FAIL %s"),*Receipt);FPlatformMisc::RequestExitWithStatus(false,1);return;}
            FFileHelper::SaveStringToFile(Receipt,*(FPaths::ProjectSavedDir()/TEXT("HeartlandVisitDone.txt")));
            UE_LOG(LogTemp,Display,TEXT("SOUL_HEARTLAND_WALK_PASS %s"),*Receipt);HandleAction(TEXT("Return"));return;
        }
    }
    const bool Moving=Walker->GetVelocity().SizeSquared2D()>100;
    if(Moving!=bWalkingAnimation){bWalkingAnimation=Moving;Walker->GetMesh()->PlayAnimation(Moving?WalkJog:WalkIdle,true);}
}
