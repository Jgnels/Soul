#include "SoulSettlementVisitGameMode.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
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

ASoulSettlementVisitGameMode::ASoulSettlementVisitGameMode()
{
    PlayerControllerClass = ASoulFounderPlaytestPlayerController::StaticClass();
    HUDClass = ASoulFounderPlaytestHUD::StaticClass();
    DefaultPawnClass = nullptr;
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = .25f;
}

bool ASoulSettlementVisitGameMode::CanVisit(const USoulFounderPlaytestStateSubsystem* Campaign, FString& OutError)
{
    auto Reject = [&OutError](const TCHAR* Reason) { OutError = Reason; return false; };
    if (!Campaign || !Campaign->IsSettlementDevelopmentReady())
        return Reject(TEXT("The settlement development environment is not ready."));
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
    if (!State || !LastFailure.IsEmpty()) return;
    if (!bCameraReady)
    {
        for (TActorIterator<ASoulTownViewAnchor> It(GetWorld()); It; ++It)
            if (It->SettlementId == State->GetSettlementScenario()->SettlementId
                && It->ActivateForPlayer(GetWorld()->GetFirstPlayerController(), 0))
            {
                bCameraReady = true;
                LastMessage = TEXT("Visit the settlement. Complete the ") + State->GetDevelopmentBuildingName() + TEXT(" to open companion hiring.");
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
        RefreshPresentation();
        if (!CanVisit(State, LastFailure)) LastMessage = LastFailure + TEXT(" Return to the campaign.");
    }
}

void ASoulSettlementVisitGameMode::HandleAction(FName Action)
{
    if (!State) return;
    if (Action == TEXT("Return"))
    {
        if (State->HasPendingBattle() || State->bPersistenceBusy)
        { LastMessage = TEXT("Finish the current battle or save/load before returning."); return; }
        if (State->CampaignMap.IsNone() || !FPackageName::DoesPackageExist(State->CampaignMap.ToString()))
        { LastMessage = TEXT("The campaign return map is unavailable."); return; }
        UGameplayStatics::OpenLevel(this, State->CampaignMap, true, TEXT("game=/Script/Soul.SoulFounderPlaytestGameMode"));
        return;
    }
    if (!IsVisitReady() || !CanVisit(State, LastMessage)) return;
    if (Action == TEXT("BuildTavern"))
    {
        if (State->BeginSettlementConstruction(State->GetTavernBuildingId(), LastMessage)) LastMessage = State->GetDevelopmentBuildingName() + TEXT(" construction started.");
    }
    else if (Action == TEXT("EndDay")) { State->AdvanceDay(); LastMessage = TEXT("A new day begins."); }
    else if (Action == TEXT("Hire"))
        LastMessage = State->HireTavernHero() ? TEXT("Tavern companion hired.")
            : TEXT("Hiring requires an operational tavern, 1200 gold and an available companion.");
    else if (Action == TEXT("Save")) State->SaveCampaign();
    else if (Action == TEXT("Load")) State->LoadCampaign();
    RefreshPresentation();
}
