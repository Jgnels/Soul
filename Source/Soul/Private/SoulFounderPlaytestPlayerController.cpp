#include "SoulFounderPlaytestPlayerController.h"
#include "SoulCampaignTerrain.h"
#include "Components/InputComponent.h"

#include "EngineUtils.h"
#include "SoulCampaignCamera.h"
#include "SoulCampaignWorldActor.h"
#include "SoulFounderPlaytestHUD.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulSettlementVisitGameMode.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "SoulFounderPlaytestCampaignActor.h"
#include "SoulPlaytestRegionActor.h"

ASoulFounderPlaytestPlayerController::ASoulFounderPlaytestPlayerController()
{
    HitResultTraceDistance=100000.f*SoulCampaignTerrain::Scale();
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
    DefaultMouseCursor = EMouseCursor::Default;
}

void ASoulFounderPlaytestPlayerController::BeginPlay()
{
    Super::BeginPlay();

    FInputModeGameAndUI Mode;
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);
}

void ASoulFounderPlaytestPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    if (!InputComponent) return;

    InputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::ZoomIn);
    InputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::ZoomOut);
    InputComponent->BindKey(EKeys::I, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::InspectNextFaction);
    InputComponent->BindKey(EKeys::Home, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::SoulFocusCompany);
    InputComponent->BindKey(EKeys::One, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::Number1);
    InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::Number2);
    InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::Number3);
    InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::Number4);
    InputComponent->BindKey(EKeys::Five, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::Number5);
    InputComponent->BindKey(EKeys::Six, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::Number6);
    InputComponent->BindKey(EKeys::Seven, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::Number7);
    InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::EndDay);
    InputComponent->BindKey(EKeys::T, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::ToggleTown);
    InputComponent->BindKey(EKeys::H, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::HireHero);
    InputComponent->BindKey(EKeys::U, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::BuildTavern);
    InputComponent->BindKey(EKeys::V, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::VisitSettlement);
    InputComponent->BindKey(EKeys::B, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::StartBattle);
    InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::PrimaryClick);
    InputComponent->BindKey(EKeys::D, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::Defend);
    InputComponent->BindKey(EKeys::W, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::Wait);
    InputComponent->BindKey(EKeys::F1, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::Spell1);
    InputComponent->BindKey(EKeys::F2, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::Spell2);
    InputComponent->BindKey(EKeys::F3, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::Spell3);
    InputComponent->BindKey(EKeys::F9, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::Spell4);
    InputComponent->BindKey(EKeys::F5, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::Spell5);
    InputComponent->BindKey(EKeys::F6, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::Spell6);
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::CancelPanel);
    InputComponent->BindKey(EKeys::Gamepad_FaceButton_Bottom, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::GamepadSelect);
    InputComponent->BindKey(EKeys::Gamepad_FaceButton_Right, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::CancelPanel);
    InputComponent->BindKey(EKeys::Gamepad_FaceButton_Left, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::ToggleTown);
    InputComponent->BindKey(EKeys::Gamepad_FaceButton_Top, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::StartBattle);
    InputComponent->BindKey(EKeys::Gamepad_LeftTrigger, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::ZoomOut);
    InputComponent->BindKey(EKeys::Gamepad_RightTrigger, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::ZoomIn);
    InputComponent->BindKey(EKeys::Gamepad_Special_Left, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::SoulFocusCompany);
    InputComponent->BindKey(EKeys::Gamepad_Special_Right, IE_Pressed, this, &ASoulFounderPlaytestPlayerController::EndDay);
}

void ASoulFounderPlaytestPlayerController::InspectNextFaction()
{
    auto* C=GetCampaign();if(!C||!C->GetState()||!C->GetState()->IsSixFactionProfile())return;
    C->CycleFactionInspection();
    if(auto* Camera=Cast<ASoulCampaignCamera>(GetViewTarget()))Camera->Focus(ASoulCampaignWorldActor::Locations().FindRef(C->GetSelectedRegion()));
}

void ASoulFounderPlaytestPlayerController::GamepadSelect()
{
    // Dispatch through the same cursor/HUD hitboxes and sole world click handler.
    InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton,IE_Pressed,1));
    InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton,IE_Released,0));
}

ASoulFounderPlaytestCampaignActor* ASoulFounderPlaytestPlayerController::GetCampaign() const
{
    if (!GetWorld()) return nullptr;
    for (TActorIterator<ASoulFounderPlaytestCampaignActor> It(GetWorld()); It; ++It)
    {
        return *It;
    }
    return nullptr;
}

void ASoulFounderPlaytestPlayerController::PrimaryClick()
{
    float MX=0, MY=0;
    if (auto* H=Cast<ASoulFounderPlaytestHUD>(GetHUD()))
        if (GetMousePosition(MX,MY) && H->IsPointerOverPanel(MX,MY)) return;
    FHitResult Hit;
    if (!GetHitResultUnderCursor(ECC_Visibility, false, Hit)) return;

    const auto* Region = Cast<ASoulPlaytestRegionActor>(Hit.GetActor());
    if (!IsValid(Region) || Region->IsHidden() || Region->RegionId.IsNone()) return;

    // This is the sole campaign click dispatcher. Actor click notifications may
    // still fire, but region actors must not dispatch the same press again.
    if (auto* Campaign = GetCampaign())
    {
        Campaign->HandleRegionClicked(Region->RegionId);
    }
}

