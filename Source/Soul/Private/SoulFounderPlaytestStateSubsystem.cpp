#include "SoulFounderPlaytestStateSubsystem.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RBSaveSubsystem.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Subsystems/SubsystemCollection.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoulCampaign, Log, All);
namespace
{
    bool ReadData(const TCHAR* File, TSharedPtr<FJsonObject>& Out)
    {
        FString Json;
        return FFileHelper::LoadFileToString(Json, *(FPaths::ProjectDir() / TEXT("Data") / File))
            && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Out) && Out.IsValid();
    }
    TSharedRef<FJsonObject> IntMap(const TMap<FName,int32>& Values)
    {
        auto Out = MakeShared<FJsonObject>();
        TArray<FName> Keys; Values.GetKeys(Keys); Keys.Sort(FNameLexicalLess());
        for (FName Key : Keys) Out->SetNumberField(Key.ToString(), Values[Key]);
        return Out;
    }
    bool ReadIntMap(const FJsonObject& Obj, const TCHAR* Key, TMap<FName,int32>& Out)
    {
        const TSharedPtr<FJsonObject>* Map;
        if (!Obj.TryGetObjectField(Key,Map)) return false;
        Out.Reset();
        for (const auto& Pair : (*Map)->Values)
        {
            double N;
            if (Pair.Key.IsEmpty() || !Pair.Value->TryGetNumber(N) || N<0 || N>MAX_int32 || N!=FMath::FloorToDouble(N)) return false;
            Out.Add(FName(*Pair.Key),static_cast<int32>(N));
        }
        return true;
    }
    TArray<TSharedPtr<FJsonValue>> NameSet(const TSet<FName>& Values)
    {
        TArray<FName> Keys=Values.Array(); Keys.Sort(FNameLexicalLess());
        TArray<TSharedPtr<FJsonValue>> Out;
        for (FName Key : Keys) Out.Add(MakeShared<FJsonValueString>(Key.ToString()));
        return Out;
    }
    bool ReadNameSet(const FJsonObject& Obj,const TCHAR* Key,TSet<FName>& Out)
    {
        const TArray<TSharedPtr<FJsonValue>>* Values;
        if (!Obj.TryGetArrayField(Key,Values)) return false;
        Out.Reset();
        for (const auto& V : *Values)
        {
            FString Name; if (!V->TryGetString(Name)||Name.IsEmpty()) return false;
            Out.Add(FName(*Name));
        }
        return true;
    }
    const TCHAR* CampaignSlot=TEXT("Soul.VerticalCampaign");
    // This checkpoint owns strategic campaign and settlement consequences. Optional
    // legacy item/weather/routine domains retain their separate registered authority.
    const TArray<FName> CampaignSaveDomains = {TEXT("Soul.Campaign"), TEXT("Soul.Settlements")};
}

void USoulFounderPlaytestStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<URBSaveSubsystem>();
    Collection.InitializeDependency<USoulCampaignBattleBridge>();
    SaveSubsystem=GetGameInstance()->GetSubsystem<URBSaveSubsystem>();
    BattleBridge=GetGameInstance()->GetSubsystem<USoulCampaignBattleBridge>();
    FString Error;
    if (SaveSubsystem.IsValid()&&!SaveSubsystem->RegisterDomainProvider(this,Error))
        UE_LOG(LogSoulCampaign,Error,TEXT("Campaign save registration failed: %s"),*Error);
    if (BattleBridge.IsValid()) BattleBridge->OnBattleResolved.AddUObject(this,&USoulFounderPlaytestStateSubsystem::HandleBattleResolved);
}
void USoulFounderPlaytestStateSubsystem::Deinitialize()
{
    if (BattleBridge.IsValid()) BattleBridge->OnBattleResolved.RemoveAll(this);
    if (SaveSubsystem.IsValid()) SaveSubsystem->UnregisterDomainProvider(this);
    Super::Deinitialize();
}
const TArray<FName>& USoulFounderPlaytestStateSubsystem::HumanPlaytestRoster()
{
    static const TArray<FName> Roster={TEXT("human_knight")}; return Roster;
}
void USoulFounderPlaytestStateSubsystem::InitializeScenario()
{
    if (bInitialized) return;
    TSharedPtr<FJsonObject> Config,Geography,Starts,Handoffs;
    if (!ReadData(TEXT("soul_vertical_scenario_20260925.json"),Config)
        ||!ReadData(TEXT("soul_world_overmap_v1_20260922.json"),Geography)
        ||!ReadData(TEXT("soul_campaign_start_states_v1_20260922.json"),Starts)
        ||!ReadData(TEXT("soul_overmap_battle_handoff_v1_20260922.json"),Handoffs))
    { UE_LOG(LogSoulCampaign,Error,TEXT("Campaign authority data unavailable; refusing fallback geography.")); return; }
    const auto Scenario=Starts->GetObjectField(TEXT("scenarios"))->GetObjectField(TEXT("founder_human_orc_micro"));
    TSet<FName> RegionIds;
    for (const auto& Id:Scenario->GetArrayField(TEXT("region_ids"))) RegionIds.Add(FName(*Id->AsString()));
    const auto Owners=Scenario->GetObjectField(TEXT("owners"));
    PlayerFaction=FName(*Config->GetStringField(TEXT("player_faction")));
    EnemyFaction=FName(*Config->GetStringField(TEXT("enemy_faction")));
    PlayerUnitId=FName(*Config->GetStringField(TEXT("player_unit_id")));
    EnemyUnitId=FName(*Config->GetStringField(TEXT("enemy_unit_id")));
    World=FSoulWorldState();
    for (const auto& Value:Geography->GetArrayField(TEXT("nodes")))
    {
        const auto Node=Value->AsObject(); FSoulRegionState Region;
        Region.Id=FName(*Node->GetStringField(TEXT("id"))); if (!RegionIds.Contains(Region.Id)) continue;
        Region.Biome=FName(*Node->GetStringField(TEXT("biome")));
        Region.Landform=FName(*Node->GetStringField(TEXT("landform")));
        Region.Feature=FName(*Node->GetStringField(TEXT("feature")));
        FString Owner,Settlement; Owners->TryGetStringField(Region.Id.ToString(),Owner);
        // Explicit qualification faction overlay preserves accepted stable geography IDs.
        Region.OwnerFactionId=Owner==TEXT("orcs")?EnemyFaction:FName(*Owner);
        Region.bSettlement=Node->TryGetStringField(TEXT("settlement_id"),Settlement)&&!Settlement.IsEmpty();
        RegionDisplayNames.Add(Region.Id,Node->GetStringField(TEXT("name")).Replace(TEXT("Orc"),TEXT("Dwarf")));
        World.Regions.Add(Region.Id,Region);
    }
    for (const auto& Value:Geography->GetArrayField(TEXT("edges")))
    {
        const auto Edge=Value->AsObject();
        const FName A(*Edge->GetStringField(TEXT("a"))),B(*Edge->GetStringField(TEXT("b")));
        auto* RA=World.Regions.Find(A); auto* RB=World.Regions.Find(B); if (!RA||!RB) continue;
        RA->Neighbors.AddUnique(B); RB->Neighbors.AddUnique(A);
        if (Edge->GetBoolField(TEXT("road"))) {RA->RoadNeighbors.Add(B);RB->RoadNeighbors.Add(A);}
    }
    for (const auto& Value:Handoffs->GetArrayField(TEXT("handoffs")))
    {
        const auto H=Value->AsObject();
        if (auto* R=World.Regions.Find(FName(*H->GetStringField(TEXT("destination_region")))))
            R->ApproachFromNeighbor.Add(FName(*H->GetStringField(TEXT("source_region"))),
                FName(*H->GetObjectField(TEXT("strategic_route"))->GetStringField(TEXT("entry_direction"))));
    }
    PlayerRegion=FName(*Scenario->GetStringField(TEXT("player_start_region")));
    EnemyRegion=FName(*Scenario->GetStringField(TEXT("enemy_primary_region")));
    BattleMap=FName(*Config->GetStringField(TEXT("battle_map")));
    CampaignMap=FName(*Config->GetStringField(TEXT("campaign_map")));
    const auto Origin=Config->GetArrayField(TEXT("arena_origin"));
    BattleOrigin=FVector(Origin[0]->AsNumber(),Origin[1]->AsNumber(),Origin[2]->AsNumber());
    ActiveCapPerSide=Config->GetIntegerField(TEXT("active_cap_per_side"));
    PlayerArmy.Reset(); PlayerArmy.Add(PlayerUnitId,Config->GetIntegerField(TEXT("player_strategic_count")));
    EnemyArmies.Reset();
    for (const auto& Pair:World.Regions)
        if (Pair.Value.OwnerFactionId==EnemyFaction) EnemyArmies.Add(Pair.Key,Config->GetIntegerField(TEXT("enemy_strategic_count")));
    Economy=FSoulCampaignEconomy(); Economy.Resources.Add(TEXT("gold"),3000); Economy.DailyIncome.Add(TEXT("gold"),450);
    FSoulRecruitmentPool Pool; Pool.UnitId=PlayerUnitId; Pool.Available=8; Pool.WeeklyGrowth=4;Pool.Capacity=24;
    Pool.CostPerUnit.Add(TEXT("gold"),140); Economy.RecruitmentPools.Add(Pool.UnitId,Pool);
    Hero=FSoulHeroState();Hero.HeroId=TEXT("human_founder_hero"); Hero.UnspentSkillPoints=1;Hero.MaxMana=80;Hero.Mana=80;
    Hero.KnownSpells.Add(TEXT("Magic.Spell.Fire.Firebolt"));
    LastAIReport=TEXT("Dwarf garrisons hold their regions; strategic reserves feed the active battle.");
    FSoulWorldRules::RefreshVision(World,PlayerFaction,PlayerRegion);bInitialized=true;
}
bool USoulFounderPlaytestStateSubsystem::IsHostile(FName RegionId) const
{
    const auto* R=World.Regions.Find(RegionId);
    return R&&!R->OwnerFactionId.IsNone()&&R->OwnerFactionId!=PlayerFaction;
}
bool USoulFounderPlaytestStateSubsystem::MovePlayerTo(FName Target)
{
    InitializeScenario();
    if (!bInitialized||HasPendingBattle()||bPersistenceBusy||IsHostile(Target)||!FSoulWorldRules::CanMove(World,PlayerRegion,Target)) return false;
    if (!FSoulCampaignRules::SpendAction(Economy,Hero.Skills.FindRef(TEXT("Adventure"))>=2?0:1)) return false;
    PlayerRegion=Target;FSoulWorldRules::RefreshVision(World,PlayerFaction,Target);
    if (World.Regions.FindChecked(Target).OwnerFactionId!=PlayerFaction)
    {
        FSoulWorldRules::Capture(World,Target,PlayerFaction);
        if (!RewardedRegions.Contains(Target))
        {RewardedRegions.Add(Target);Economy.Resources.FindOrAdd(TEXT("gold"))+=250;FSoulHeroRules::AddExperience(Hero,140);}
    }
    return true;
}
bool USoulFounderPlaytestStateSubsystem::BuildBattleDescriptor(FName Target,FSoulCampaignBattleDescriptor& Out,FString& Error) const
{
    if (!bInitialized||HasPendingBattle()||bPersistenceBusy||!IsHostile(Target)
        ||!FSoulWorldRules::CanMove(World,PlayerRegion,Target)||PlayerArmy.FindRef(PlayerUnitId)<=0||EnemyArmies.FindRef(Target)<=0)
    {Error=TEXT("Encounter requires adjacent hostile forces and a living player army.");return false;}
    Out=FSoulCampaignBattleDescriptor();Out.SourceRegion=PlayerRegion;Out.TargetRegion=Target;
    Out.PlayerFaction=PlayerFaction;Out.EnemyFaction=World.Regions.FindChecked(Target).OwnerFactionId;
    Out.PlayerUnitId=PlayerUnitId;Out.EnemyUnitId=EnemyUnitId;Out.MapPackage=BattleMap;Out.ReturnMapPackage=CampaignMap;
    Out.ArenaOrigin=BattleOrigin;Out.PlayerStrategicCount=PlayerArmy.FindRef(PlayerUnitId);Out.EnemyStrategicCount=EnemyArmies.FindRef(Target);
    Out.ActiveCapPerSide=ActiveCapPerSide;Out.EncounterOrdinal=EncounterOrdinal+1;
    Out.EncounterId=FName(*FString::Printf(TEXT("encounter.%d.%d.%s.%s"),Economy.Day,Out.EncounterOrdinal,*PlayerRegion.ToString(),*Target.ToString()));
    Error.Reset();return Out.IsValid();
}
bool USoulFounderPlaytestStateSubsystem::BeginBattle(FName Target,int32 Cap)
{
    FSoulCampaignBattleDescriptor D;FString Error;
    if (Economy.ActionPoints<1||!BuildBattleDescriptor(Target,D,Error)) return false;
    if (Cap>0) D.ActiveCapPerSide=FMath::Clamp(Cap,1,35);
    if (BattleBridge.IsValid()&&!BattleBridge->BeginEncounter(D)) return false;
    if (!FSoulCampaignRules::SpendAction(Economy,1)) return false;
    PendingBattle=D;EncounterOrdinal=D.EncounterOrdinal;
    UE_LOG(LogSoulCampaign,Display,TEXT("SOUL_CAMPAIGN_ENCOUNTER id=%s source=%s target=%s map=%s forces=%d/%d cap=%d"),
        *D.EncounterId.ToString(),*D.SourceRegion.ToString(),*D.TargetRegion.ToString(),*D.MapPackage.ToString(),D.PlayerStrategicCount,D.EnemyStrategicCount,D.ActiveCapPerSide);
    return true;
}
bool USoulFounderPlaytestStateSubsystem::ApplyBattleResult(const FSoulCampaignBattleResult& R)
{
    if (!HasPendingBattle()||ResolvedEncounters.Contains(R.EncounterId)||R.EncounterId!=PendingBattle.EncounterId||R.TargetRegion!=PendingBattle.TargetRegion
        ||R.PlayerSurvivors<0||R.PlayerSurvivors>PendingBattle.PlayerStrategicCount||R.EnemySurvivors<0||R.EnemySurvivors>PendingBattle.EnemyStrategicCount
        ||(R.bPlayerWon&&(R.EnemySurvivors!=0||R.PlayerSurvivors==0))||(!R.bPlayerWon&&R.PlayerSurvivors!=0)) return false;
    PlayerArmy.FindOrAdd(PendingBattle.PlayerUnitId)=R.PlayerSurvivors;EnemyArmies.FindOrAdd(PendingBattle.TargetRegion)=R.EnemySurvivors;bBattleWon=R.bPlayerWon;
    if (R.bPlayerWon)
    {
        FSoulWorldRules::Capture(World,PendingBattle.TargetRegion,PendingBattle.PlayerFaction);PlayerRegion=PendingBattle.TargetRegion;
        FSoulHeroRules::AddExperience(Hero,360);
        if (!RewardedRegions.Contains(PlayerRegion)) {RewardedRegions.Add(PlayerRegion);Economy.Resources.FindOrAdd(TEXT("gold"))+=600;}
    }
    else PlayerRegion=PendingBattle.SourceRegion;
    FSoulWorldRules::RefreshVision(World,PlayerFaction,PlayerRegion);
    LastBattleResult=R;ResolvedEncounters.Add(R.EncounterId);PendingBattle=FSoulCampaignBattleDescriptor();
    UE_LOG(LogSoulCampaign,Display,TEXT("SOUL_CAMPAIGN_RESULT id=%s target=%s victory=%d survivors=%d/%d player_region=%s"),
        *R.EncounterId.ToString(),*R.TargetRegion.ToString(),R.bPlayerWon,R.PlayerSurvivors,R.EnemySurvivors,*PlayerRegion.ToString());
    return true;
}
void USoulFounderPlaytestStateSubsystem::HandleBattleResolved(const FSoulCampaignBattleResult& R)
{if(ApplyBattleResult(R)) SaveCampaign();else UE_LOG(LogSoulCampaign,Error,TEXT("Rejected campaign result"));}
void USoulFounderPlaytestStateSubsystem::AdvanceDay()
{InitializeScenario();if(HasPendingBattle()||bPersistenceBusy)return;FSoulCampaignRules::AdvanceDay(Economy);Hero.Mana=FMath::Min(Hero.MaxMana,Hero.Mana+6);AdvanceEnemyAI();}
void USoulFounderPlaytestStateSubsystem::AdvanceEnemyAI()
{LastAIReport=TEXT("Dwarf garrisons hold; no synthetic strategic army bypasses encounter resolution.");}
bool USoulFounderPlaytestStateSubsystem::Recruit(FName Id)
{InitializeScenario();if(HasPendingBattle()||bPersistenceBusy||PlayerRegion!=TEXT("human_capital")||!FSoulCampaignRules::Recruit(Economy,Id,1))return false;PlayerArmy.FindOrAdd(Id)++;return true;}
bool USoulFounderPlaytestStateSubsystem::ChooseSkill(FName Id)
{
    if(HasPendingBattle()||bPersistenceBusy||!FSoulHeroRules::SpendSkillPoint(Hero,Id,2))return false;
    if(Id==TEXT("Adventure")){Economy.MaxActionPoints++;Economy.ActionPoints++;}
    else if(Id==TEXT("Magic")){Hero.MaxMana+=10;Hero.Mana+=10;}return true;
}
bool USoulFounderPlaytestStateSubsystem::HireTavernHero()
{
    if(HasPendingBattle()||bPersistenceBusy||PlayerRegion!=TEXT("human_capital")||bSecondHeroHired||Economy.Resources.FindRef(TEXT("gold"))<1200)return false;
    Economy.Resources.FindOrAdd(TEXT("gold"))-=1200;bSecondHeroHired=true;return true;
}
FString USoulFounderPlaytestStateSubsystem::BuildSummary() const
{return FString::Printf(TEXT("Day %d | AP %d/%d | Gold %d | Knights %d | Hero L%d XP %d"),Economy.Day,Economy.ActionPoints,Economy.MaxActionPoints,Economy.Resources.FindRef(TEXT("gold")),PlayerArmy.FindRef(PlayerUnitId),Hero.Level,Hero.Experience);}

