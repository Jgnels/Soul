#include "SoulFounderPlaytestStateSubsystem.h"
#include "Dom/JsonObject.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
    bool Number(const FJsonObject& O, const TCHAR* K, int32& Out)
    {
        double N;
        if (!O.TryGetNumberField(K,N) || !FMath::IsFinite(N) || N<0 || N>MAX_int32 || N!=FMath::FloorToDouble(N)) return false;
        Out=static_cast<int32>(N); return true;
    }
    TSharedRef<FJsonObject> Numbers(const TMap<FName,int32>& M)
    {
        auto O=MakeShared<FJsonObject>(); TArray<FName> Keys; M.GetKeys(Keys); Keys.Sort(FNameLexicalLess());
        for (FName K:Keys) O->SetNumberField(K.ToString(),M[K]); return O;
    }
    bool ReadNumbers(const FJsonObject& O,const TCHAR* K,TMap<FName,int32>& Out)
    {
        const TSharedPtr<FJsonObject>* M=nullptr; if(!O.TryGetObjectField(K,M))return false;
        for(const auto& P:(*M)->Values)
        {
            int32 N; FName Name(*P.Key);
            if(Name.IsNone()||Out.Contains(Name)||!Number(**M,*P.Key,N))return false;
            Out.Add(Name,N);
        }
        return true;
    }
    TArray<TSharedPtr<FJsonValue>> Names(const TSet<FName>& Set)
    {
        TArray<FName> Sorted=Set.Array();Sorted.Sort(FNameLexicalLess());TArray<TSharedPtr<FJsonValue>> Out;
        for(FName Id:Sorted)Out.Add(MakeShared<FJsonValueString>(Id.ToString()));return Out;
    }
    bool ReadRegions(const FJsonObject& O,const TCHAR* K,const FSoulWorldState& World,TSet<FName>& Out)
    {
        const TArray<TSharedPtr<FJsonValue>>* A=nullptr;if(!O.TryGetArrayField(K,A))return false;
        for(const auto& V:*A){FString S;if(!V->TryGetString(S)||!World.Regions.Contains(FName(*S))||Out.Contains(FName(*S)))return false;Out.Add(FName(*S));}
        return true;
    }
}

