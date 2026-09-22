#pragma once

#include "CoreMinimal.h"
#include "RBPBILTypes.generated.h"

UENUM(BlueprintType)
enum class ERBPBILChannel : uint8
{
    Threat,
    Congestion,
    Objective,
    MagicHazard,
    Retreat,
    FlankOpportunity,
    FriendlySupport,
    TerrainValue
};

class RBPBIL_API FRBPBILLayers
{
public:
    static FName ToTag(ERBPBILChannel Channel);
    static void GetAll(TArray<FName>& OutTags);
};
USTRUCT(BlueprintType)
struct RBPBIL_API FRBPBILQueryRequest
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB PBIL")
    ERBPBILChannel Channel = ERBPBILChannel::Threat;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB PBIL")
    FVector Origin = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB PBIL", meta=(ClampMin="0"))
    float SearchRadius = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB PBIL")
    bool bFindHighest = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB PBIL")
    bool bReachableOnly = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB PBIL")
    bool bIgnoreHeight = false;
};
USTRUCT(BlueprintType)
struct RBPBIL_API FRBPBILQueryResult
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RB PBIL")
    bool bSuccess = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RB PBIL")
    FVector WorldLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RB PBIL")
    float Value = 0.0f;
};