FName USoulFounderPlaytestStateSubsystem::GetRBSaveDomainId_Implementation() const {return TEXT("Soul.Campaign");}
int32 USoulFounderPlaytestStateSubsystem::GetRBSaveSchemaVersion_Implementation() const {return 1;}
bool USoulFounderPlaytestStateSubsystem::CaptureRBSaveDomain_Implementation(FRBSaveDomainState& Out,FString& Error) const
{
    if(!bInitialized||HasPendingBattle()){Error=TEXT("Save requires settled campaign state.");return false;}
    auto Root=MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("player_region"),PlayerRegion.ToString());Root->SetStringField(TEXT("enemy_region"),EnemyRegion.ToString());
    Root->SetStringField(TEXT("player_faction"),PlayerFaction.ToString());Root->SetStringField(TEXT("enemy_faction"),EnemyFaction.ToString());
    Root->SetNumberField(TEXT("day"),Economy.Day);Root->SetNumberField(TEXT("ap"),Economy.ActionPoints);Root->SetNumberField(TEXT("max_ap"),Economy.MaxActionPoints);
    Root->SetNumberField(TEXT("ordinal"),EncounterOrdinal);Root->SetBoolField(TEXT("won"),bBattleWon);Root->SetBoolField(TEXT("hired"),bSecondHeroHired);
    Root->SetObjectField(TEXT("army"),IntMap(PlayerArmy));Root->SetObjectField(TEXT("enemies"),IntMap(EnemyArmies));
    Root->SetObjectField(TEXT("resources"),IntMap(Economy.Resources));Root->SetObjectField(TEXT("income"),IntMap(Economy.DailyIncome));
    TMap<FName,int32> Pools;for(const auto& P:Economy.RecruitmentPools)Pools.Add(P.Key,P.Value.Available);
    Root->SetObjectField(TEXT("pools"),IntMap(Pools));
    auto Owners=MakeShared<FJsonObject>();for(const auto& P:World.Regions)Owners->SetStringField(P.Key.ToString(),P.Value.OwnerFactionId.ToString());
    Root->SetObjectField(TEXT("owners"),Owners);Root->SetArrayField(TEXT("rewarded"),NameSet(RewardedRegions));Root->SetArrayField(TEXT("resolved"),NameSet(ResolvedEncounters));
    Root->SetArrayField(TEXT("explored"),NameSet(World.KnowledgeByFaction.FindChecked(PlayerFaction).ExploredRegions));
    TMap<FName,int32> HeroNumbers={{TEXT("level"),Hero.Level},{TEXT("xp"),Hero.Experience},{TEXT("points"),Hero.UnspentSkillPoints},{TEXT("mana"),Hero.Mana},{TEXT("max_mana"),Hero.MaxMana}};
    Root->SetObjectField(TEXT("hero"),IntMap(HeroNumbers));Root->SetObjectField(TEXT("skills"),IntMap(Hero.Skills));Root->SetArrayField(TEXT("spells"),NameSet(Hero.KnownSpells));
    Root->SetStringField(TEXT("result_id"),LastBattleResult.EncounterId.ToString());Root->SetStringField(TEXT("result_target"),LastBattleResult.TargetRegion.ToString());
    Root->SetBoolField(TEXT("result_won"),LastBattleResult.bPlayerWon);
    TMap<FName,int32> Result={{TEXT("player"),LastBattleResult.PlayerSurvivors},{TEXT("enemy"),LastBattleResult.EnemySurvivors},{TEXT("player_waves"),LastBattleResult.PlayerReinforcements},{TEXT("enemy_waves"),LastBattleResult.EnemyReinforcements},{TEXT("magic"),LastBattleResult.MagicCasts}};
    Root->SetObjectField(TEXT("result"),IntMap(Result));
    FString Json;if(!FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Json)))return false;
    Out=FRBSaveDomainState();Out.DomainId=GetRBSaveDomainId_Implementation();Out.SchemaVersion=1;
    FRBSaveField Field;Field.Name=TEXT("CampaignJson");Field.Type=ERBSaveFieldType::String;Field.StringValue=Json;Out.Fields.Add(Field);Error.Reset();return true;
}
bool USoulFounderPlaytestStateSubsystem::RestoreRBSaveDomain_Implementation(const FRBSaveDomainState& In,FString& Error)
{
    if(HasPendingBattle()||In.DomainId!=GetRBSaveDomainId_Implementation()||In.SchemaVersion!=1){Error=TEXT("Domain/schema mismatch or pending battle.");return false;}
    const auto* Field=In.Fields.FindByPredicate([](const FRBSaveField& F){return F.Name==TEXT("CampaignJson")&&F.Type==ERBSaveFieldType::String;});
    TSharedPtr<FJsonObject> Root;
    if(!Field||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Field->StringValue),Root)||!Root.IsValid()){Error=TEXT("Invalid campaign JSON.");return false;}
    InitializeScenario();if(!bInitialized)return false;
    // Restore into temporary values: invalid snapshots must not partially mutate authority.
    TMap<FName,int32> Army,Enemies,Resources,Income,Pools,Numbers,Skills,Result;
    TSet<FName> Rewarded,Resolved,Explored,Spells;
    FString Region,Enemy,PF,EF,ResultId,ResultTarget;
    int32 Day=0,AP=0,MaxAP=0,Ordinal=0;bool Won=false,Hired=false,ResultWon=false;
    const TSharedPtr<FJsonObject>* Owners=nullptr;
    bool Valid=Root->TryGetStringField(TEXT("player_region"),Region)&&Root->TryGetStringField(TEXT("enemy_region"),Enemy)
        &&World.Regions.Contains(FName(*Region))&&World.Regions.Contains(FName(*Enemy))
        &&Root->TryGetStringField(TEXT("player_faction"),PF)&&FName(*PF)==PlayerFaction
        &&Root->TryGetStringField(TEXT("enemy_faction"),EF)&&FName(*EF)==EnemyFaction
        &&Root->TryGetNumberField(TEXT("day"),Day)&&Day>0&&Root->TryGetNumberField(TEXT("ap"),AP)&&AP>=0
        &&Root->TryGetNumberField(TEXT("max_ap"),MaxAP)&&MaxAP>0&&AP<=MaxAP+1
        &&Root->TryGetNumberField(TEXT("ordinal"),Ordinal)&&Ordinal>=0
        &&Root->TryGetBoolField(TEXT("won"),Won)&&Root->TryGetBoolField(TEXT("hired"),Hired)
        &&ReadIntMap(*Root,TEXT("army"),Army)&&Army.Num()==1&&Army.Contains(PlayerUnitId)
        &&ReadIntMap(*Root,TEXT("enemies"),Enemies)&&ReadIntMap(*Root,TEXT("resources"),Resources)
        &&ReadIntMap(*Root,TEXT("income"),Income)&&ReadIntMap(*Root,TEXT("pools"),Pools)
        &&ReadIntMap(*Root,TEXT("hero"),Numbers)&&ReadIntMap(*Root,TEXT("skills"),Skills)
        &&ReadNameSet(*Root,TEXT("rewarded"),Rewarded)&&ReadNameSet(*Root,TEXT("resolved"),Resolved)
        &&ReadNameSet(*Root,TEXT("explored"),Explored)&&ReadNameSet(*Root,TEXT("spells"),Spells)
        &&Root->TryGetObjectField(TEXT("owners"),Owners)&&(*Owners)->Values.Num()==World.Regions.Num()
        &&Root->TryGetStringField(TEXT("result_id"),ResultId)&&Root->TryGetStringField(TEXT("result_target"),ResultTarget)
        &&Root->TryGetBoolField(TEXT("result_won"),ResultWon)&&ReadIntMap(*Root,TEXT("result"),Result);
    if(Valid)
    {
        Valid=Numbers.Num()==5&&Numbers.Contains(TEXT("level"))&&Numbers.Contains(TEXT("xp"))&&Numbers.Contains(TEXT("points"))&&Numbers.Contains(TEXT("mana"))&&Numbers.Contains(TEXT("max_mana"))
            &&Numbers.FindRef(TEXT("level"))>0&&Numbers.FindRef(TEXT("mana"))<=Numbers.FindRef(TEXT("max_mana"))
            &&Result.Num()==5&&Result.Contains(TEXT("player"))&&Result.Contains(TEXT("enemy"))&&Result.Contains(TEXT("player_waves"))&&Result.Contains(TEXT("enemy_waves"))&&Result.Contains(TEXT("magic"));
        for(const auto& P:Enemies)Valid&=World.Regions.Contains(P.Key);
        for(const auto& P:Pools)Valid&=Economy.RecruitmentPools.Contains(P.Key);
        for(FName R:Rewarded)Valid&=World.Regions.Contains(R);for(FName R:Explored)Valid&=World.Regions.Contains(R);
        for(const auto& P:World.Regions)
        {
            FString Owner;Valid&=(*Owners)->TryGetStringField(P.Key.ToString(),Owner);
            Valid&=Owner==TEXT("None")||FName(*Owner)==PlayerFaction||FName(*Owner)==EnemyFaction;
        }
    }
    if(!Valid){Error=TEXT("Campaign snapshot failed validation.");return false;}
    for(auto& P:World.Regions)P.Value.OwnerFactionId=FName(*(*Owners)->GetStringField(P.Key.ToString()));
    Economy.Day=Day;Economy.ActionPoints=AP;Economy.MaxActionPoints=MaxAP;Economy.Resources=MoveTemp(Resources);Economy.DailyIncome=MoveTemp(Income);
    for(const auto& P:Pools)Economy.RecruitmentPools[P.Key].Available=P.Value;
    Hero.Level=Numbers[TEXT("level")];Hero.Experience=Numbers[TEXT("xp")];Hero.UnspentSkillPoints=Numbers[TEXT("points")];Hero.Mana=Numbers[TEXT("mana")];Hero.MaxMana=Numbers[TEXT("max_mana")];Hero.Skills=MoveTemp(Skills);Hero.KnownSpells=MoveTemp(Spells);
    PlayerArmy=MoveTemp(Army);EnemyArmies=MoveTemp(Enemies);PlayerRegion=FName(*Region);EnemyRegion=FName(*Enemy);
    EncounterOrdinal=Ordinal;bBattleWon=Won;bSecondHeroHired=Hired;RewardedRegions=MoveTemp(Rewarded);ResolvedEncounters=MoveTemp(Resolved);
    LastBattleResult.EncounterId=FName(*ResultId);LastBattleResult.TargetRegion=FName(*ResultTarget);LastBattleResult.bPlayerWon=ResultWon;
    LastBattleResult.PlayerSurvivors=Result[TEXT("player")];LastBattleResult.EnemySurvivors=Result[TEXT("enemy")];
    LastBattleResult.PlayerReinforcements=Result[TEXT("player_waves")];LastBattleResult.EnemyReinforcements=Result[TEXT("enemy_waves")];LastBattleResult.MagicCasts=Result[TEXT("magic")];
    World.KnowledgeByFaction.FindOrAdd(PlayerFaction).ExploredRegions=MoveTemp(Explored);FSoulWorldRules::RefreshVision(World,PlayerFaction,PlayerRegion);
    Error.Reset();return true;
}
void USoulFounderPlaytestStateSubsystem::SaveCampaign()
{
    if(!SaveSubsystem.IsValid()||bPersistenceBusy||HasPendingBattle())return;
    bPersistenceBusy=true;bLastSaveSucceeded=false;
    FRBSaveOperationDelegate Done;Done.BindDynamic(this,&USoulFounderPlaytestStateSubsystem::OnCampaignSaved);
    UE_LOG(LogSoulCampaign,Display,TEXT("SOUL_CAMPAIGN_SAVE_SCOPE domains=Soul.Campaign,Soul.Settlements"));
    SaveSubsystem->SaveSelectedDomainsAsync(CampaignSlot,CampaignSaveDomains,Done);
}
void USoulFounderPlaytestStateSubsystem::LoadCampaign()
{
    if(!SaveSubsystem.IsValid()||bPersistenceBusy||HasPendingBattle())return;
    bPersistenceBusy=true;bLastLoadSucceeded=false;
    FRBSaveOperationDelegate Done;Done.BindDynamic(this,&USoulFounderPlaytestStateSubsystem::OnCampaignLoaded);
    SaveSubsystem->LoadSelectedDomainsAsync(CampaignSlot,CampaignSaveDomains,Done);
}
void USoulFounderPlaytestStateSubsystem::OnCampaignSaved(const FRBSaveOperationResult& R)
{
    bPersistenceBusy=false;bLastSaveSucceeded=R.bSuccess;LastPersistenceReport=R.bSuccess?TEXT("Campaign saved with RB Save."):R.Message;
    const FString SavedPath = SaveSubsystem.IsValid()
        ? SaveSubsystem->GetDomainSlotPath(CampaignSlot) : R.ArtifactPath;
    UE_LOG(LogSoulCampaign,Display,TEXT("SOUL_CAMPAIGN_SAVE success=%d path=%s message=%s"),R.bSuccess,*SavedPath,*R.Message);
}
void USoulFounderPlaytestStateSubsystem::OnCampaignLoaded(const FRBSaveOperationResult& R)
{
    bPersistenceBusy=false;bLastLoadSucceeded=R.bSuccess;LastPersistenceReport=R.bSuccess?TEXT("Campaign reloaded with RB Save."):R.Message;
    UE_LOG(LogSoulCampaign,Display,TEXT("SOUL_CAMPAIGN_LOAD success=%d region=%s knights=%d xp=%d"),R.bSuccess,*PlayerRegion.ToString(),PlayerArmy.FindRef(PlayerUnitId),Hero.Experience);
}
