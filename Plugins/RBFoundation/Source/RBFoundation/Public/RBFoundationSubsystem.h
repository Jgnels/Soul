#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RBFoundationTypes.h"
#include "RBFoundationSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRBFoundationDomainEventSignature,
    const FRBFoundationDomainEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRBFoundationTraceSignature,
    const FRBFoundationTraceRecord&, Record);

UCLASS()
class RBFOUNDATION_API URBFoundationSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintPure, Category="RB Foundation|Adapter")
    int32 GetSupportedAdapterApiVersion() const { return 1; }

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Adapter")
    bool RegisterProductAdapter(UObject* Adapter, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Adapter")
    bool RegisterDeclarativeProduct(const FRBFoundationProductDescriptor& Descriptor, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Adapter")
    bool RegisterInstalledDeclarativeProducts(FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Adapter")
    void UnregisterProductAdapter(UObject* Adapter);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Adapter")
    bool UnregisterProduct(FName ProductId);

    UFUNCTION(BlueprintPure, Category="RB Foundation|Adapter")
    TArray<FRBFoundationProductRecord> GetProducts() const;

    UFUNCTION(BlueprintPure, Category="RB Foundation|Adapter")
    bool GetProduct(FName ProductId, FRBFoundationProductRecord& OutProduct) const;

    UFUNCTION(BlueprintPure, Category="RB Foundation|Authority")
    TArray<FRBFoundationAuthorityRecord> GetAuthorities() const;

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Authority")
    bool RegisterAuthority(FName Domain, UObject* Owner, const FString& Version, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Authority")
    void UnregisterAuthority(FName Domain, UObject* Owner);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Capability")
    bool RegisterCapability(FName Capability, FName AuthorityDomain, UObject* Owner,
        const FString& Version, bool bExclusive, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Capability")
    void UnregisterCapabilitiesForOwner(UObject* Owner);

    UFUNCTION(BlueprintPure, Category="RB Foundation|Capability")
    bool HasCapability(FName Capability) const;

    UFUNCTION(BlueprintPure, Category="RB Foundation|Capability")
    TArray<FRBFoundationCapabilityRecord> GetCapabilities() const;

    bool SetRequiredCapabilities(const TArray<FName>& Capabilities, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Time")
    bool RegisterTimeSource(UObject* Source, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Time")
    void UnregisterTimeSource(UObject* Source);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Time")
    bool RegisterTimeConsumer(FName ConsumerId, UObject* Consumer, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Time")
    void UnregisterTimeConsumer(FName ConsumerId, UObject* Consumer);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Time")
    bool SynchronizeCanonicalTime(FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Time")
    void ResetCanonicalTimeBaseline();

    ERBFoundationBeginResult BeginTransaction(const FGuid& CorrelationId, FName Operation,
        const FString& PayloadFingerprint, FRBFoundationTransactionRecord& OutRecord,
        FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Transactions")
    ERBFoundationBeginResult BeginTransactionEx(FGuid TransactionId, FGuid CorrelationId, FName Operation,
        const FString& PayloadFingerprint, FRBFoundationTransactionRecord& OutRecord, FString& OutError);

    bool CommitTransaction(const FGuid& CorrelationId, int64 DomainRevision, FString& OutError);
    bool FailTransaction(const FGuid& CorrelationId, const FString& Error, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Transactions")
    bool CommitTransactionById(FGuid TransactionId, int64 DomainRevision, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Transactions")
    bool FailTransactionById(FGuid TransactionId, const FString& Error, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Transactions")
    bool RecordTransactionParticipantResult(FGuid TransactionId, FName ParticipantId, bool bSuccess,
        const FString& Code, const FString& Message, double DurationMs, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Transactions")
    bool RecordTransactionCompensation(FGuid TransactionId, FName ParticipantId, bool bSucceeded,
        const FString& Message, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Transactions")
    bool RecordTransactionPersistence(FGuid TransactionId, FName DomainId, int64 Revision, FString& OutError);

    UFUNCTION(BlueprintPure, Category="RB Foundation|Transactions")
    bool GetTransaction(const FGuid& CorrelationId, FRBFoundationTransactionRecord& OutRecord) const;

    UFUNCTION(BlueprintPure, Category="RB Foundation|Transactions")
    bool GetTransactionById(FGuid TransactionId, FRBFoundationTransactionRecord& OutRecord) const;

    UFUNCTION(BlueprintPure, Category="RB Foundation|Transactions")
    TArray<FRBFoundationTransactionRecord> GetTransactionsForCorrelation(FGuid CorrelationId) const;

    UFUNCTION(BlueprintPure, Category="RB Foundation|Transactions")
    int32 GetTransactionCount() const { return Transactions.Num(); }

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Events")
    bool RegisterEventConsumer(FName ConsumerId, FName ProductId, UObject* Consumer,
        const TArray<FName>& EventTypes, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Events")
    void UnregisterEventConsumer(FName ConsumerId, UObject* Consumer);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Events")
    bool PublishDomainEvent(FGuid CorrelationId, FName SourceDomain, FName EventType,
        const FString& Payload, int64 Revision, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Events")
    bool PublishDomainEventWithTransaction(FGuid CorrelationId, FGuid TransactionId,
        FName SourceDomain, FName EventType, const FString& Payload, int64 Revision, FString& OutError);

    UPROPERTY(BlueprintAssignable, Category="RB Foundation|Events")
    FRBFoundationDomainEventSignature OnDomainEvent;

    UPROPERTY(BlueprintAssignable, Category="RB Foundation|Diagnostics")
    FRBFoundationTraceSignature OnTraceRecord;

    UFUNCTION(BlueprintPure, Category="RB Foundation|Diagnostics")
    TArray<FRBFoundationTraceRecord> GetTraceForCorrelation(FGuid CorrelationId) const;

    UFUNCTION(BlueprintPure, Category="RB Foundation|Diagnostics")
    TArray<FRBFoundationTraceRecord> GetRecentTrace(int32 MaxRecords = 100) const;

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Diagnostics")
    void ClearTrace();

    UFUNCTION(BlueprintPure, Category="RB Foundation|Diagnostics")
    int32 GetTraceCount() const { return TraceRecords.Num(); }

    UFUNCTION(BlueprintPure, Category="RB Foundation|Health")
    FRBFoundationHealthReport EvaluateHealth() const;

    bool VerifyPluginVersion(const FString& PluginName, const FString& ExpectedVersion,
        FString& OutError) const;

    FRBFoundationSnapshot MakeSnapshot() const;
    bool RestoreSnapshot(const FRBFoundationSnapshot& Snapshot, FString& OutError);

private:
    struct FProductAdapterEntry
    {
        TWeakObjectPtr<UObject> Adapter;
        FRBFoundationProductDescriptor Descriptor;
        bool bStarted = false;
    };

    struct FAuthorityEntry
    {
        TWeakObjectPtr<UObject> Owner;
        FString Version;
    };

    struct FCapabilityEntry
    {
        FName AuthorityDomain;
        TWeakObjectPtr<UObject> Owner;
        FString Version;
        bool bExclusive = true;
    };

    struct FEventConsumerEntry
    {
        FName ProductId;
        TWeakObjectPtr<UObject> Consumer;
        TSet<FName> EventTypes;
    };

    UPROPERTY(Transient)
    TArray<TObjectPtr<UObject>> OwnedProductAdapters;

    TMap<FName, FProductAdapterEntry> ProductAdapters;
    TMap<FName, FAuthorityEntry> Authorities;
    TMultiMap<FName, FCapabilityEntry> Capabilities;
    TSet<FName> RequiredCapabilities;
    TWeakObjectPtr<UObject> CanonicalTimeSource;
    TMap<FName, TWeakObjectPtr<UObject>> TimeConsumers;
    TMap<FName, FEventConsumerEntry> EventConsumers;
    TMap<FGuid, FRBFoundationTransactionRecord> Transactions;
    TArray<FRBFoundationTraceRecord> TraceRecords;
    int64 NextTraceSequence = 1;
    FRBFoundationTimeState LastTime;
    bool bHasLastTime = false;
    bool bSynchronizingTime = false;
    FString ManifestRegistrationError;

    void CompactInvalidRegistrations();
    void AppendTrace(FRBFoundationTraceRecord Record);
    FName FindProductIdForOwner(const UObject* Owner) const;
};
