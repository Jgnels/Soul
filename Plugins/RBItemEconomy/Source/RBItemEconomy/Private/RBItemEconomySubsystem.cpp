#include "RBItemEconomySubsystem.h"
#include "Core/RBItemEconomyCore.h"
#include "Engine/World.h"
#include <algorithm>
#include <stdexcept>

using namespace RB::Items;

namespace {
std::string Str(FName Name){return TCHAR_TO_UTF8(*Name.ToString().ToLower());}
FName Name(const std::string& Value){return FName(UTF8_TO_TCHAR(Value.c_str()));}
const std::string Service="rb.service.item-economy";

FRBItemOperationResult View(const Result& R){
    FRBItemOperationResult V;
    V.bSuccess=static_cast<bool>(R);V.bReplayed=R.replayed;
    V.Code=UTF8_TO_TCHAR(errorName(R.error));V.Message=UTF8_TO_TCHAR(R.message.c_str());
    V.Revision=static_cast<int64>(R.revision);
    for(Id I:R.created)V.CreatedIds.Add(static_cast<int64>(I));
    return V;
}
FRBItemOperationResult Denied(){return View({RB::Items::Error::Forbidden,"server game-thread authority or initialization required",0,{},false});}

const FRBItemDefinition* FindDefinition(const URBItemEconomyCatalog* Data,const std::string& Id){
    if(!Data)return nullptr;
    const FName Wanted=Name(Id);
    return Data->Definitions.FindByPredicate([&](const FRBItemDefinition& D){return D.DefinitionId==Wanted;});
}
bool IsEquipped(const State& StateValue,Id ItemId){
    for(const auto& Actor:StateValue.equipment)for(const auto& Slot:Actor.second)if(Slot.second==ItemId)return true;
    return false;
}
}

struct URBItemEconomySubsystem::FImpl {
    std::unique_ptr<Engine> Economy;
    TWeakObjectPtr<URBItemEconomyCatalog> CatalogAsset;
    bool bDispatching=false;

    FRBItemOperationResult Apply(URBItemEconomySubsystem* Owner,const Context& Ctx,const Command& Cmd){
        if(bDispatching || !Economy)return Denied();
        auto R=Economy->execute(Ctx,Cmd);auto V=View(R);
        if(R && !R.replayed){TGuardValue<bool> Guard(bDispatching,true);Owner->OnCommitted.Broadcast(V.Revision);}
        return V;
    }
    Command Next(Operation Op){
        auto P=Economy->state().receipts.find(Service);
        return {P==Economy->state().receipts.end()?1:P->second.sequence+1,Economy->state().revision,std::move(Op)};
    }
};

URBItemEconomySubsystem::URBItemEconomySubsystem():Impl(new FImpl()){}
URBItemEconomySubsystem::~URBItemEconomySubsystem(){delete Impl;Impl=nullptr;}
void URBItemEconomySubsystem::Deinitialize(){Impl->Economy.reset();Impl->CatalogAsset.Reset();Super::Deinitialize();}

bool URBItemEconomySubsystem::MayMutate() const {
    return IsInGameThread() && GetWorld() && GetWorld()->GetNetMode()!=NM_Client && Impl && !Impl->bDispatching;
}