bool USoulFounderPlaytestStateSubsystem::InitializeSixFactionState(const FJsonObject& Starts,const FJsonObject& Config,FString& Error)
{
    const TSharedPtr<FJsonObject>* Scenarios=nullptr; const TSharedPtr<FJsonObject>* Sandbox=nullptr;
    const TSharedPtr<FJsonObject>* Factions=nullptr;const TSharedPtr<FJsonObject>* Owners=nullptr;
    FString Scenario; int32 Count=0;
    if(!Config.TryGetStringField(TEXT("canonical_start_scenario"),Scenario)||Scenario!=TEXT("six_faction_sandbox_candidate")
        ||!Number(Config,TEXT("qualification_nonplayer_troops"),Count)||Count<=0
        ||!Starts.TryGetObjectField(TEXT("scenarios"),Scenarios)||!(*Scenarios)->TryGetObjectField(Scenario,Sandbox)
        ||!(*Sandbox)->TryGetObjectField(TEXT("factions"),Factions)||(*Factions)->Values.Num()!=6
        ||!(*Sandbox)->TryGetObjectField(TEXT("region_owners"),Owners)||(*Owners)->Values.Num()!=12||World.Regions.Num()!=36)
    {Error=TEXT("Six-faction qualification configuration invalid.");return false;}
    FSoulWorldState CandidateWorld=World;TMap<FName,FSoulFactionCampaignState> CandidateStates;
    for(auto& P:CandidateWorld.Regions)P.Value.OwnerFactionId=NAME_None;
    for(const auto& P:(*Owners)->Values)
    {
        FString Faction;auto* Region=CandidateWorld.Regions.Find(FName(*P.Key));
        if(!Region||!P.Value->TryGetString(Faction)||!FSoulCampaignRules::CanonicalFactions().Contains(FName(*Faction)))
        {Error=TEXT("Unknown initial region/owner.");return false;}
        Region->OwnerFactionId=FName(*Faction);
    }
    for(FName Id:FSoulCampaignRules::CanonicalFactions())
    {
        const TSharedPtr<FJsonObject>* F=nullptr; const TSharedPtr<FJsonObject>* Army=nullptr;FString Region;
        if(!(*Factions)->TryGetObjectField(Id.ToString(),F)||!(*F)->TryGetObjectField(TEXT("starting_army"),Army)
            ||!(*Army)->TryGetStringField(TEXT("region"),Region)||!CandidateWorld.Regions.Contains(FName(*Region))
            ||CandidateWorld.Regions.FindChecked(FName(*Region)).OwnerFactionId!=Id)
        {Error=TEXT("Starting army does not belong to its canonical faction.");return false;}
        if(Id==PlayerFaction){if(PlayerRegion!=FName(*Region)){Error=TEXT("Player start mismatch.");return false;}}
        else
        {
            FSoulFactionCampaignState Faction;
            Faction.Army={FName(*(Id.ToString()+TEXT(".primary"))),Id,FName(*Region),FSoulCampaignRules::AdmittedStrategicUnit(Id),Count};
            // Explicit test forces/resources, not approved balance or recruitment.
            Faction.Economy.Resources.Add(TEXT("gold"),Economy.Resources.FindRef(TEXT("gold")));
            CandidateStates.Add(Id,MoveTemp(Faction));
        }
        const TSharedPtr<FJsonObject>* Knowledge=nullptr;
        if(!(*F)->TryGetObjectField(TEXT("initial_knowledge"),Knowledge))return false;
        FSoulFactionKnowledge K;
        if(!ReadRegions(**Knowledge,TEXT("explored_regions"),CandidateWorld,K.ExploredRegions)
            ||!ReadRegions(**Knowledge,TEXT("visible_regions"),CandidateWorld,K.VisibleRegions))return false;
        CandidateWorld.KnowledgeByFaction.Add(Id,MoveTemp(K));
    }
    FString ControlledAttacker;
    if (FParse::Value(FCommandLine::Get(),TEXT("SoulControlledBattle="),ControlledAttacker))
    {
        if (ControlledAttacker!=TEXT("dwarves") && ControlledAttacker!=TEXT("orcs") && ControlledAttacker!=TEXT("vikings") && ControlledAttacker!=TEXT("nature") && ControlledAttacker!=TEXT("human_nature") && ControlledAttacker!=TEXT("dwarves_orcs") && ControlledAttacker!=TEXT("orcs_dwarves") && ControlledAttacker!=TEXT("dwarves_vikings") && ControlledAttacker!=TEXT("vikings_dwarves") && ControlledAttacker!=TEXT("orcs_vikings") && ControlledAttacker!=TEXT("vikings_orcs"))
        { Error=TEXT("Unknown controlled proof faction."); return false; }
        SixFactionSaveSlot=TEXT("Soul.Composition3500.Controlled.")+ControlledAttacker;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("SoulControlledTurn")))
    {
        if(!ControlledAttacker.IsEmpty()){Error=TEXT("Controlled proof modes conflict.");return false;}
        SixFactionSaveSlot=TEXT("Soul.Composition3500.ControlledTurn");
    }
    World=MoveTemp(CandidateWorld);OtherFactionStates=MoveTemp(CandidateStates);
    EnemyArmies.Reset(); // The legacy two-side ledger is inactive in this profile.
    bSixFactionProfile=true; LastAIReport=TEXT("Six canonical factions; strategic AI OFF; qualification forces, balance pending.");
    Error.Reset();return true;
}

bool USoulFounderPlaytestStateSubsystem::InspectFactionArmy(FName Id,FSoulFactionCampaignState& Out) const
{
    if(!bInitialized||!bSixFactionProfile||!FSoulCampaignRules::CanonicalFactions().Contains(Id))return false;
    if(Id==PlayerFaction)
    {
        Out.Army={TEXT("humans.primary"),PlayerFaction,PlayerRegion,PlayerUnitId,PlayerArmy.FindRef(PlayerUnitId)};
        Out.Economy=Economy;return true;
    }
    const auto* F=OtherFactionStates.Find(Id);if(!F)return false;Out=*F;return true;
}
int32 USoulFounderPlaytestStateSubsystem::ArmyCountAtRegion(FName Region) const
{
    if(!bSixFactionProfile)return EnemyArmies.FindRef(Region);
    if(Region==PlayerRegion && PlayerArmy.FindRef(PlayerUnitId)>0)return PlayerArmy.FindRef(PlayerUnitId);
    for(const auto& P:OtherFactionStates)if(P.Value.Army.RegionId==Region && P.Value.Army.TroopCount>0)return P.Value.Army.TroopCount;
    return 0;
}
FName USoulFounderPlaytestStateSubsystem::ArmyUnitAtRegion(FName Region) const
{
    if(!bSixFactionProfile)return EnemyUnitId;
    if(Region==PlayerRegion && PlayerArmy.FindRef(PlayerUnitId)>0)return PlayerUnitId;
    for(const auto& P:OtherFactionStates)if(P.Value.Army.RegionId==Region && P.Value.Army.TroopCount>0)return P.Value.Army.UnitId;
    return NAME_None;
}
FString USoulFounderPlaytestStateSubsystem::ArmyInspectionAtRegion(FName Region) const
{
    for(FName Id:FSoulCampaignRules::CanonicalFactions())
    {
        FSoulFactionCampaignState F;
        if(InspectFactionArmy(Id,F)&&F.Army.RegionId==Region)
            return FString::Printf(TEXT("%s: %d troops | %d AP | %s"),*Id.ToString(),F.Army.TroopCount,F.Economy.ActionPoints,
                F.Army.UnitId.IsNone()?TEXT("battle roster unavailable"):*F.Army.UnitId.ToString());
    }
    return TEXT("No field army stationed here.");
}
bool USoulFounderPlaytestStateSubsystem::MoveFactionArmy(FName Id,FName Target,FString& Error)
{
    // Explicit controlled action only. No scheduler/AI calls this method.
    if(!bSixFactionProfile||!bInitialized||HasPendingBattle()||bPersistenceBusy){Error=TEXT("Campaign is not ready.");return false;}
    if(Id==PlayerFaction){const bool Ok=MovePlayerTo(Target);Error=Ok?TEXT(""):TEXT("Player movement rejected.");return Ok;}
    auto* F=OtherFactionStates.Find(Id);const auto* Destination=World.Regions.Find(Target);
    if(!F||!Destination||F->Army.TroopCount<=0||!FSoulWorldRules::CanMove(World,F->Army.RegionId,Target)
        ||(!Destination->OwnerFactionId.IsNone()&&Destination->OwnerFactionId!=Id))
    {Error=TEXT("Movement requires a legal non-hostile canonical edge; non-Human battle admission is unavailable.");return false;}
    if(!FSoulCampaignRules::SpendAction(F->Economy)){Error=TEXT("No action points.");return false;}
    F->Army.RegionId=Target;FSoulWorldRules::Capture(World,Target,Id);FSoulWorldRules::RefreshVision(World,Id,Target);Error.Reset();return true;
}

