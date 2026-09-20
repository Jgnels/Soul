#pragma once
#include "Components/ActorComponent.h"
#include "RBCombatCommands.h"
#include "RBCombatBlueprintBinding.h"
#include "RBCombatGroupDriver.generated.h"

class URBVariantCombatBindingComponent;
class URBCombatMeleeComponent;
class URBCombatRangedComponent;
class APawn;

UCLASS(Abstract, ClassGroup = (RefinedBadger))
class REFINEDBADGERCOMBATVARIANT_API URBCombatGroupDriver : public UActorComponent
{
    GENERATED_BODY()
public:
    URBCombatGroupDriver();
    // Transient representations only; authoritative group identity comes from ReadGroup.
    UFUNCTION(BlueprintCallable, Category = "RefinedBadger|Combat")
    bool SetRepresentations(const TArray<URBVariantCombatBindingComponent*>& Bindings);
    UPROPERTY(EditAnywhere, Category = "RefinedBadger|Combat") float FormationSpacing = 140;
    UPROPERTY(EditAnywhere, Category = "RefinedBadger|Combat") float SightDistance = 3500;
    // Optional local steering for open arenas without navmesh. Capsule collision
    // still applies; this is not a replacement for navigation around complex terrain.
    UPROPERTY(EditAnywhere, Category = "RefinedBadger|Combat") bool bAllowDirectSteering = false;
    virtual void TickComponent(float Seconds, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    // Qualification-only counters. They expose work already performed by the
    // shared driver without changing target selection or adding durable state.
    void ResetWorkCounters();
    uint64 GetRepresentationFindVisits() const { return RepresentationFindVisits; }
    uint64 GetCandidateVisits() const { return CandidateVisits; }
    uint64 GetOpponentQueries() const { return OpponentQueries; }
    uint64 GetSightTraces() const { return SightTraces; }
protected:
    URBVariantCombatBindingComponent* Find(const FRBCombatantRef& Person) const;
    virtual bool ReadGroup(FRBCombatGroup& Out) const { return false; }
    virtual bool ReadParticipant(const FRBCombatantRef& Person, FRBCombatSituation& Out) const { return false; }
    virtual bool AreOpponents(const FRBCombatantRef& Person, const FRBCombatantRef& Other) const { return false; }
    virtual bool HasAmmunition(const FRBCombatantRef& Person, FName Item) const { return false; }
    virtual bool RequestMeleeFallback(URBVariantCombatBindingComponent* Binding) { return false; }
    virtual void ObserveDecision(const FRBCombatantRef& Person, const FRBCombatDecision& Decision) {}
private:
    void DecideAndDrive(const FRBCombatGroup& Group, URBVariantCombatBindingComponent* Binding);
    void StopMotion(APawn* Pawn);
    void RevokeControl(APawn* Pawn);
    void Move(APawn* Pawn, const FVector& Goal);
    TArray<TWeakObjectPtr<URBVariantCombatBindingComponent>> Representations;
    TMap<TWeakObjectPtr<APawn>, FVector> DirectGoals;
    TSet<TWeakObjectPtr<APawn>> ControlledPawns;
    TMap<TWeakObjectPtr<URBCombatRangedComponent>, FGuid> OwnedDraws;
    TMap<TWeakObjectPtr<URBCombatMeleeComponent>, FGuid> OwnedAttacks;
    float DecisionElapsed = 0;
    mutable uint64 RepresentationFindVisits = 0;
    uint64 CandidateVisits = 0;
    uint64 OpponentQueries = 0;
    uint64 SightTraces = 0;
};

UENUM(BlueprintType)
enum class ERBHostGroupOrder : uint8 { Follow, Hold, Face, Advance, FallBack, Charge };

UENUM(BlueprintType)
enum class ERBHostCombatIntent : uint8 { Hold, Move, Face, Attack, Draw, Release, SwitchToMelee, Flee, Surrender, Incapacitated };

// A snapshot supplied by the game's existing group authority, never durable
// component state. Signed revisions cover Blueprint's nonnegative int64 range.
USTRUCT(BlueprintType)
struct REFINEDBADGERCOMBATVARIANT_API FRBHostGroup
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") FGuid Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") FRBHostIdentity Leader;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") TArray<FRBHostIdentity> Members;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") ERBHostGroupOrder Order = ERBHostGroupOrder::Hold;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") FVector Anchor = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") FVector Facing = FVector::ForwardVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") FRBHostIdentity Focus;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") int64 Revision = 0;
    bool ToCore(FRBCombatGroup& Out) const;
    static bool FromCore(const FRBCombatGroup& Value, FRBHostGroup& Out);
};

