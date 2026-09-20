#pragma once

#include "CoreMinimal.h"
#include "RBFoundationTypes.generated.h"

UENUM(BlueprintType)
enum class ERBFoundationTransactionStatus : uint8
{
    Pending,
    Committed,
    Failed
};

UENUM(BlueprintType)
enum class ERBFoundationBeginResult : uint8
{
    NewTransaction,
    ResumePending,
    ReplayCommitted,
    ReplayFailed,
    Conflict,
    Invalid
};

USTRUCT(BlueprintType)
struct RBFOUNDATION_API FRBFoundationTimeState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int64 GameSeconds = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int64 DayIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MinuteOfDay = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName Season = TEXT("Default");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double HourOfDay = 0.0;

    bool IsValid(FString* OutError = nullptr) const;
};

USTRUCT(BlueprintType)
struct RBFOUNDATION_API FRBFoundationOperationResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly) bool bReplayed = false;
    UPROPERTY(BlueprintReadOnly) FString Code;
    UPROPERTY(BlueprintReadOnly) FString Message;
    UPROPERTY(BlueprintReadOnly) int64 Revision = 0;
    UPROPERTY(BlueprintReadOnly) FGuid CorrelationId;
};

USTRUCT(BlueprintType)
struct RBFOUNDATION_API FRBFoundationTransactionParticipantResult
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ParticipantId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bSuccess = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Code;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Message;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) double DurationMs = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCompensationAttempted = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCompensationSucceeded = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString CompensationMessage;
};

USTRUCT(BlueprintType)
struct RBFOUNDATION_API FRBFoundationPersistenceTouch
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName DomainId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int64 Revision = 0;
};

USTRUCT(BlueprintType)
struct RBFOUNDATION_API FRBFoundationTransactionRecord
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FGuid TransactionId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FGuid CorrelationId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Operation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString PayloadFingerprint;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) ERBFoundationTransactionStatus Status = ERBFoundationTransactionStatus::Pending;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int64 DomainRevision = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int64 CreatedGameSeconds = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Error;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FRBFoundationTransactionParticipantResult> Participants;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FRBFoundationPersistenceTouch> PersistenceTouches;
};

USTRUCT(BlueprintType)
struct RBFOUNDATION_API FRBFoundationDependencyDeclaration
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ProductId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString VersionRange = TEXT("*");
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bOptional = false;
};

USTRUCT(BlueprintType)
struct RBFOUNDATION_API FRBFoundationCapabilityDeclaration
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Capability;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName AuthorityDomain;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bExclusive = true;
};

USTRUCT(BlueprintType)
struct RBFOUNDATION_API FRBFoundationProductDescriptor
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 AdapterApiVersion = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ProductId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString ProductName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString SemanticVersion;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString ReleaseIdentity;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString BuildIdentity;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString PluginName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString EngineCompatibility;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> AuthorityDomains;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FRBFoundationCapabilityDeclaration> Capabilities;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FRBFoundationDependencyDeclaration> Dependencies;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> EmitsEvents;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> ConsumesEvents;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> PersistenceDomains;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bParticipatesInPersistence = false;
    bool IsValid(FString* OutError = nullptr) const;
};

USTRUCT(BlueprintType)
struct RBFOUNDATION_API FRBFoundationProductRecord
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FRBFoundationProductDescriptor Descriptor;
    UPROPERTY(BlueprintReadOnly) FString AdapterName;
    UPROPERTY(BlueprintReadOnly) bool bStarted = false;
};

USTRUCT(BlueprintType)
struct RBFOUNDATION_API FRBFoundationCapabilityRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FName Capability;
    UPROPERTY(BlueprintReadOnly) FName AuthorityDomain;
    UPROPERTY(BlueprintReadOnly) FString Version;
    UPROPERTY(BlueprintReadOnly) bool bExclusive = true;
    UPROPERTY(BlueprintReadOnly) FString OwnerName;
};

USTRUCT(BlueprintType)
struct RBFOUNDATION_API FRBFoundationAuthorityRecord
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FName Domain;
    UPROPERTY(BlueprintReadOnly) FString Version;
    UPROPERTY(BlueprintReadOnly) FString OwnerName;
    UPROPERTY(BlueprintReadOnly) FName ProductId;
};

USTRUCT(BlueprintType)
struct RBFOUNDATION_API FRBFoundationHealthIssue
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FName Code;
    UPROPERTY(BlueprintReadOnly) FString Message;
    UPROPERTY(BlueprintReadOnly) FName OwnerDomain;
    UPROPERTY(BlueprintReadOnly) bool bBlocking = true;
};

USTRUCT(BlueprintType)
struct RBFOUNDATION_API FRBFoundationHealthReport
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) bool bReady = false;
    UPROPERTY(BlueprintReadOnly) int32 ProductCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 AuthorityCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 CapabilityCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 TransactionCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 EventConsumerCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 TraceCount = 0;
    UPROPERTY(BlueprintReadOnly) bool bHasCanonicalTimeSource = false;
    UPROPERTY(BlueprintReadOnly) TArray<FRBFoundationHealthIssue> Issues;
};

USTRUCT()
struct RBFOUNDATION_API FRBFoundationSnapshot
{
    GENERATED_BODY()

    UPROPERTY() int32 SchemaVersion = 2;
    UPROPERTY() TArray<FRBFoundationTransactionRecord> Transactions;
    UPROPERTY() FRBFoundationTimeState LastTime;
    UPROPERTY() bool bHasLastTime = false;
};

USTRUCT(BlueprintType)
struct RBFOUNDATION_API FRBFoundationEventConsumerResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) bool bAccepted = true;
    UPROPERTY(BlueprintReadWrite) FString Code;
    UPROPERTY(BlueprintReadWrite) FString Message;
    UPROPERTY(BlueprintReadWrite) FGuid TransactionId;
};

USTRUCT(BlueprintType)
struct RBFOUNDATION_API FRBFoundationTraceRecord
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int64 Sequence = 0;
    UPROPERTY(BlueprintReadOnly) double TimestampSeconds = 0.0;
    UPROPERTY(BlueprintReadOnly) FGuid CorrelationId;
    UPROPERTY(BlueprintReadOnly) FGuid TransactionId;
    UPROPERTY(BlueprintReadOnly) FGuid EventId;
    UPROPERTY(BlueprintReadOnly) FName Stage;
    UPROPERTY(BlueprintReadOnly) FName Source;
    UPROPERTY(BlueprintReadOnly) FName Target;
    UPROPERTY(BlueprintReadOnly) FName Operation;
    UPROPERTY(BlueprintReadOnly) FString Code;
    UPROPERTY(BlueprintReadOnly) FString Message;
    UPROPERTY(BlueprintReadOnly) bool bSuccess = true;
    UPROPERTY(BlueprintReadOnly) double DurationMs = 0.0;
    UPROPERTY(BlueprintReadOnly) int64 Revision = 0;
};

USTRUCT(BlueprintType)
struct RBFOUNDATION_API FRBFoundationDomainEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FGuid EventId;
    UPROPERTY(BlueprintReadOnly) FGuid CorrelationId;
    UPROPERTY(BlueprintReadOnly) FGuid TransactionId;
    UPROPERTY(BlueprintReadOnly) FName SourceDomain;
    UPROPERTY(BlueprintReadOnly) FName EventType;
    UPROPERTY(BlueprintReadOnly) FString Payload;
    UPROPERTY(BlueprintReadOnly) int64 Revision = 0;
};
