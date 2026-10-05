#include "SoulTitanPilot.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputKeyEventArgs.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "UnrealClient.h"

ASoulTitanPilotPawn::ASoulTitanPilotPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(34, 88);
    GetCharacterMovement()->MaxWalkSpeed = 450;
    auto* Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("SoulTraversalCamera"));
    Camera->SetupAttachment(GetRootComponent());
    Camera->SetRelativeLocation(FVector(0, 0, 64));
    Camera->bUsePawnControlRotation = true;
}
void ASoulTitanPilotPawn::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxisKey(EKeys::MouseX, this, &ASoulTitanPilotPawn::LookYaw);
    Input->BindAxisKey(EKeys::MouseY, this, &ASoulTitanPilotPawn::LookPitch);
    Input->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ACharacter::Jump);
    Input->BindKey(EKeys::SpaceBar, IE_Released, this, &ACharacter::StopJumping);
}
void ASoulTitanPilotPawn::LookYaw(float Value) { AddControllerYawInput(Value); }
void ASoulTitanPilotPawn::LookPitch(float Value) { AddControllerPitchInput(-Value); }
void ASoulTitanPilotPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const auto* PC = Cast<APlayerController>(Controller);
    if (!PC) return;
    const FRotator Facing(0, PC->GetControlRotation().Yaw, 0);
    AddMovementInput(Facing.Vector(), (PC->IsInputKeyDown(EKeys::W) ? 1.f : 0.f) - (PC->IsInputKeyDown(EKeys::S) ? 1.f : 0.f));
    AddMovementInput(FRotationMatrix(Facing).GetUnitAxis(EAxis::Y), (PC->IsInputKeyDown(EKeys::D) ? 1.f : 0.f) - (PC->IsInputKeyDown(EKeys::A) ? 1.f : 0.f));
}
void ASoulTitanPilotController::BeginPlay()
{
    Super::BeginPlay();
    bShowMouseCursor = false;
    SetInputMode(FInputModeGameOnly());
}
ASoulTitanPilotGameMode::ASoulTitanPilotGameMode()
{
    DefaultPawnClass = ASoulTitanPilotPawn::StaticClass();
    PlayerControllerClass = ASoulTitanPilotController::StaticClass();
    PrimaryActorTick.bCanEverTick = true;
}
void ASoulTitanPilotGameMode::BeginPlay()
{
    Super::BeginPlay();
    auto* State = GetGameInstance()->GetSubsystem<USoulFounderPlaytestStateSubsystem>();
    if (State) State->InitializeScenario();
    bProof = FParse::Param(FCommandLine::Get(), TEXT("SoulTitanPilotProof"));
    UE_LOG(LogTemp, Display, TEXT("SOUL_TITAN_BEGIN mode=%s controller=%s state=%d persistence=RBSave combat=RefinedBadgerCombat"),
        *GetClass()->GetPathName(), *PlayerControllerClass->GetPathName(), State && State->bInitialized);
}
void ASoulTitanPilotGameMode::Finish(bool Passed, const FString& Reason)
{
    bDone = true;
    UE_LOG(LogTemp, Display, TEXT("SOUL_TITAN_%s reason=%s elapsed=%.2f frames=%d avg_ms=%.2f max_ms=%.2f"),
        Passed ? TEXT("PASS") : TEXT("FAIL"), *Reason, Elapsed, Frames, Frames ? 1000*FrameTotal/Frames : 0, 1000*MaxFrame);
    FPlatformMisc::RequestExitWithStatus(false, Passed ? 0 : 1);
}
void ASoulTitanPilotGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bProof || bDone) return;
    Elapsed += DeltaSeconds;
    PhaseTime += DeltaSeconds;
    if (Elapsed > 180) { Finish(false, TEXT("qualification timeout")); return; }
    if (Elapsed > 10) { ++Frames; FrameTotal += DeltaSeconds; MaxFrame = FMath::Max(MaxFrame, double(DeltaSeconds)); }
    auto* PC = GetWorld()->GetFirstPlayerController();
    auto* Pawn = PC ? Cast<ASoulTitanPilotPawn>(PC->GetPawn()) : nullptr;
    auto* State = GetGameInstance()->GetSubsystem<USoulFounderPlaytestStateSubsystem>();
    auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!Pawn || !State || !State->bInitialized || !Nav) return;
    if (Phase == 0 && Elapsed > 10 && !Nav->IsNavigationBuildInProgress())
    {
        int32 Meshes = 0, Foreign = 0;
        for (TActorIterator<AActor> It(GetWorld()); It; ++It)
        {
            TArray<UStaticMeshComponent*> Components; It->GetComponents(Components);
            for (auto* Component : Components) if (Component->GetStaticMesh()) ++Meshes;
            const FString ClassPath = It->GetClass()->GetPathName();
            if (ClassPath.StartsWith(TEXT("/Script/Titan")) || ClassPath.StartsWith(TEXT("/Game/Blueprint/Framework/"))) ++Foreign;
        }
        UE_LOG(LogTemp, Display, TEXT("SOUL_TITAN_LOADED meshes=%d foreign_authority=%d spawn=%s falling=%d"), Meshes, Foreign, *Pawn->GetActorLocation().ToString(), Pawn->GetCharacterMovement()->IsFalling());
        if (Meshes < 200 || Foreign || Pawn->GetCharacterMovement()->IsFalling()) { Finish(false, TEXT("load/spawn")); return; }
        FNavLocation Start;
        if (!Nav->ProjectPointToNavigation(Pawn->GetActorLocation(), Start, FVector(150,150,300))) { Finish(false, TEXT("spawn nav projection")); return; }
        int32 Paths = 0;
        for (const FVector Offset : {FVector(600,0,0), FVector(0,600,0), FVector(1000,1000,0)})
        {
            FNavLocation End;
            if (!Nav->ProjectPointToNavigation(Start.Location+Offset, End, FVector(200,200,300))) continue;
            auto* Path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), Start.Location, End.Location, Pawn);
            if (Path && Path->IsValid() && !Path->IsPartial()) ++Paths;
        }
        UE_LOG(LogTemp, Display, TEXT("SOUL_TITAN_NAV complete_routes=%d"), Paths);
        if (Paths < 2) { Finish(false, TEXT("navigation routes")); return; }
        int32 InteriorPaths = 0, WallHits = 0;
        double BestInterior = TNumericLimits<double>::Max();
        for (int32 X=-4000; X<=4000; X+=1000) for (int32 Y=-4000; Y<=4000; Y+=1000)
        {
            FNavLocation End;
            if (!Nav->ProjectPointToNavigation(FVector(X,Y,100), End, FVector(150,150,200))) continue;
            auto* Path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), Start.Location, End.Location, Pawn);
            if (!Path || !Path->IsValid() || Path->IsPartial()) continue;
            ++InteriorPaths;
            if (End.Location.SizeSquared2D() < BestInterior)
            {
                BestInterior = End.Location.SizeSquared2D(); InteriorRoute = Path->PathPoints;
            }
        }
        FCollisionQueryParams Query(SCENE_QUERY_STAT(SoulPilotWall), false, Pawn);
        for (int32 Y=-4000; Y<=4000; Y+=1000)
        {
            FHitResult Hit;
            if (GetWorld()->SweepSingleByChannel(Hit, FVector(-6000,Y,100), FVector(6000,Y,100),
                FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(32,86), Query)) ++WallHits;
        }
        UE_LOG(LogTemp, Display, TEXT("SOUL_TITAN_INTERIOR reachable_samples=%d route_points=%d wall_sweeps_blocked=%d"), InteriorPaths, InteriorRoute.Num(), WallHits);
        if (InteriorPaths < 10 || InteriorRoute.Num()<2 || WallHits == 0) { Finish(false, TEXT("interior navigation/collision")); return; }
        MoveStart = Pawn->GetActorLocation();
        PC->SetControlRotation(FRotator::ZeroRotator);
        PC->InputKey(FInputKeyEventArgs(nullptr, FInputDeviceId::CreateFromInternalId(0), EKeys::W, IE_Pressed, FPlatformTime::Cycles64()));
        Phase = 1; PhaseTime = 0;
    }
    else if (Phase == 1 && PhaseTime > 2)
    {
        PC->InputKey(FInputKeyEventArgs(nullptr, FInputDeviceId::CreateFromInternalId(0), EKeys::W, IE_Released, FPlatformTime::Cycles64()));
        const double Distance = FVector::Dist2D(MoveStart, Pawn->GetActorLocation());
        UE_LOG(LogTemp, Display, TEXT("SOUL_TITAN_INPUT keyboard_W_distance=%.2f z=%.2f"), Distance, Pawn->GetActorLocation().Z);
        if (Distance < 200 || Pawn->GetCharacterMovement()->IsFalling()) { Finish(false, TEXT("input/movement collision")); return; }
        Phase = 6; PhaseTime = 0;
    }
    else if (Phase == 6)
    {
        if (PhaseTime > 90) { Finish(false, TEXT("physical interior route timeout")); return; }
        if (RoutePoint < InteriorRoute.Num())
        {
            FVector Direction = InteriorRoute[RoutePoint] - Pawn->GetActorLocation(); Direction.Z = 0;
            if (Direction.Size() < 80) ++RoutePoint;
            else { Pawn->AddMovementInput(Direction.GetSafeNormal()); PC->SetControlRotation(Direction.Rotation()); }
            return;
        }
        UE_LOG(LogTemp, Display, TEXT("SOUL_TITAN_TRAVERSED interior=%s route_seconds=%.2f falling=%d"), *Pawn->GetActorLocation().ToString(), PhaseTime, Pawn->GetCharacterMovement()->IsFalling());
        if (Pawn->GetCharacterMovement()->IsFalling()) { Finish(false, TEXT("interior grounding")); return; }
        FRBSaveDomainState Before; FString Error;
        if (!State->CaptureRBSaveDomain_Implementation(Before, Error)) { Finish(false, Error); return; }
        Snapshot = Before.Fields[0].StringValue;
        State->SaveCampaign(); Phase = 2; PhaseTime = 0;
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/TitanPilot_Traversal.png"), false, false);
    }
    else if (Phase == 2 && !State->bPersistenceBusy)
    {
        if (!State->bLastSaveSucceeded) { Finish(false, TEXT("RBSave save")); return; }
        State->Hero.Experience += 17;
        State->LoadCampaign(); Phase = 3; PhaseTime = 0;
    }
    else if (Phase == 3 && !State->bPersistenceBusy)
    {
        FRBSaveDomainState After; FString Error;
        if (!State->bLastLoadSucceeded || !State->CaptureRBSaveDomain_Implementation(After, Error) || After.Fields[0].StringValue != Snapshot)
        { Finish(false, TEXT("RBSave restoration")); return; }
        UE_LOG(LogTemp, Display, TEXT("SOUL_TITAN_PERSISTENCE_PASS provider=Soul.Campaign storage=RBSave"));
        for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It) { PC->SetViewTarget(*It); break; }
        Phase = 4; PhaseTime = 0;
    }
    else if (Phase == 4 && PhaseTime > 5)
    {
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/TitanPilot_Overview.png"), false, false);
        Phase = 5; PhaseTime = 0;
    }
    else if (Phase == 5 && Elapsed > 75) Finish(true, TEXT("load spawn input navigation RBSave authority"));
}