bool URBItemEconomySubsystem::InitializeCatalog(URBItemEconomyCatalog* Data,FString& Error){
    Error.Reset();
    if(!MayMutate() || !Data || Impl->Economy){Error=TEXT("Server initialization required; existing economy cannot be silently replaced.");return false;}
    try {
        Catalog C;State S;
        for(FName N:Data->Currencies)if(!C.currencies.insert(Str(N)).second)throw std::runtime_error("duplicate currency");
        for(const auto& V:Data->Definitions){
            if(V.DefinitionId.IsNone())throw std::runtime_error("definition ID required");
            Definition D;D.id=Str(V.DefinitionId);D.category=V.CategoryId.IsNone()?std::string():Str(V.CategoryId);D.rarity=V.Rarity;
            D.maxStack=V.MaxStack;D.unitMassGrams=V.MassGrams;D.width=V.Width;D.height=V.Height;
            D.bagSlots=V.BagSlots;D.bagMaxMassGrams=V.BagMaxMassGrams;D.defaultShelfLifeSeconds=V.DefaultShelfLifeSeconds;
            D.decaysTo=V.DecaysToDefinitionId.IsNone()?std::string():Str(V.DecaysToDefinitionId);
            for(const auto& Cell:V.Shape)D.shape.push_back({Cell.X,Cell.Y});
            for(FName N:V.Tags)D.tags.insert(Str(N));for(FName N:V.EquipmentSlots)D.equipmentSlots.insert(Str(N));
            for(const auto& A:V.AttachmentSlots){std::set<std::string> Tags;for(FName N:A.AcceptedTags)Tags.insert(Str(N));if(!D.attachmentSlots.emplace(Str(A.SlotId),std::move(Tags)).second)throw std::runtime_error("duplicate attachment slot");}
            for(const auto& P:V.RepairInputs)D.repairInputs.emplace(Str(P.Key),P.Value);
            if(!C.definitions.emplace(D.id,std::move(D)).second)throw std::runtime_error("duplicate definition");
        }
        for(const auto& V:Data->Recipes){
            if(V.RecipeId.IsNone() || V.StationTag.IsNone())throw std::runtime_error("recipe IDs required");
            Recipe R;R.id=Str(V.RecipeId);R.stationTag=Str(V.StationTag);R.durationSeconds=V.DurationSeconds;
            R.requiresUnlock=V.bRequiresUnlock;R.minQuality=V.MinimumQuality;R.toolDurabilityCost=V.ToolDurabilityCost;
            for(const auto& P:V.Inputs)R.inputs.emplace(Str(P.Key),P.Value);for(const auto& P:V.Outputs)R.outputs.emplace(Str(P.Key),P.Value);
            for(const auto& P:V.FuelInputs)R.fuelInputs.emplace(Str(P.Key),P.Value);for(FName N:V.RequiredTools)R.requiredTools.insert(Str(N));
            if(!C.recipes.emplace(R.id,std::move(R)).second)throw std::runtime_error("duplicate recipe");
        }
        for(const auto& V:Data->LootTables){
            if(V.LootTableId.IsNone())throw std::runtime_error("loot table ID required");LootTable T;T.id=Str(V.LootTableId);T.rolls=static_cast<std::uint32_t>(V.Rolls);
            for(const auto& Entry:V.Entries)T.entries.push_back({Str(Entry.DefinitionId),Entry.Minimum,Entry.Maximum,static_cast<std::uint32_t>(Entry.Weight)});
            if(!C.lootTables.emplace(T.id,std::move(T)).second)throw std::runtime_error("duplicate loot table");
        }
        for(const auto& V:Data->InitialInventories){
            if(V.InventoryId<=0 || V.OwnerId.IsNone())throw std::runtime_error("positive inventory ID and owner required");
            Container I;I.id=static_cast<Id>(V.InventoryId);I.owner=Str(V.OwnerId);I.slots=V.Slots;I.maxMassGrams=V.MaxMassGrams;
            I.gridWidth=V.GridWidth;I.gridHeight=V.GridHeight;I.publicAccess=V.bPublicAccess;I.decayRateBasisPoints=V.DecayRateBasisPoints;
            I.craftQueueLimit=V.CraftQueueLimit;I.craftEnabled=V.bCraftEnabled;
            for(FName N:V.Tags)I.tags.insert(Str(N));for(FName N:V.AcceptedItemTags)I.acceptedTags.insert(Str(N));for(FName N:V.Delegates)I.delegates.insert(Str(N));
            for(const auto& P:V.Money)I.money.emplace(Str(P.Key),P.Value);
            S.nextId=std::max(S.nextId,I.id+1);if(!S.containers.emplace(I.id,std::move(I)).second)throw std::runtime_error("duplicate inventory");
        }
        for(const auto& V:Data->Vendors){
            if(V.VendorId<=0 || V.StockInventoryId<=0 || V.CurrencyId.IsNone())throw std::runtime_error("positive vendor IDs and currency required");
            Vendor I;I.id=static_cast<Id>(V.VendorId);I.stock=static_cast<Id>(V.StockInventoryId);I.currency=Str(V.CurrencyId);
            I.sellMultiplierBasisPoints=V.SellMultiplierBasisPoints;I.buyMultiplierBasisPoints=V.BuyMultiplierBasisPoints;
            I.restockEverySeconds=V.RestockEverySeconds;I.nextRestockAt=V.FirstRestockAt;
            for(const auto& P:V.SellingPrices)I.sells.emplace(Str(P.Key),P.Value);for(const auto& P:V.BuyingPrices)I.buys.emplace(Str(P.Key),P.Value);
            for(FName N:V.RefusedTags)I.refusedTags.insert(Str(N));for(const auto& P:V.RestockTargets)I.restockTargets.emplace(Str(P.Key),P.Value);
            for(const auto& P:V.RestockBatches)I.restockBatches.emplace(Str(P.Key),P.Value);
            S.nextId=std::max(S.nextId,I.id+1);if(!S.vendors.emplace(I.id,std::move(I)).second)throw std::runtime_error("duplicate vendor");
        }
        for(const auto& V:Data->LootSources){
            if(V.SourceId.IsNone() || V.LootTableId.IsNone())throw std::runtime_error("loot source IDs required");
            Source SourceValue;SourceValue.id=Str(V.SourceId);SourceValue.table=Str(V.LootTableId);SourceValue.seed=static_cast<std::uint64_t>(V.Seed);
            SourceValue.cooldownSeconds=V.CooldownSeconds;SourceValue.renewable=V.bRenewable;
            if(!S.sources.emplace(SourceValue.id,std::move(SourceValue)).second)throw std::runtime_error("duplicate loot source");
        }
        auto Economy=std::make_unique<Engine>(std::move(C),std::move(S));
        for(const auto& V:Data->InitialItems){
            Meta M;M.quality=V.Quality;M.durability=V.Durability;M.provenance=V.Provenance.IsNone()?"initial":Str(V.Provenance);
            auto P=Economy->state().receipts.find(Service);Command Cmd{P==Economy->state().receipts.end()?1:P->second.sequence+1,Economy->state().revision,Grant{static_cast<Id>(V.InventoryId),Str(V.DefinitionId),V.Quantity,M}};
            auto R=Economy->execute({Service,true,{},{}},Cmd);if(!R)throw std::runtime_error(std::string("initial item failed: ")+R.message);
        }
        Impl->CatalogAsset=Data;Impl->Economy=std::move(Economy);return true;
    }catch(const std::exception& E){Error=UTF8_TO_TCHAR(E.what());return false;}
}