void USoulFounderPlaytestStateSubsystem::CaptureSixFactionState(FJsonObject& Root) const
{
    Root.SetStringField(TEXT("profile"),GetCampaignSaveSlotName());
    Root.SetNumberField(TEXT("six_faction_version"),1);
    TArray<TSharedPtr<FJsonValue>> Regions,States;TArray<FName> Keys;World.Regions.GetKeys(Keys);Keys.Sort(FNameLexicalLess());
    for(FName Id:Keys)
    {
        auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("id"),Id.ToString());O->SetStringField(TEXT("owner"),World.Regions[Id].OwnerFactionId.ToString());Regions.Add(MakeShared<FJsonValueObject>(O));
    }
    Root.SetArrayField(TEXT("canonical_regions"),Regions);
    for(FName Id:FSoulCampaignRules::CanonicalFactions())
    {
        if(Id==PlayerFaction)continue;const auto& F=OtherFactionStates.FindChecked(Id);auto O=MakeShared<FJsonObject>();
        O->SetStringField(TEXT("faction"),Id.ToString());O->SetStringField(TEXT("army_id"),F.Army.ArmyId.ToString());
        O->SetStringField(TEXT("army_owner"),F.Army.FactionId.ToString());O->SetStringField(TEXT("region"),F.Army.RegionId.ToString());
        O->SetStringField(TEXT("unit"),F.Army.UnitId.ToString());O->SetNumberField(TEXT("troops"),F.Army.TroopCount);
        O->SetNumberField(TEXT("day"),F.Economy.Day);O->SetNumberField(TEXT("ap"),F.Economy.ActionPoints);O->SetNumberField(TEXT("max_ap"),F.Economy.MaxActionPoints);
        O->SetObjectField(TEXT("resources"),Numbers(F.Economy.Resources));O->SetObjectField(TEXT("income"),Numbers(F.Economy.DailyIncome));
        const auto& K=World.KnowledgeByFaction.FindChecked(Id);
        O->SetArrayField(TEXT("explored"),Names(K.ExploredRegions));O->SetArrayField(TEXT("visible"),Names(K.VisibleRegions));
        States.Add(MakeShared<FJsonValueObject>(O));
    }
    Root.SetArrayField(TEXT("other_faction_states"),States);
}