USTRUCT(BlueprintType)
struct REFINEDBADGERCOMBATVARIANT_API FRBHostParticipation
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") bool bCanParticipate = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") bool bFleeing = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") bool bSurrendered = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") bool bFriendlyObstruction = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") float RangedMinimum = 220;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") float RangedMaximum = 3200;
};

USTRUCT(BlueprintType)
struct REFINEDBADGERCOMBATVARIANT_API FRBHostDecision
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category = "Combat") ERBHostCombatIntent Intent = ERBHostCombatIntent::Hold;
    UPROPERTY(BlueprintReadOnly, Category = "Combat") FVector Destination = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category = "Combat") FRBHostIdentity Target;
    UPROPERTY(BlueprintReadOnly, Category = "Combat") FName Reason;
};

// Empty callbacks reject participation/commands. The host retains group,
// identity, hostility, equipment, morale and persistence authority.
UCLASS(Blueprintable, ClassGroup = (RefinedBadger), meta = (BlueprintSpawnableComponent))
class REFINEDBADGERCOMBATVARIANT_API URBCombatBlueprintGroupDriver : public URBCombatGroupDriver
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, Category = "RefinedBadger|Host")
    bool HostReadGroup(FRBHostGroup& Group) const;
    UFUNCTION(BlueprintNativeEvent, Category = "RefinedBadger|Host")
    bool HostReadParticipation(FRBHostIdentity Person, FRBHostParticipation& Participation) const;
    UFUNCTION(BlueprintNativeEvent, Category = "RefinedBadger|Host")
    bool HostAreOpponents(FRBHostIdentity Person, FRBHostIdentity Other) const;
    UFUNCTION(BlueprintNativeEvent, Category = "RefinedBadger|Host")
    bool HostHasAmmunition(FRBHostIdentity Person, FName Item) const;
    UFUNCTION(BlueprintNativeEvent, Category = "RefinedBadger|Host")
    bool HostRequestMeleeFallback(URBVariantCombatBindingComponent* Binding);
    UFUNCTION(BlueprintNativeEvent, Category = "RefinedBadger|Host")
    void HostObserveDecision(FRBHostIdentity Person, const FRBHostDecision& Decision);
    // Compare against current durable host state and persist atomically before
    // returning true. Reject without mutation. No latent work is permitted.
    UFUNCTION(BlueprintNativeEvent, Category = "RefinedBadger|Host")
    bool HostCommitGroupOrder(const FRBHostGroup& Previous, const FRBHostGroup& Replacement, FString& Error);
    UFUNCTION(BlueprintCallable, Category = "RefinedBadger|Combat")
    bool RequestGroupOrder(FRBHostIdentity Issuer, ERBHostGroupOrder Order, FVector Anchor,
        FVector Facing, FRBHostIdentity Focus, int64 ExpectedRevision, FString& Error);
protected:
    virtual bool ReadGroup(FRBCombatGroup& Out) const override;
    virtual bool ReadParticipant(const FRBCombatantRef& Person, FRBCombatSituation& Out) const override;
    virtual bool AreOpponents(const FRBCombatantRef& Person, const FRBCombatantRef& Other) const override;
    virtual bool HasAmmunition(const FRBCombatantRef& Person, FName Item) const override;
    virtual bool RequestMeleeFallback(URBVariantCombatBindingComponent* Binding) override;
    virtual void ObserveDecision(const FRBCombatantRef& Person, const FRBCombatDecision& Decision) override;
};