int64 URBItemEconomySubsystem::GetRevision() const {return IsInGameThread() && Impl->Economy?static_cast<int64>(Impl->Economy->state().revision):0;}
int64 URBItemEconomySubsystem::GetNextSequence(FName ActorId) const {
    if(!IsInGameThread() || !Impl->Economy)return 0;auto P=Impl->Economy->state().receipts.find(Str(ActorId));
    return P==Impl->Economy->state().receipts.end()?1:static_cast<int64>(P->second.sequence+1);
}

bool URBItemEconomySubsystem::ReadInventorySummary(FName ActorId,int64 InventoryId,FRBInventoryView& ViewValue,FString& Error) const {
    Error.Reset();ViewValue=FRBInventoryView();if(!IsInGameThread()||!Impl->Economy){Error=TEXT("No authoritative economy");return false;}
    try {
        if(InventoryId<=0||!Impl->Economy->canAccess(Str(ActorId),static_cast<Id>(InventoryId))){Error=TEXT("Inventory access denied");return false;}
        auto It=Impl->Economy->state().containers.find(static_cast<Id>(InventoryId));if(It==Impl->Economy->state().containers.end()){Error=TEXT("Inventory not found");return false;}
        const auto& C=It->second;ViewValue.InventoryId=InventoryId;ViewValue.OwnerId=Name(C.owner);ViewValue.Slots=C.slots;ViewValue.UsedMassGrams=Impl->Economy->mass(C.id);
        ViewValue.MaxMassGrams=C.maxMassGrams;ViewValue.GridWidth=C.gridWidth;ViewValue.GridHeight=C.gridHeight;ViewValue.DecayRateBasisPoints=C.decayRateBasisPoints;
        for(const auto& P:C.money)ViewValue.Money.Add(Name(P.first),P.second);return true;
    }catch(const std::exception& E){Error=UTF8_TO_TCHAR(E.what());return false;}
}

