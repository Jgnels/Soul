#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SoulSettlementScenarioData.generated.h"

USTRUCT(BlueprintType)
struct SOUL_API FSoulInitialBuildingSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    FName BuildingId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement", meta=(ClampMin="0"))
    int32 Level = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement", meta=(ClampMin="0", ClampMax="1000"))
    int32 IntegrityPermille = 1000;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    bool bBuilt = true;
};

UCLASS(BlueprintType)
class SOUL_API USoulSettlementScenarioData : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    FName SettlementId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    FName FactionId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    FName RegionId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement", meta=(ClampMin="0"))
    int32 FortificationLevel = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement", meta=(ClampMin="0", ClampMax="1000"))
    int32 WallIntegrityPermille = 1000;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    TArray<FSoulInitialBuildingSpec> Buildings;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    TSet<FName> PermanentScars;
};