void ASoulFounderPlaytestPlayerController::Number1() { if (auto* C = GetCampaign()) C->HandleNumberKey(1); }
void ASoulFounderPlaytestPlayerController::Number2() { if (auto* C = GetCampaign()) C->HandleNumberKey(2); }
void ASoulFounderPlaytestPlayerController::Number3() { if (auto* C = GetCampaign()) C->HandleNumberKey(3); }
void ASoulFounderPlaytestPlayerController::Number4() { if (auto* C = GetCampaign()) C->HandleNumberKey(4); }
void ASoulFounderPlaytestPlayerController::Number5() { if (auto* C = GetCampaign()) C->HandleNumberKey(5); }
void ASoulFounderPlaytestPlayerController::Number6() { if (auto* C = GetCampaign()) C->HandleNumberKey(6); }
void ASoulFounderPlaytestPlayerController::Number7() { if (auto* C = GetCampaign()) C->HandleNumberKey(7); }
void ASoulFounderPlaytestPlayerController::EndDay() { if (auto* V = GetWorld()->GetAuthGameMode<ASoulSettlementVisitGameMode>()) V->HandleAction(TEXT("EndDay")); else if (auto* C = GetCampaign()) C->EndDay(); }
void ASoulFounderPlaytestPlayerController::ToggleTown() { if (auto* C = GetCampaign()) C->ToggleTownPanel(); }
void ASoulFounderPlaytestPlayerController::HireHero() { if (auto* V = GetWorld()->GetAuthGameMode<ASoulSettlementVisitGameMode>()) V->HandleAction(TEXT("Hire")); else if (auto* C = GetCampaign()) C->HireTavernHero(); }
void ASoulFounderPlaytestPlayerController::BuildTavern() { if (auto* V = GetWorld()->GetAuthGameMode<ASoulSettlementVisitGameMode>()) V->HandleAction(TEXT("BuildTavern")); else if (auto* C = GetCampaign()) C->BuildTavern(); }
void ASoulFounderPlaytestPlayerController::VisitSettlement() { if (auto* C = GetCampaign()) C->VisitSettlement(); }
void ASoulFounderPlaytestPlayerController::StartBattle() { if (auto* C = GetCampaign()) C->StartBattle(); }
void ASoulFounderPlaytestPlayerController::Defend() {}
void ASoulFounderPlaytestPlayerController::Wait() {}
void ASoulFounderPlaytestPlayerController::Spell1() {}
void ASoulFounderPlaytestPlayerController::Spell2() {}
void ASoulFounderPlaytestPlayerController::Spell3() {}
void ASoulFounderPlaytestPlayerController::Spell4() { if(auto* V=GetWorld()->GetAuthGameMode<ASoulSettlementVisitGameMode>())V->HandleAction(TEXT("Load"));else if(auto* S=GetGameInstance()->GetSubsystem<USoulFounderPlaytestStateSubsystem>())S->LoadCampaign(); }
void ASoulFounderPlaytestPlayerController::Spell5() { if(auto* V=GetWorld()->GetAuthGameMode<ASoulSettlementVisitGameMode>())V->HandleAction(TEXT("Save"));else if(auto* S=GetGameInstance()->GetSubsystem<USoulFounderPlaytestStateSubsystem>())S->SaveCampaign(); }
void ASoulFounderPlaytestPlayerController::Spell6() {}
void ASoulFounderPlaytestPlayerController::CancelPanel() { if (auto* V = GetWorld()->GetAuthGameMode<ASoulSettlementVisitGameMode>()) V->HandleAction(TEXT("Return")); else if (auto* C = GetCampaign()) C->CancelPanel(); }

void ASoulFounderPlaytestPlayerController::ZoomIn() { if(auto* C=Cast<ASoulCampaignCamera>(GetViewTarget())) C->Zoom(1); }
void ASoulFounderPlaytestPlayerController::ZoomOut() { if(auto* C=Cast<ASoulCampaignCamera>(GetViewTarget())) C->Zoom(-1); }
void ASoulFounderPlaytestPlayerController::SoulFocusCompany()
{
    if(auto* Campaign=GetCampaign())Campaign->SelectCompany();
    if(auto* C=Cast<ASoulCampaignCamera>(GetViewTarget())) if(auto* S=GetGameInstance()->GetSubsystem<USoulFounderPlaytestStateSubsystem>()) C->Focus(ASoulCampaignWorldActor::Locations().FindRef(S->PlayerRegion));
}
void ASoulFounderPlaytestPlayerController::PlayerTick(float DeltaSeconds)
{
    Super::PlayerTick(DeltaSeconds);
    auto* C=GetCampaign();if(!C)return;
    const FVector2D Stick(GetInputAnalogKeyState(EKeys::Gamepad_RightX),-GetInputAnalogKeyState(EKeys::Gamepad_RightY));
    if(Stick.SizeSquared()>.04f)
    {
        int32 Width=0,Height=0;GetViewportSize(Width,Height);float X=Width*.5f,Y=Height*.5f;GetMousePosition(X,Y);
        const FVector2D Delta=Stick.GetClampedToMaxSize(1.f)*900.f*FMath::Min(DeltaSeconds,.1f);
        if(Width>0&&Height>0)SetMouseLocation(FMath::RoundToInt(FMath::Clamp(X+Delta.X,0.f,Width-1.f)),FMath::RoundToInt(FMath::Clamp(Y+Delta.Y,0.f,Height-1.f)));
    }
    FHitResult Hit; FName Hover;
    if(GetHitResultUnderCursor(ECC_Visibility,false,Hit)) if(auto* R=Cast<ASoulPlaytestRegionActor>(Hit.GetActor()))if(!R->IsHidden())Hover=R->RegionId;
    C->HoveredRegion=Hover;
}