TArray<FRBItemView> URBItemEconomySubsystem::ReadInventory(FName ActorId,int64 InventoryId,FString& Error) const {
    TArray<FRBItemView> Out;Error.Reset();if(!IsInGameThread() || !Impl->Economy){Error=TEXT("No authoritative economy");return Out;}
    try {
        if(InventoryId<=0 || !Impl->Economy->canAccess(Str(ActorId),static_cast<Id>(InventoryId))){Error=TEXT("Inventory access denied");return Out;}
        const State& S=Impl->Economy->state();
        for(const auto& Pair:S.items){
            const auto& I=Pair.second;if(I.container!=static_cast<Id>(InventoryId))continue;const auto& CoreDef=Impl->Economy->catalog().definitions.at(I.definition);
            FRBItemView V;V.ItemId=static_cast<int64>(I.id);V.InventoryId=InventoryId;V.DefinitionId=Name(I.definition);V.Quantity=I.quantity;
            V.Reserved=Impl->Economy->reserved(I.id);V.Quality=I.meta.quality;V.Durability=I.meta.durability;V.MassGrams=CoreDef.unitMassGrams*I.quantity;
            V.ExpiresAt=I.meta.expiresAt;V.RemainingShelfLife=I.meta.remainingShelfLife;V.ChildInventoryId=static_cast<int64>(I.childContainer);V.AttachmentInventoryId=static_cast<int64>(I.attachmentContainer);
            V.Slot=I.slot;V.X=I.x;V.Y=I.y;V.bRotated=I.rotated;V.bFavorite=I.favorite;V.bLocked=I.locked;V.bEquipped=IsEquipped(S,I.id);
            V.CategoryId=CoreDef.category.empty()?NAME_None:Name(CoreDef.category);V.Rarity=CoreDef.rarity;for(const auto& Tag:CoreDef.tags)V.Tags.Add(Name(Tag));
            const auto CtIt=S.containers.find(I.container);if(CtIt!=S.containers.end()&&CtIt->second.parentItem){const auto HostIt=S.items.find(CtIt->second.parentItem);if(HostIt!=S.items.end()&&HostIt->second.attachmentContainer==CtIt->second.id){V.ParentHostItemId=static_cast<int64>(HostIt->second.id);const auto& HostDef=Impl->Economy->catalog().definitions.at(HostIt->second.definition);if(I.slot>=0&&static_cast<size_t>(I.slot)<HostDef.attachmentSlots.size()){auto It=HostDef.attachmentSlots.begin();std::advance(It,I.slot);V.AttachmentSlotId=Name(It->first);}}}
            if(const FRBItemDefinition* Display=FindDefinition(Impl->CatalogAsset.Get(),I.definition)){V.DisplayName=Display->DisplayName;V.Description=Display->Description;V.Icon=Display->Icon;V.WorldMesh=Display->WorldMesh;V.EquippedStaticMesh=Display->EquippedStaticMesh;V.EquippedSkeletalMesh=Display->EquippedSkeletalMesh;V.EquipSocket=Display->EquipSocket;V.EquippedRelativeTransform=Display->EquippedRelativeTransform;V.bUseLeaderPose=Display->bUseLeaderPose;V.AttachmentSlots=Display->AttachmentSlots;}
            if(V.DisplayName.IsEmpty())V.DisplayName=FText::FromName(V.DefinitionId);Out.Add(MoveTemp(V));
        }
    }catch(const std::exception& E){Error=UTF8_TO_TCHAR(E.what());Out.Reset();}
    return Out;
}

bool URBItemEconomySubsystem::ReadItem(FName ActorId,int64 ItemId,FRBItemView& ViewValue,FString& Error) const {
    Error.Reset();ViewValue=FRBItemView();if(!IsInGameThread()||!Impl->Economy||ItemId<=0){Error=TEXT("No authoritative economy or invalid item");return false;}
    const auto It=Impl->Economy->state().items.find(static_cast<Id>(ItemId));if(It==Impl->Economy->state().items.end()){Error=TEXT("Item not found");return false;}
    TArray<FRBItemView> Views=ReadInventory(ActorId,static_cast<int64>(It->second.container),Error);if(!Error.IsEmpty())return false;
    if(const FRBItemView* Found=Views.FindByPredicate([&](const FRBItemView& V){return V.ItemId==ItemId;})){ViewValue=*Found;return true;}Error=TEXT("Item projection unavailable");return false;
}

TArray<FRBItemView> URBItemEconomySubsystem::ReadAttachments(FName ActorId,int64 HostItemId,FString& Error) const {
    Error.Reset();if(!IsInGameThread()||!Impl->Economy||HostItemId<=0){Error=TEXT("No authoritative economy or invalid host item");return {};}
    const auto It=Impl->Economy->state().items.find(static_cast<Id>(HostItemId));if(It==Impl->Economy->state().items.end()){Error=TEXT("Host item not found");return {};}
    if(!Impl->Economy->canAccess(Str(ActorId),It->second.container)){Error=TEXT("Host item access denied");return {};}
    if(!It->second.attachmentContainer)return {};return ReadInventory(ActorId,static_cast<int64>(It->second.attachmentContainer),Error);
}

