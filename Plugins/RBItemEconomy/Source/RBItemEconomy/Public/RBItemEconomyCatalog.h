#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "RBItemEconomyCatalog.generated.h"

USTRUCT(BlueprintType)
struct RBITEMECONOMY_API FRBItemShapeCell
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 X = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Y = 0;
};

USTRUCT(BlueprintType)
struct RBITEMECONOMY_API FRBItemAttachmentSlotDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName SlotId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> AcceptedTags;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName SocketName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FTransform RelativeTransform = FTransform::Identity;
};
USTRUCT(BlueprintType)
struct RBITEMECONOMY_API FRBItemDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName DefinitionId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine=true)) FText Description;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName CategoryId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="10000")) int32 Rarity = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) int64 MaxStack = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) int64 MassGrams = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1", ClampMax="64")) int32 Width = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1", ClampMax="64")) int32 Height = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FRBItemShapeCell> Shape;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> Tags;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> EquipmentSlots;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FRBItemAttachmentSlotDefinition> AttachmentSlots;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) int32 BagSlots = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) int64 BagMaxMassGrams = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 DefaultShelfLifeSeconds = -1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName DecaysToDefinitionId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName,int64> RepairInputs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> Icon;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UStaticMesh> WorldMesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UStaticMesh> EquippedStaticMesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<USkeletalMesh> EquippedSkeletalMesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName EquipSocket;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FTransform EquippedRelativeTransform = FTransform::Identity;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bUseLeaderPose = false;
};

USTRUCT(BlueprintType)
struct RBITEMECONOMY_API FRBItemRecipe
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName RecipeId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName CategoryId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName StationTag;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName,int64> Inputs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName,int64> Outputs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName,int64> FuelInputs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> RequiredTools;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) int64 DurationSeconds = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bRequiresUnlock = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="10000")) int32 MinimumQuality = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="10000")) int32 ToolDurabilityCost = 0;
};

USTRUCT(BlueprintType)
struct RBITEMECONOMY_API FRBInitialInventory
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 InventoryId = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName OwnerId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) int32 Slots = 32;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) int64 MaxMassGrams = 100000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="64")) int32 GridWidth = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="64")) int32 GridHeight = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> Tags;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> AcceptedItemTags;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> Delegates;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bPublicAccess = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName,int64> Money;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) int32 DecayRateBasisPoints = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1", ClampMax="128")) int32 CraftQueueLimit = 8;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bCraftEnabled = true;
};

USTRUCT(BlueprintType)
struct RBITEMECONOMY_API FRBInitialItem
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 InventoryId = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName DefinitionId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) int64 Quantity = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="10000")) int32 Quality = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="10000")) int32 Durability = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Provenance;
};

USTRUCT(BlueprintType)
struct RBITEMECONOMY_API FRBInitialVendor
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 VendorId = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 StockInventoryId = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName CurrencyId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName,int64> SellingPrices;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName,int64> BuyingPrices;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> RefusedTags;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) int32 SellMultiplierBasisPoints = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) int32 BuyMultiplierBasisPoints = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) int64 RestockEverySeconds = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) int64 FirstRestockAt = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName,int64> RestockTargets;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName,int64> RestockBatches;
};

USTRUCT(BlueprintType)
struct RBITEMECONOMY_API FRBLootEntry
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName DefinitionId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) int64 Minimum = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) int64 Maximum = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) int32 Weight = 1;
};

USTRUCT(BlueprintType)
struct RBITEMECONOMY_API FRBLootTable
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName LootTableId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FRBLootEntry> Entries;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1", ClampMax="64")) int32 Rolls = 1;
};

USTRUCT(BlueprintType)
struct RBITEMECONOMY_API FRBInitialLootSource
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName SourceId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName LootTableId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 Seed = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) int64 CooldownSeconds = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bRenewable = false;
};

UCLASS(BlueprintType)
class RBITEMECONOMY_API URBItemEconomyCatalog : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FRBItemDefinition> Definitions;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FRBItemRecipe> Recipes;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> Currencies;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FRBInitialInventory> InitialInventories;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FRBInitialItem> InitialItems;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FRBInitialVendor> Vendors;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FRBLootTable> LootTables;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FRBInitialLootSource> LootSources;
};
