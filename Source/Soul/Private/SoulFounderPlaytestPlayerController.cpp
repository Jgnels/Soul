#include "SoulFounderPlaytestPlayerController.h"

#include "EngineUtils.h"
#include "InputCoreTypes.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "Engine/GameInstance.h"
#include "SoulFounderPlaytestCampaignActor.h"
#include "SoulPlaytestRegionActor.h"

ASoulFounderPlaytestPlayerController::ASoulFounderPlaytestPlayerController()
{
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
void ASoulFounderPlaytestPlayerController::EndDay() { if (auto* C = GetCampaign()) C->EndDay(); }
void ASoulFounderPlaytestPlayerController::ToggleTown() { if (auto* C = GetCampaign()) C->ToggleTownPanel(); }
void ASoulFounderPlaytestPlayerController::HireHero() { if (auto* C = GetCampaign()) C->HireTavernHero(); }
void ASoulFounderPlaytestPlayerController::StartBattle() { if (auto* C = GetCampaign()) C->StartBattle(); }
void ASoulFounderPlaytestPlayerController::Defend() {}
void ASoulFounderPlaytestPlayerController::Wait() {}
void ASoulFounderPlaytestPlayerController::Spell1() {}
void ASoulFounderPlaytestPlayerController::Spell2() {}
void ASoulFounderPlaytestPlayerController::Spell3() {}
void ASoulFounderPlaytestPlayerController::Spell4() { if(auto* S=GetGameInstance()->GetSubsystem<USoulFounderPlaytestStateSubsystem>())S->LoadCampaign(); }
void ASoulFounderPlaytestPlayerController::Spell5() { if(auto* S=GetGameInstance()->GetSubsystem<USoulFounderPlaytestStateSubsystem>())S->SaveCampaign(); }
void ASoulFounderPlaytestPlayerController::Spell6() {}
void ASoulFounderPlaytestPlayerController::CancelPanel() { if (auto* C = GetCampaign()) C->CancelPanel(); }
