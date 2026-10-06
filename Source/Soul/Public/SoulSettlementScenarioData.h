#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SoulSettlement.h"
#include "SoulSettlementScenarioData.generated.h"

class UWorld;
class UStaticMesh;

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

// Authored definitions are inputs to SoulCore rules, never a second mutable town state.
USTRUCT(BlueprintType)
struct SOUL_API FSoulBuildingDevelopmentSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    FName BuildingId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    FName Category;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement", meta=(ClampMin="1"))
    int32 MaxLevel = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement", meta=(ClampMin="1"))
    int32 BuildDays = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    TMap<FName, int32> BuildCost;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    TArray<FName> Prerequisites;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    TArray<FName> UnlockIds;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    FText DisplayName;

    FSoulBuildingDefinition ToDefinition() const;
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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement|Development")
    TArray<FSoulBuildingDevelopmentSpec> DevelopmentDefinitions;

    // Optional owned environment binding; it does not determine ownership or encounter legality.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement|Presentation")
    TSoftObjectPtr<UWorld> OwnedEnvironmentMap;

    // Deterministic derivatives of the same authored scene. The optional
    // completed piece reads the service building's existing saved condition.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement|Presentation")
    TSoftObjectPtr<UStaticMesh> MiniatureBaseMesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement|Presentation")
    TSoftObjectPtr<UStaticMesh> MiniatureUpgradeMesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement|Presentation")
    FTransform MiniatureTransform = FTransform::Identity;

    bool ValidateDefinition(FString& OutError) const;
    const FSoulBuildingDevelopmentSpec* FindDevelopmentDefinition(FName BuildingId) const;
    // The proof's existing service is data-bound, so another approved faction
    // environment does not masquerade as the Human capital or its building IDs.
    const FSoulBuildingDevelopmentSpec* FindUniqueServiceDefinition(FName UnlockId) const;
};
