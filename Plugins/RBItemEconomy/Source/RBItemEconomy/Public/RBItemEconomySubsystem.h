#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RBItemEconomyCatalog.h"
#include "RBItemEconomySubsystem.generated.h"

USTRUCT(BlueprintType)
struct RBITEMECONOMY_API FRBItemOperationResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly) bool bReplayed = false;
    UPROPERTY(BlueprintReadOnly) FString Code;
    UPROPERTY(BlueprintReadOnly) FString Message;
    UPROPERTY(BlueprintReadOnly) int64 Revision = 0;
    UPROPERTY(BlueprintReadOnly) TArray<int64> CreatedIds;
};

UENUM(BlueprintType)
enum class ERBInventorySort : uint8
{
    Slot,
    Name,
    Category,
    Quantity,
    Quality,
    Durability,
    Mass,
    Expiry,
    Rarity
};

USTRUCT(BlueprintType)
struct RBITEMECONOMY_API FRBInventoryQuery
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString SearchText;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName CategoryId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> RequiredTags;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bFavoritesOnly = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIncludeExpired = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) ERBInventorySort Sort = ERBInventorySort::Slot;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bDescending = false;
};

USTRUCT(BlueprintType)
struct RBITEMECONOMY_API FRBItemView
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int64 ItemId = 0;
    UPROPERTY(BlueprintReadOnly) int64 InventoryId = 0;
    UPROPERTY(BlueprintReadOnly) FName DefinitionId;
    UPROPERTY(BlueprintReadOnly) FText DisplayName;
    UPROPERTY(BlueprintReadOnly) FText Description;
    UPROPERTY(BlueprintReadOnly) FName CategoryId;
    UPROPERTY(BlueprintReadOnly) TArray<FName> Tags;
    UPROPERTY(BlueprintReadOnly) int32 Rarity = 0;
    UPROPERTY(BlueprintReadOnly) int64 Quantity = 0;
    UPROPERTY(BlueprintReadOnly) int64 Reserved = 0;
    UPROPERTY(BlueprintReadOnly) int32 Quality = 0;
    UPROPERTY(BlueprintReadOnly) int32 Durability = 0;
    UPROPERTY(BlueprintReadOnly) int64 MassGrams = 0;
    UPROPERTY(BlueprintReadOnly) int64 ExpiresAt = -1;
    UPROPERTY(BlueprintReadOnly) int64 RemainingShelfLife = -1;
    UPROPERTY(BlueprintReadOnly) int64 ChildInventoryId = 0;
    UPROPERTY(BlueprintReadOnly) int64 AttachmentInventoryId = 0;
    UPROPERTY(BlueprintReadOnly) int64 ParentHostItemId = 0;
    UPROPERTY(BlueprintReadOnly) FName AttachmentSlotId;
    UPROPERTY(BlueprintReadOnly) TArray<FRBItemAttachmentSlotDefinition> AttachmentSlots;
    UPROPERTY(BlueprintReadOnly) TSoftObjectPtr<UStaticMesh> WorldMesh;
    UPROPERTY(BlueprintReadOnly) TSoftObjectPtr<UStaticMesh> EquippedStaticMesh;
    UPROPERTY(BlueprintReadOnly) TSoftObjectPtr<USkeletalMesh> EquippedSkeletalMesh;
    UPROPERTY(BlueprintReadOnly) FName EquipSocket;
    UPROPERTY(BlueprintReadOnly) FTransform EquippedRelativeTransform = FTransform::Identity;
    UPROPERTY(BlueprintReadOnly) bool bUseLeaderPose = false;
    UPROPERTY(BlueprintReadOnly) int32 Slot = 0;
    UPROPERTY(BlueprintReadOnly) int32 X = 0;
    UPROPERTY(BlueprintReadOnly) int32 Y = 0;
    UPROPERTY(BlueprintReadOnly) bool bRotated = false;
    UPROPERTY(BlueprintReadOnly) bool bFavorite = false;
    UPROPERTY(BlueprintReadOnly) bool bLocked = false;
    UPROPERTY(BlueprintReadOnly) bool bEquipped = false;
    UPROPERTY(BlueprintReadOnly) TSoftObjectPtr<UTexture2D> Icon;
};