TArray<FRBItemView> URBItemEconomySubsystem::QueryInventory(FName ActorId,int64 InventoryId,const FRBInventoryQuery& Query,FString& Error) const {
    TArray<FRBItemView> Items=ReadInventory(ActorId,InventoryId,Error);if(!Error.IsEmpty())return {};
    const FString Needle=Query.SearchText.TrimStartAndEnd().ToLower();
    Items.RemoveAll([&](const FRBItemView& V){
        if(Query.bFavoritesOnly&&!V.bFavorite)return true;if(!Query.CategoryId.IsNone()&&V.CategoryId!=Query.CategoryId)return true;
        if(!Query.bIncludeExpired&&V.RemainingShelfLife==0)return true;
        for(FName Required:Query.RequiredTags)if(!V.Tags.Contains(Required))return true;
        if(!Needle.IsEmpty()){
            const FString NameText=V.DisplayName.ToString().ToLower();const FString IdText=V.DefinitionId.ToString().ToLower();const FString Desc=V.Description.ToString().ToLower();
            if(!NameText.Contains(Needle)&&!IdText.Contains(Needle)&&!Desc.Contains(Needle))return true;
        }return false;
    });
    auto Less=[&](const FRBItemView& A,const FRBItemView& B){
        int32 Cmp=0;switch(Query.Sort){
        case ERBInventorySort::Name: Cmp=A.DisplayName.ToString().Compare(B.DisplayName.ToString(),ESearchCase::IgnoreCase);break;
        case ERBInventorySort::Category: Cmp=A.CategoryId.ToString().Compare(B.CategoryId.ToString(),ESearchCase::IgnoreCase);break;
        case ERBInventorySort::Quantity: Cmp=A.Quantity<B.Quantity?-1:(A.Quantity>B.Quantity?1:0);break;
        case ERBInventorySort::Quality: Cmp=A.Quality-B.Quality;break;
        case ERBInventorySort::Durability: Cmp=A.Durability-B.Durability;break;
        case ERBInventorySort::Mass: Cmp=A.MassGrams<B.MassGrams?-1:(A.MassGrams>B.MassGrams?1:0);break;
        case ERBInventorySort::Expiry: Cmp=A.ExpiresAt<B.ExpiresAt?-1:(A.ExpiresAt>B.ExpiresAt?1:0);break;
        case ERBInventorySort::Rarity: Cmp=A.Rarity-B.Rarity;break;
        default: Cmp=A.Slot-B.Slot;break;}
        if(Cmp==0)Cmp=A.ItemId<B.ItemId?-1:(A.ItemId>B.ItemId?1:0);return Query.bDescending?Cmp>0:Cmp<0;
    };
    Items.Sort(Less);return Items;
}

TArray<FRBRecipeView> URBItemEconomySubsystem::ReadRecipes(FName ActorId,FName StationTag) const {
    TArray<FRBRecipeView> Out;if(!IsInGameThread()||!Impl->Economy)return Out;const std::string Actor=Str(ActorId);const std::string RequiredStation=StationTag.IsNone()?std::string():Str(StationTag);
    const auto UnlockIt=Impl->Economy->state().unlocks.find(Actor);
    for(const auto& Pair:Impl->Economy->catalog().recipes){const auto& R=Pair.second;if(!RequiredStation.empty()&&R.stationTag!=RequiredStation)continue;FRBRecipeView V;V.RecipeId=Name(R.id);V.StationTag=Name(R.stationTag);V.DurationSeconds=R.durationSeconds;V.MinimumQuality=R.minQuality;V.bUnlocked=!R.requiresUnlock||(UnlockIt!=Impl->Economy->state().unlocks.end()&&UnlockIt->second.contains(R.id));
        for(const auto& P:R.inputs)V.Inputs.Add(Name(P.first),P.second);for(const auto& P:R.outputs)V.Outputs.Add(Name(P.first),P.second);for(const auto& P:R.fuelInputs)V.FuelInputs.Add(Name(P.first),P.second);for(const auto& Tool:R.requiredTools)V.RequiredTools.Add(Name(Tool));
        if(const URBItemEconomyCatalog* Data=Impl->CatalogAsset.Get())if(const FRBItemRecipe* Display=Data->Recipes.FindByPredicate([&](const FRBItemRecipe& D){return D.RecipeId==V.RecipeId;})){V.DisplayName=Display->DisplayName;V.CategoryId=Display->CategoryId;}if(V.DisplayName.IsEmpty())V.DisplayName=FText::FromName(V.RecipeId);Out.Add(MoveTemp(V));}
    return Out;
}

