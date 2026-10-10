#include "SoulSettlementVisitGameMode.h"
#include "Components/TextRenderComponent.h"
#include "InputKeyEventArgs.h"
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
    RefreshCompanion();
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
    if(Walker&&!Companion&&State->bSecondHeroHired&&IsStreamingReady())
    {
        CompanionRetryTime+=Seconds;
        if(CompanionRetryTime>=1){CompanionRetryTime=0;RefreshCompanion();}
    }
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
    else if(Action==TEXT("CompanionAssign")){State->AssignHeartlandCompanion(!State->bCompanionAssigned);LastMessage=State->CompanionStatus();}
    else if(Action.ToString().StartsWith(TEXT("Company:"))){LastMessage=State->Recruit(FName(*Action.ToString().Mid(8)))?TEXT("Paid recruit joined your company."):TEXT("Requires operational barracks (level 2 for guards), stock, gold and movement.");}
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

void ASoulSettlementVisitGameMode::RefreshCompanion()
{
    if(!State||!State->IsHeartlandEnabled()||!Walker||!IsStreamingReady())return;
    if(!State->bSecondHeroHired){if(Companion){Companion->Destroy();Companion=nullptr;}return;}
    if(Companion)
    {
        if(auto* Label=Companion->FindComponentByClass<UTextRenderComponent>())
            Label->SetText(FText::FromString(State->bCompanionAssigned?TEXT("Rowan | Companion"):TEXT("Rowan | Unassigned")));
        return;
    }
    auto* Mesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Knights_Pack/Meshes/Knight_04/Mesh_UE4/Full_Mesh/SK_Knight_04_Full_01.SK_Knight_04_Full_01"));
    auto* Idle=LoadObject<UAnimationAsset>(nullptr,TEXT("/Game/Knights_Pack/Demoscene_UE4/Animations/ThirdPersonIdle.ThirdPersonIdle"));
    if(!Mesh||!Idle)return;
    // A successful simple capsule spawn is insufficient against authored walls
    // whose visible geometry differs from simple collision. Use the actual
    // nearby street and a complex standing-volume sweep before committing.
    FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_WorldStatic);
    FCollisionQueryParams Q(SCENE_QUERY_STAT(SoulCompanionStreet),true);Q.AddIgnoredActor(Walker);
    FVector Ground;bool Clear=false;
    for(const FVector2D Offset:{FVector2D(250,120),FVector2D(450,120),FVector2D(600,100),FVector2D(400,350),FVector2D(0,350),FVector2D(-200,350),FVector2D(0,650),FVector2D(0,1000),FVector2D(-500,500),FVector2D(500,1000),FVector2D(-700,700),FVector2D(650,-350)})
    {
        const FVector Spot=Walker->GetActorLocation()+FVector(Offset,0);FHitResult Floor,Body;
        if(!GetWorld()->LineTraceSingleByObjectType(Floor,Spot+FVector(0,0,400),Spot-FVector(0,0,800),Objects,Q)||Floor.ImpactNormal.Z<.75f)continue;
        if(GetWorld()->SweepSingleByObjectType(Body,Floor.ImpactPoint+FVector(0,0,55),Floor.ImpactPoint+FVector(0,0,160),FQuat::Identity,Objects,FCollisionShape::MakeSphere(45),Q))continue;
        // Standing sweeps may begin inside a late streamed mesh. Require a
        // clear view from the player and at least one short open street sightline.
        const FVector Eye=Floor.ImpactPoint+FVector(0,0,123);FHitResult Sight;
        if(GetWorld()->LineTraceSingleByObjectType(Sight,Walker->GetActorLocation()+FVector(0,0,50),Eye,Objects,Q))continue;
        bool OpenStreet=false;
        for(int32 I=0;I<8;++I)
            if(!GetWorld()->LineTraceSingleByObjectType(Sight,Eye,Eye+FRotator(0,I*45,0).RotateVector(FVector(380,0,100)),Objects,Q)){OpenStreet=true;break;}
        if(!OpenStreet)continue;
        Ground=Floor.ImpactPoint;Clear=true;break;
    }
    if(!Clear)return;
    FActorSpawnParameters P;P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
    const FRotator Greeting(0,(Walker->GetActorLocation()-Ground).Rotation().Yaw,0);
    Companion=GetWorld()->SpawnActor<ACharacter>(ACharacter::StaticClass(),Ground+FVector(0,0,88),Greeting,P);
    if(!Companion)return;
    Companion->Tags.Add(TEXT("Soul.Companion.Rowan"));Companion->GetMesh()->SetSkeletalMesh(Mesh);
    Companion->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Companion->GetMesh()->SetRelativeLocation(FVector(0,0,-88));Companion->GetMesh()->SetRelativeRotation(FRotator(0,-90,0));Companion->GetMesh()->PlayAnimation(Idle,true);
    auto* Label=NewObject<UTextRenderComponent>(Companion);Label->SetupAttachment(Companion->GetRootComponent());
    Companion->AddInstanceComponent(Label);Label->SetRelativeLocation(FVector(0,0,130));
    Label->SetRelativeRotation(FRotator::ZeroRotator);Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(18);Label->SetTextRenderColor(FColor(240,214,155));
    Label->SetText(FText::FromString(State->bCompanionAssigned?TEXT("Rowan | Companion"):TEXT("Rowan | Unassigned")));Label->RegisterComponent();
    UE_LOG(LogTemp,Display,TEXT("SOUL_HEARTLAND_COMPANION_VISIBLE id=human.rowan assigned=%d location=%s"),State->bCompanionAssigned,*Companion->GetActorLocation().ToString());
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
    RefreshCompanion();
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
    if(FParse::Param(FCommandLine::Get(),TEXT("SoulHeartlandDepthQualification"))){TickDepthVisitQualification(Seconds);return;}
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
