#include "SoulSettlementBootstrapActor.h"

#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "SoulSettlement.h"
#include "SoulSettlementPresentationController.h"
#include "SoulSettlementScenarioData.h"
#include "SoulSettlementStateSubsystem.h"

ASoulSettlementBootstrapActor::ASoulSettlementBootstrapActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ASoulSettlementBootstrapActor::BeginPlay()
{
    Super::BeginPlay();
    ApplyScenario();
}

bool ASoulSettlementBootstrapActor::ApplyScenario()
{
    if (!Scenario || Scenario->SettlementId.IsNone() || !GetWorld())
    {
        return false;
    }

    UGameInstance* GameInstance = GetWorld()->GetGameInstance();
    USoulSettlementStateSubsystem* State = GameInstance
        ? GameInstance->GetSubsystem<USoulSettlementStateSubsystem>()
        : nullptr;
    if (!State)
    {
        return false;
    }

    FString Error;
    if (!State->EnsureScenario(Scenario, Error, bOnlyCreateIfMissing))
    {
        UE_LOG(LogTemp, Error, TEXT("SOUL_SETTLEMENT_BOOTSTRAP_FAIL %s"), *Error);
        return false;
    }

    // A map can bootstrap before or after presentation actors. Refresh all matching
    // controllers immediately so town/siege views agree with canonical state.
    for (TActorIterator<ASoulSettlementPresentationController> It(GetWorld()); It; ++It)
    {
        ASoulSettlementPresentationController* Controller = *It;
        if (Controller && Controller->SettlementId == Scenario->SettlementId)
        {
            Controller->RefreshSettlementPresentation();
        }
    }

    return true;
}