TArray<FRBCraftJobView> URBItemEconomySubsystem::ReadCraftJobs(FName ActorId,int64 StationId,FString& Error) const {
    TArray<FRBCraftJobView> Out;Error.Reset();if(!IsInGameThread()||!Impl->Economy){Error=TEXT("No authoritative economy");return Out;}
    const std::string Actor=Str(ActorId);for(const auto& P:Impl->Economy->state().jobs){const auto& J=P.second;if(J.owner!=Actor||(StationId>0&&J.station!=static_cast<Id>(StationId)))continue;
        FRBCraftJobView V;V.JobId=static_cast<int64>(J.id);V.RecipeId=Name(J.recipe);V.StationId=static_cast<int64>(J.station);V.DestinationId=static_cast<int64>(J.destination);V.Batches=J.batches;V.StartedAt=J.startedAt;V.DueAt=J.due;V.bReady=Impl->Economy->state().now>=J.due;Out.Add(V);}return Out;
}

TMap<int32,int64> URBItemEconomySubsystem::ReadHotbar(FName ActorId) const {
    TMap<int32,int64> Out;if(!IsInGameThread()||!Impl->Economy)return Out;auto It=Impl->Economy->state().hotbars.find(Str(ActorId));if(It==Impl->Economy->state().hotbars.end())return Out;
    for(const auto& P:It->second)Out.Add(P.first,static_cast<int64>(P.second));return Out;
}
TMap<FName,int64> URBItemEconomySubsystem::ReadEquipment(FName ActorId) const {
    TMap<FName,int64> Out;if(!IsInGameThread()||!Impl->Economy)return Out;auto It=Impl->Economy->state().equipment.find(Str(ActorId));if(It==Impl->Economy->state().equipment.end())return Out;
    for(const auto& P:It->second)Out.Add(Name(P.first),static_cast<int64>(P.second));return Out;
}
int64 URBItemEconomySubsystem::GetVendorStockInventoryId(int64 VendorId) const {
    if(!IsInGameThread()||!Impl->Economy||VendorId<=0)return 0;auto It=Impl->Economy->state().vendors.find(static_cast<Id>(VendorId));return It==Impl->Economy->state().vendors.end()?0:static_cast<int64>(It->second.stock);
}
TArray<FRBItemView> URBItemEconomySubsystem::ReadVendorStock(int64 VendorId,FString& Error) const {
    Error.Reset();if(!IsInGameThread()||!Impl->Economy){Error=TEXT("No authoritative economy");return {};};auto It=Impl->Economy->state().vendors.find(static_cast<Id>(VendorId));if(It==Impl->Economy->state().vendors.end()){Error=TEXT("Vendor not found");return {};}
    auto Ct=Impl->Economy->state().containers.find(It->second.stock);if(Ct==Impl->Economy->state().containers.end()){Error=TEXT("Vendor stock inventory missing");return {};}
    return ReadInventory(Name(Ct->second.owner),static_cast<int64>(Ct->second.id),Error);
}
FRBVendorQuoteView URBItemEconomySubsystem::QuoteVendor(int64 VendorId,int64 ItemId,int64 Quantity,bool bBuying) const {
    FRBVendorQuoteView V;if(!IsInGameThread()||!Impl->Economy){V.Code=TEXT("NotInitialized");return V;}auto Q=Impl->Economy->quote(static_cast<Id>(VendorId),static_cast<Id>(ItemId),Quantity,bBuying);
    V.bValid=Q.error==RB::Items::Error::None;V.Code=UTF8_TO_TCHAR(errorName(Q.error));V.Total=Q.total;V.Revision=static_cast<int64>(Q.revision);return V;
}