bool USoulFounderPlaytestStateSubsystem::ValidateSixFactionRestore(const FJsonObject& Root,
    TMap<FName,FSoulFactionCampaignState>& OutStates,FSoulWorldState& OutWorld,FString& Error) const
{
    Error=TEXT("Six-faction snapshot failed identity, region, army, or resource validation.");
    FString Profile;int32 Version=0;
    const TArray<TSharedPtr<FJsonValue>>* Regions=nullptr;const TArray<TSharedPtr<FJsonValue>>* States=nullptr;
    const TSharedPtr<FJsonObject>* Owners=nullptr;
    if(!Root.TryGetStringField(TEXT("profile"),Profile)||Profile!=GetCampaignSaveSlotName()||!Number(Root,TEXT("six_faction_version"),Version)||Version!=1
        ||!Root.TryGetArrayField(TEXT("canonical_regions"),Regions)||Regions->Num()!=World.Regions.Num()||Regions->Num()!=36
        ||!Root.TryGetArrayField(TEXT("other_faction_states"),States)||States->Num()!=5
        ||!Root.TryGetObjectField(TEXT("owners"),Owners)||(*Owners)->Values.Num()!=36)return false;
    OutWorld=World;TSet<FName> Seen;
    for(const auto& V:*Regions)
    {
        if(V->Type!=EJson::Object)return false;const auto O=V->AsObject();FString Id,Owner,LegacyOwner;
        if(!O->TryGetStringField(TEXT("id"),Id)||!O->TryGetStringField(TEXT("owner"),Owner))return false;
        FName R(*Id),F(*Owner);
        if(Seen.Contains(R)||!World.Regions.Contains(R)||(!F.IsNone()&&!FSoulCampaignRules::CanonicalFactions().Contains(F))
            ||!(*Owners)->TryGetStringField(Id,LegacyOwner)||LegacyOwner!=Owner)return false;
        Seen.Add(R);OutWorld.Regions.FindChecked(R).OwnerFactionId=F;
    }
    TSet<FName> Occupied;
    FString PlayerLocation;int32 PlayerCount=0;const TSharedPtr<FJsonObject>* PlayerUnits=nullptr;
    if(!Root.TryGetStringField(TEXT("player_region"),PlayerLocation)||!OutWorld.Regions.Contains(FName(*PlayerLocation))
        ||!Root.TryGetObjectField(TEXT("army"),PlayerUnits)||!Number(**PlayerUnits,TEXT("human_knight"),PlayerCount))return false;
    if(PlayerCount>0&&OutWorld.Regions.FindChecked(FName(*PlayerLocation)).OwnerFactionId!=PlayerFaction)return false;
    if(PlayerCount>0)Occupied.Add(FName(*PlayerLocation));
    int32 PlayerDay=0;if(!Number(Root,TEXT("day"),PlayerDay))return false;
    for(const auto& V:*States)
    {
        if(V->Type!=EJson::Object)return false;const auto O=V->AsObject();FString Id,ArmyId,Owner,Region,Unit;
        if(!O->TryGetStringField(TEXT("faction"),Id)||!O->TryGetStringField(TEXT("army_id"),ArmyId)
            ||!O->TryGetStringField(TEXT("army_owner"),Owner)||!O->TryGetStringField(TEXT("region"),Region)||!O->TryGetStringField(TEXT("unit"),Unit))return false;
        FName Faction(*Id),Location(*Region);
        if(Faction==PlayerFaction||!FSoulCampaignRules::CanonicalFactions().Contains(Faction)||OutStates.Contains(Faction)
            ||Owner!=Id||ArmyId!=Id+TEXT(".primary")||!OutWorld.Regions.Contains(Location)
            || (FName(*Unit)!=FSoulCampaignRules::AdmittedStrategicUnit(Faction)
                && !((Faction==TEXT("vikings")||Faction==TEXT("nature"))&&FName(*Unit).IsNone())))return false;
        // Earlier SixFactionProof saves intentionally had no Viking/Nature combat binding.
        // Preserve that None exactly; never silently upgrade a saved army roster.
        FSoulFactionCampaignState F;F.Army={FName(*ArmyId),Faction,Location,FName(*Unit),0};
        if(!Number(*O,TEXT("troops"),F.Army.TroopCount)||!Number(*O,TEXT("day"),F.Economy.Day)||F.Economy.Day!=PlayerDay
            ||!Number(*O,TEXT("ap"),F.Economy.ActionPoints)||!Number(*O,TEXT("max_ap"),F.Economy.MaxActionPoints)
            ||F.Economy.MaxActionPoints!=3||F.Economy.ActionPoints>F.Economy.MaxActionPoints
            ||!ReadNumbers(*O,TEXT("resources"),F.Economy.Resources)||!ReadNumbers(*O,TEXT("income"),F.Economy.DailyIncome)
            ||F.Economy.Resources.Num()!=1||!F.Economy.Resources.Contains(TEXT("gold"))||F.Economy.DailyIncome.Num()!=0)return false;
        if(F.Army.TroopCount>0)
        {
            if(OutWorld.Regions.FindChecked(Location).OwnerFactionId!=Faction||Occupied.Contains(Location))return false;
            Occupied.Add(Location);
        }
        FSoulFactionKnowledge K;
        if(!ReadRegions(*O,TEXT("explored"),OutWorld,K.ExploredRegions)||!ReadRegions(*O,TEXT("visible"),OutWorld,K.VisibleRegions))return false;
        OutWorld.KnowledgeByFaction.Add(Faction,MoveTemp(K));OutStates.Add(Faction,MoveTemp(F));
    }
    Error.Reset();return true;
}
