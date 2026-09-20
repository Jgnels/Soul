#pragma once

#include "CoreMinimal.h"
#include "RBSaveTypes.generated.h"

UENUM(BlueprintType)
enum class ERBSaveFieldType : uint8
{
    Integer,
    Number,
    Boolean,
    String,
    Bytes
};

USTRUCT(BlueprintType)
struct RBSAVE_API FRBSaveField
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) ERBSaveFieldType Type = ERBSaveFieldType::String;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int64 IntegerValue = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) double NumberValue = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool BooleanValue = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString StringValue;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<uint8> BytesValue;
};
USTRUCT(BlueprintType)
struct RBSAVE_API FRBSaveDomainState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName DomainId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 SchemaVersion = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FRBSaveField> Fields;
};

USTRUCT(BlueprintType)
struct RBSAVE_API FRBSaveArtifactInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FName Role;
    UPROPERTY(BlueprintReadOnly) FName BackendId;
    UPROPERTY(BlueprintReadOnly) FString Locator;
    UPROPERTY(BlueprintReadOnly) FString Path;
    UPROPERTY(BlueprintReadOnly) int64 Bytes = 0;
    UPROPERTY(BlueprintReadOnly) int64 Checksum = 0;
};
USTRUCT(BlueprintType)
struct RBSAVE_API FRBSaveGenerationInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) int64 Generation = 0;
    UPROPERTY(BlueprintReadOnly) int64 CreatedUtcMs = 0;
    UPROPERTY(BlueprintReadOnly) FString LogicalSlot;
    UPROPERTY(BlueprintReadOnly) FName WorldBackend;
    UPROPERTY(BlueprintReadOnly) int32 ArtifactCount = 0;
    UPROPERTY(BlueprintReadOnly) TArray<FRBSaveArtifactInfo> Artifacts;
};

USTRUCT(BlueprintType)
struct RBSAVE_API FRBSaveOperationResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly) FString Message;
    UPROPERTY(BlueprintReadOnly) int64 Bytes = 0;
    UPROPERTY(BlueprintReadOnly) int64 Sequence = 0;
    UPROPERTY(BlueprintReadOnly) int64 Checksum = 0;
    UPROPERTY(BlueprintReadOnly) int64 Generation = 0;
    UPROPERTY(BlueprintReadOnly) FName BackendId;
    UPROPERTY(BlueprintReadOnly) FString ArtifactPath;    UPROPERTY(BlueprintReadOnly) int32 ActorRecords = 0;
    UPROPERTY(BlueprintReadOnly) int32 SkippedUnstableActors = 0;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FRBSaveOperationDelegate, const FRBSaveOperationResult&, Result);