#define RB_APPLY_ACTOR(OP) if(!MayMutate() || !Impl->Economy || Sequence<=0 || ExpectedRevision<0 || ActorId.IsNone() || Str(ActorId)==Service)return Denied();return Impl->Apply(this,Context{Str(ActorId),false,{},{}},Command{static_cast<std::uint64_t>(Sequence),static_cast<std::uint64_t>(ExpectedRevision),(OP)})
FRBItemOperationResult URBItemEconomySubsystem::TransferItem(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 ItemId,int64 DestinationId,int64 Quantity,int32 Slot,int32 X,int32 Y,bool bRotated){RB_APPLY_ACTOR((Transfer{static_cast<Id>(ItemId),static_cast<Id>(DestinationId),Quantity,Slot,X,Y,bRotated}));}
FRBItemOperationResult URBItemEconomySubsystem::TransferAllItems(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 SourceId,int64 DestinationId){RB_APPLY_ACTOR((TransferAll{static_cast<Id>(SourceId),static_cast<Id>(DestinationId)}));}
FRBItemOperationResult URBItemEconomySubsystem::QuickStackItems(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 SourceId,int64 DestinationId){RB_APPLY_ACTOR((QuickStack{static_cast<Id>(SourceId),static_cast<Id>(DestinationId)}));}
FRBItemOperationResult URBItemEconomySubsystem::MergeItems(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 SourceId,int64 TargetId,int64 Quantity){RB_APPLY_ACTOR((RB::Items::Merge{static_cast<Id>(SourceId),static_cast<Id>(TargetId),Quantity}));}
FRBItemOperationResult URBItemEconomySubsystem::ConsumeOrDiscard(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 ItemId,int64 Quantity,bool bDiscard){Operation Op=bDiscard?Operation(Discard{static_cast<Id>(ItemId),Quantity}):Operation(Consume{static_cast<Id>(ItemId),Quantity});RB_APPLY_ACTOR(Op);}
FRBItemOperationResult URBItemEconomySubsystem::SetItemFlags(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 ItemId,bool bSetFavorite,bool bFavorite,bool bSetLocked,bool bLocked){RB_APPLY_ACTOR((RB::Items::SetItemFlags{static_cast<Id>(ItemId),bSetFavorite,bFavorite,bSetLocked,bLocked}));}
FRBItemOperationResult URBItemEconomySubsystem::BindHotbar(FName ActorId,int64 Sequence,int64 ExpectedRevision,int32 Slot,int64 ItemId){RB_APPLY_ACTOR((Hotbar{Slot,static_cast<Id>(ItemId)}));}
FRBItemOperationResult URBItemEconomySubsystem::SetEquipment(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 ItemId,bool bUnequip){RB_APPLY_ACTOR((Equip{static_cast<Id>(ItemId),bUnequip}));}
FRBItemOperationResult URBItemEconomySubsystem::RepairItem(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 ItemId,const TArray<int64>& Sources,int32 TargetDurability){Repair Op;Op.item=static_cast<Id>(ItemId);Op.targetDurability=TargetDurability;for(int64 I:Sources)Op.sources.push_back(static_cast<Id>(I));RB_APPLY_ACTOR(Op);}
FRBItemOperationResult URBItemEconomySubsystem::AttachItem(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 HostItemId,int64 AttachmentItemId,FName AttachmentSlotId){if(AttachmentSlotId.IsNone())return Denied();RB_APPLY_ACTOR((Attach{static_cast<Id>(HostItemId),static_cast<Id>(AttachmentItemId),Str(AttachmentSlotId)}));}
FRBItemOperationResult URBItemEconomySubsystem::DetachItem(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 AttachmentItemId,int64 DestinationId,int32 Slot,int32 X,int32 Y,bool bRotated){RB_APPLY_ACTOR((Detach{static_cast<Id>(AttachmentItemId),static_cast<Id>(DestinationId),Slot,X,Y,bRotated}));}
FRBItemOperationResult URBItemEconomySubsystem::BeginCraft(FName ActorId,int64 Sequence,int64 ExpectedRevision,FName Recipe,int64 StationId,int64 DestinationId,const TArray<int64>& Sources,int64 Batches){StartCraft Op{Str(Recipe),static_cast<Id>(StationId),static_cast<Id>(DestinationId),{},Batches};for(int64 I:Sources)Op.sources.push_back(static_cast<Id>(I));RB_APPLY_ACTOR(Op);}
FRBItemOperationResult URBItemEconomySubsystem::CompleteCraft(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 JobId,bool bCancel){Operation Op=bCancel?Operation(CancelCraft{static_cast<Id>(JobId)}):Operation(FinishCraft{static_cast<Id>(JobId)});RB_APPLY_ACTOR(Op);}
#undef RB_APPLY_ACTOR