USTRUCT(BlueprintType)
struct RBITEMECONOMY_API FRBInventoryView
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int64 InventoryId = 0;
    UPROPERTY(BlueprintReadOnly) FName OwnerId;
    UPROPERTY(BlueprintReadOnly) int32 Slots = 0;
    UPROPERTY(BlueprintReadOnly) int64 UsedMassGrams = 0;
    UPROPERTY(BlueprintReadOnly) int64 MaxMassGrams = 0;
    UPROPERTY(BlueprintReadOnly) int32 GridWidth = 0;
    UPROPERTY(BlueprintReadOnly) int32 GridHeight = 0;
    UPROPERTY(BlueprintReadOnly) int32 DecayRateBasisPoints = 10000;
    UPROPERTY(BlueprintReadOnly) TMap<FName,int64> Money;
};

USTRUCT(BlueprintType)
struct RBITEMECONOMY_API FRBRecipeView
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FName RecipeId;
    UPROPERTY(BlueprintReadOnly) FText DisplayName;
    UPROPERTY(BlueprintReadOnly) FName CategoryId;
    UPROPERTY(BlueprintReadOnly) FName StationTag;
    UPROPERTY(BlueprintReadOnly) TMap<FName,int64> Inputs;
    UPROPERTY(BlueprintReadOnly) TMap<FName,int64> Outputs;
    UPROPERTY(BlueprintReadOnly) TMap<FName,int64> FuelInputs;
    UPROPERTY(BlueprintReadOnly) TArray<FName> RequiredTools;
    UPROPERTY(BlueprintReadOnly) int64 DurationSeconds = 0;
    UPROPERTY(BlueprintReadOnly) bool bUnlocked = true;
    UPROPERTY(BlueprintReadOnly) int32 MinimumQuality = 0;
};

USTRUCT(BlueprintType)
struct RBITEMECONOMY_API FRBCraftJobView
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int64 JobId = 0;
    UPROPERTY(BlueprintReadOnly) FName RecipeId;
    UPROPERTY(BlueprintReadOnly) int64 StationId = 0;
    UPROPERTY(BlueprintReadOnly) int64 DestinationId = 0;
    UPROPERTY(BlueprintReadOnly) int64 Batches = 1;
    UPROPERTY(BlueprintReadOnly) int64 StartedAt = 0;
    UPROPERTY(BlueprintReadOnly) int64 DueAt = 0;
    UPROPERTY(BlueprintReadOnly) bool bReady = false;
};

USTRUCT(BlueprintType)
struct RBITEMECONOMY_API FRBVendorQuoteView
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) bool bValid = false;
    UPROPERTY(BlueprintReadOnly) FString Code;
    UPROPERTY(BlueprintReadOnly) int64 Total = 0;
    UPROPERTY(BlueprintReadOnly) int64 Revision = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRBItemEconomyCommitted,int64,Revision);