FRBItemOperationResult URBItemEconomySubsystem::CommitVerifiedTrade(FName ActorId,int64 Sequence,int64 ExpectedRevision,int64 VendorId,int64 ItemId,int64 CustomerInventoryId,int64 Quantity,int64 QuotedTotal,bool bBuying){
    if(!MayMutate() || !Impl->Economy || Sequence<=0 || ExpectedRevision<0 || ActorId.IsNone() || Str(ActorId)==Service)return Denied();
    Context Ctx{Str(ActorId),false,{static_cast<Id>(VendorId)},{}};
    return Impl->Apply(this,Ctx,{static_cast<std::uint64_t>(Sequence),static_cast<std::uint64_t>(ExpectedRevision),Trade{static_cast<Id>(VendorId),static_cast<Id>(ItemId),static_cast<Id>(CustomerInventoryId),Quantity,QuotedTotal,bBuying}});
}
FRBItemOperationResult URBItemEconomySubsystem::RestockVendor(int64 VendorId,int64 ExpectedGeneration){
    if(!MayMutate()||!Impl->Economy||VendorId<=0||ExpectedGeneration<0)return Denied();return Impl->Apply(this,{Service,true,{},{}},Impl->Next(RB::Items::RestockVendor{static_cast<Id>(VendorId),static_cast<std::uint64_t>(ExpectedGeneration)}));
}
FRBItemOperationResult URBItemEconomySubsystem::CommitVerifiedLootRoll(FName SourceId,int64 ExpectedGeneration,int64 DestinationId){
    if(!MayMutate()||!Impl->Economy||SourceId.IsNone()||ExpectedGeneration<0||DestinationId<=0)return Denied();return Impl->Apply(this,{Service,true,{},{}},Impl->Next(RollLoot{Str(SourceId),static_cast<std::uint64_t>(ExpectedGeneration),static_cast<Id>(DestinationId)}));
}
FRBItemOperationResult URBItemEconomySubsystem::CommitProductionAward(FName EventId,int64 DestinationId,FName DefinitionId,int64 Quantity,int32 Quality){
    if(!MayMutate() || !Impl->Economy)return Denied();Meta M;M.quality=Quality;M.provenance=Str(EventId);
    return Impl->Apply(this,{Service,true,{},{}},Impl->Next(Award{Str(EventId),{{static_cast<Id>(DestinationId),Str(DefinitionId),Quantity,M}}}));
}
FRBItemOperationResult URBItemEconomySubsystem::UnlockRecipe(FName ActorId,FName RecipeId){
    if(!MayMutate()||!Impl->Economy||ActorId.IsNone()||RecipeId.IsNone())return Denied();return Impl->Apply(this,{Service,true,{},{}},Impl->Next(Unlock{Str(ActorId),Str(RecipeId)}));
}
FRBItemOperationResult URBItemEconomySubsystem::AdvanceCanonicalTime(int64 GameSeconds){
    if(!MayMutate() || !Impl->Economy)return Denied();return Impl->Apply(this,{Service,true,{},{}},Impl->Next(AdvanceTime{GameSeconds}));
}

bool URBItemEconomySubsystem::CaptureEconomy(TArray<uint8>& Bytes,FString& Error) const {
    Error.Reset();Bytes.Reset();if(!MayMutate() || !Impl->Economy){Error=TEXT("Server economy required");return false;}
    try{auto B=Impl->Economy->snapshot();Bytes.Append(B.data(),static_cast<int32>(B.size()));return true;}catch(const std::exception& E){Error=UTF8_TO_TCHAR(E.what());return false;}
}
bool URBItemEconomySubsystem::RestoreEconomy(const TArray<uint8>& Bytes,FString& Error){
    Error.Reset();if(!MayMutate() || !Impl->Economy){Error=TEXT("Server economy required");return false;}
    if(Bytes.Num()<4 || Bytes.Num()>static_cast<int32>(MaxSnapshotBytes)){Error=TEXT("Snapshot size invalid");return false;}
    std::vector<std::uint8_t> B(Bytes.GetData(),Bytes.GetData()+Bytes.Num());auto R=Impl->Economy->restore(B);
    if(!R){Error=UTF8_TO_TCHAR(R.message.c_str());return false;}TGuardValue<bool> Guard(Impl->bDispatching,true);OnCommitted.Broadcast(static_cast<int64>(R.revision));return true;
}