// Authoritative server/single-player economy facade. UI may read copies, but it never owns state.
// Multiplayer games must derive ActorId and interaction authorization on the server, not from client payloads.
UCLASS()
class RBITEMECONOMY_API URBItemEconomySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    URBItemEconomySubsystem();
    virtual ~URBItemEconomySubsystem() override;
    virtual void Deinitialize() override;

    UPROPERTY(BlueprintAssignable) FRBItemEconomyCommitted OnCommitted;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Items")
    bool InitializeCatalog(URBItemEconomyCatalog* Catalog,FString& Error);
    UFUNCTION(BlueprintPure, Category="RB Items") int64 GetRevision() const;
    UFUNCTION(BlueprintPure, Category="RB Items") int64 GetNextSequence(FName ActorId) const;

    UFUNCTION(BlueprintCallable, Category="RB Items|Read")
    bool ReadInventorySummary(FName ActorId,int64 InventoryId,FRBInventoryView& View,FString& Error) const;
    UFUNCTION(BlueprintCallable, Category="RB Items|Read")
    TArray<FRBItemView> ReadInventory(FName ActorId,int64 InventoryId,FString& Error) const;
    UFUNCTION(BlueprintCallable, Category="RB Items|Read")
    bool ReadItem(FName ActorId,int64 ItemId,FRBItemView& View,FString& Error) const;
    UFUNCTION(BlueprintCallable, Category="RB Items|Read")
    TArray<FRBItemView> ReadAttachments(FName ActorId,int64 HostItemId,FString& Error) const;
    UFUNCTION(BlueprintCallable, Category="RB Items|Read")
    TArray<FRBItemView> QueryInventory(FName ActorId,int64 InventoryId,const FRBInventoryQuery& Query,FString& Error) const;
    UFUNCTION(BlueprintCallable, Category="RB Items|Read")
    TArray<FRBRecipeView> ReadRecipes(FName ActorId,FName StationTag) const;
    UFUNCTION(BlueprintCallable, Category="RB Items|Read")
    TArray<FRBCraftJobView> ReadCraftJobs(FName ActorId,int64 StationId,FString& Error) const;
    UFUNCTION(BlueprintCallable, Category="RB Items|Read")
    TMap<int32,int64> ReadHotbar(FName ActorId) const;
    UFUNCTION(BlueprintCallable, Category="RB Items|Read")
    TMap<FName,int64> ReadEquipment(FName ActorId) const;
    UFUNCTION(BlueprintPure, Category="RB Items|Read")
    int64 GetVendorStockInventoryId(int64 VendorId) const;
    UFUNCTION(BlueprintCallable, Category="RB Items|Read")
    TArray<FRBItemView> ReadVendorStock(int64 VendorId,FString& Error) const;
    UFUNCTION(BlueprintCallable, Category="RB Items|Read")
    FRBVendorQuoteView QuoteVendor(int64 VendorId,int64 ItemId,int64 Quantity,bool bBuying) const;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Items")
    FRBItemOperationResult TransferItem(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 ItemId,int64 DestinationId,int64 Quantity,int32 Slot=-1,int32 X=-1,int32 Y=-1,bool bRotated=false);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Items")
    FRBItemOperationResult TransferAllItems(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 SourceId,int64 DestinationId);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Items")
    FRBItemOperationResult QuickStackItems(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 SourceId,int64 DestinationId);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Items")
    FRBItemOperationResult MergeItems(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 SourceId,int64 TargetId,int64 Quantity);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Items")
    FRBItemOperationResult ConsumeOrDiscard(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 ItemId,int64 Quantity,bool bDiscard);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Items")
    FRBItemOperationResult SetItemFlags(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 ItemId,bool bSetFavorite,bool bFavorite,bool bSetLocked,bool bLocked);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Items")
    FRBItemOperationResult BindHotbar(FName ActorId,int64 Sequence,int64 ExpectedRevision,int32 Slot,int64 ItemId);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Items")
    FRBItemOperationResult SetEquipment(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 ItemId,bool bUnequip);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Items")
    FRBItemOperationResult RepairItem(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 ItemId,const TArray<int64>& Sources,int32 TargetDurability=10000);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Items")
    FRBItemOperationResult AttachItem(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 HostItemId,int64 AttachmentItemId,FName AttachmentSlotId);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Items")
    FRBItemOperationResult DetachItem(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 AttachmentItemId,int64 DestinationId,int32 Slot=-1,int32 X=-1,int32 Y=-1,bool bRotated=false);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Items|Crafting")
    FRBItemOperationResult BeginCraft(FName ActorId,int64 Sequence,int64 ExpectedRevision,FName Recipe,int64 StationId,int64 DestinationId,const TArray<int64>& Sources,int64 Batches=1);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Items|Crafting")
    FRBItemOperationResult CompleteCraft(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 JobId,bool bCancel);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Items|Trusted Host")
    FRBItemOperationResult CommitVerifiedTrade(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 VendorId,int64 ItemId,int64 CustomerInventoryId,int64 Quantity,int64 QuotedTotal,bool bBuying);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Items|Trusted Host")
    FRBItemOperationResult RestockVendor(int64 VendorId,int64 ExpectedGeneration);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Items|Trusted Host")
    FRBItemOperationResult CommitVerifiedLootRoll(FName SourceId,int64 ExpectedGeneration,int64 DestinationId);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Items|Trusted Host")
    FRBItemOperationResult CommitProductionAward(FName EventId,int64 DestinationId,FName DefinitionId,int64 Quantity,int32 Quality=10000);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Items|Trusted Host")
    FRBItemOperationResult UnlockRecipe(FName ActorId,FName RecipeId);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Items|Trusted Host")
    FRBItemOperationResult AdvanceCanonicalTime(int64 GameSeconds);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Items|Save")
    bool CaptureEconomy(TArray<uint8>& Bytes,FString& Error) const;
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Items|Save")
    bool RestoreEconomy(const TArray<uint8>& Bytes,FString& Error);

private:
    struct FImpl;
    FImpl* Impl = nullptr;
    bool MayMutate() const;
};
