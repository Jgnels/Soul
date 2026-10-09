#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulSettlementScenarioData.h"
#include "SoulSettlementStateSubsystem.h"
#include "SoulTown.h"
#include "SoulCampaignTerrain.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
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
            const FName Name(*Pair.Key);
            if (Name.IsNone() || Out.Contains(Name) || !Pair.Value->TryGetNumber(N) || !FMath::IsFinite(N)
                || N<0 || N>MAX_int32 || N!=FMath::FloorToDouble(N)) return false;
            Out.Add(Name,static_cast<int32>(N));
        }
        return true;
    }
    bool ReadNonNegativeInt(const FJsonObject& Obj, const TCHAR* Key, int32& Out)
    {
        double Number;
        if (!Obj.TryGetNumberField(Key, Number) || !FMath::IsFinite(Number)
            || Number < 0 || Number > MAX_int32 || Number != FMath::FloorToDouble(Number)) return false;
        Out = static_cast<int32>(Number);
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
    // This checkpoint owns strategic campaign and settlement consequences. Optional
    // legacy item/weather/routine domains retain their separate registered authority.
    const TArray<FName> CampaignSaveDomains = {TEXT("Soul.Campaign"), TEXT("Soul.Settlements")};
}

void USoulFounderPlaytestStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<URBSaveSubsystem>();
    Collection.InitializeDependency<USoulCampaignBattleBridge>();
    Collection.InitializeDependency<USoulSettlementStateSubsystem>();
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
    const bool bDwarfEnvironmentProof = FParse::Param(FCommandLine::Get(), TEXT("SoulDwarfSettlementProof"));
    const bool bHumanEnvironmentProof = FParse::Param(FCommandLine::Get(), TEXT("SoulHumanSettlementProof"));
    const bool bAuthoredEnvironmentProof = bDwarfEnvironmentProof || bHumanEnvironmentProof;
    const bool bComposition = SoulCampaignTerrain::Composition();
    const bool bVikingMatchupProof = FParse::Param(FCommandLine::Get(),TEXT("SoulVikingMatchupProof"));
    if(bVikingMatchupProof&&(!bComposition||bAuthoredEnvironmentProof||FParse::Param(FCommandLine::Get(),TEXT("SoulOrcMatchupProof"))))
    {UE_LOG(LogSoulCampaign,Error,TEXT("SOUL_VIKING_PROOF_FAIL requires isolated Composition fixture"));return;}
    const bool bAlphaRequested=FParse::Param(FCommandLine::Get(),TEXT("SoulFourFactionAlpha"));
    const bool bSixFactionRequested = bAlphaRequested || FParse::Param(FCommandLine::Get(),TEXT("SoulSixFactionProof"));
    FString ConflictingControlled;
    if(bAlphaRequested && (FParse::Value(FCommandLine::Get(),TEXT("SoulControlledBattle="),ConflictingControlled)
        || FParse::Param(FCommandLine::Get(),TEXT("SoulControlledTurn"))
        || (FParse::Param(FCommandLine::Get(),TEXT("SoulAlphaDefenseProof")) && FParse::Param(FCommandLine::Get(),TEXT("SoulAlphaAttackProof")))))
    {UE_LOG(LogSoulCampaign,Error,TEXT("Alpha cannot share a controlled qualification slot."));return;}
    if(bSixFactionRequested&&(!bComposition||bAuthoredEnvironmentProof||bVikingMatchupProof||FParse::Param(FCommandLine::Get(),TEXT("SoulOrcMatchupProof"))))
    {UE_LOG(LogSoulCampaign,Error,TEXT("SOUL_SIX_FACTION_FAIL requires isolated Composition profile"));return;}
    const bool bOrcMatchupProof = FParse::Param(FCommandLine::Get(),TEXT("SoulOrcMatchupProof"));
    if(bOrcMatchupProof&&(!bComposition||bAuthoredEnvironmentProof))
    {UE_LOG(LogSoulCampaign,Error,TEXT("SOUL_ORC_PROOF_FAIL requires isolated Composition fixture without authored-city proof"));return;}
    if(bComposition&&(bDwarfEnvironmentProof||FParse::Param(FCommandLine::Get(),TEXT("SoulCampaignExpansion"))||FParse::Param(FCommandLine::Get(),TEXT("SoulWorldTerrain"))))
    {UE_LOG(LogSoulCampaign,Error,TEXT("SOUL_COMPOSITION_SCENARIO_FAIL conflicting experimental flag"));return;}
    if ((bAuthoredEnvironmentProof && !SoulCampaignTerrain::EvilCorridor() && !bComposition)
        || (bDwarfEnvironmentProof && bHumanEnvironmentProof))
    { UE_LOG(LogSoulCampaign, Error, TEXT("SOUL_AUTHORED_PROOF_FAIL requires one explicit settlement and the retained EvilCorridor terrain profile")); return; }
    TSharedPtr<FJsonObject> Config,Geography,Starts,Handoffs;
    const bool bConfigLoaded = bComposition ? ReadData(bSixFactionRequested?TEXT("CampaignComposition/SixFactionRuntimeProof.json"):bVikingMatchupProof?TEXT("CampaignComposition/VikingRuntimeProof.json"):bOrcMatchupProof?TEXT("CampaignComposition/OrcRuntimeProof.json"):bHumanEnvironmentProof?TEXT("CampaignComposition/HumanRuntimeProof.json"):TEXT("CampaignComposition/RuntimeProof.json"),Config) : bDwarfEnvironmentProof
        ? ReadData(TEXT("SettlementEnvironments/DwarfHoldRuntimeProof.json"), Config)
        : bHumanEnvironmentProof ? ReadData(TEXT("SettlementEnvironments/HumanCapitalRuntimeProof.json"), Config)
        : ReadData(TEXT("soul_vertical_scenario_20260925.json"), Config);
    if (!bConfigLoaded
        ||!ReadData(TEXT("soul_world_overmap_v1_20260922.json"),Geography)
        ||!ReadData(TEXT("soul_campaign_start_states_v1_20260922.json"),Starts)
        ||!ReadData(TEXT("soul_overmap_battle_handoff_v1_20260922.json"),Handoffs))
    { UE_LOG(LogSoulCampaign,Error,TEXT("Campaign authority data unavailable; refusing fallback geography.")); return; }
    // This opt-in content qualification uses existing Crownspine nodes/edges and
    // a captured-hold starting owner. It does not alter the canonical world graph.
    const auto Scenario=(bAuthoredEnvironmentProof||bComposition) ? Config->GetObjectField(TEXT("start_state"))
        : Starts->GetObjectField(TEXT("scenarios"))->GetObjectField(TEXT("founder_human_orc_micro"));
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
        Region.OwnerFactionId=!bSixFactionRequested&&Owner==TEXT("orcs")?EnemyFaction:FName(*Owner);
        Region.bSettlement=Node->TryGetStringField(TEXT("settlement_id"),Settlement)&&!Settlement.IsEmpty();
        const FString DisplayName=Node->GetStringField(TEXT("name"));
        RegionDisplayNames.Add(Region.Id,!bSixFactionRequested&&EnemyFaction==TEXT("dwarves")?DisplayName.Replace(TEXT("Orc"),TEXT("Dwarf")):DisplayName);
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
    FSoulBattlefieldTemplate DefaultBattlefield;
    DefaultBattlefield.Id = TEXT("dragon_graveyard");
    if (bDwarfEnvironmentProof) DefaultBattlefield.Id = TEXT("dwarf_hold_approach");
    if (bHumanEnvironmentProof) DefaultBattlefield.Id = TEXT("human_capital_approach");
    DefaultBattlefield.MapPackage = BattleMap;
    DefaultBattlefield.bPlayable = true;
    DefaultBattlefield.ArenaOrigin = BattleOrigin;
    // A zero-match candidate must not displace the qualified fallback merely
    // because its stable id sorts earlier. Any real geography match wins.
    DefaultBattlefield.BaseScore = 1;
    BattlefieldTemplates = {DefaultBattlefield};
    // Only explicitly enabled candidates participate. Unqualified/asset-incomplete
    // environments can be recorded in data without being loaded or selected.
    if (Config->HasField(TEXT("battlefield_candidates")))
    {
        const TArray<TSharedPtr<FJsonValue>>* Candidates = nullptr;
        if (!Config->TryGetArrayField(TEXT("battlefield_candidates"), Candidates))
        { UE_LOG(LogSoulCampaign, Error, TEXT("Battlefield candidates must be an array.")); return; }
        TSet<FName> Seen = {DefaultBattlefield.Id};
        for (const auto& Value : *Candidates)
        {
            if (!Value.IsValid() || Value->Type != EJson::Object)
            { UE_LOG(LogSoulCampaign, Error, TEXT("Battlefield candidate must be an object.")); return; }
            const auto Candidate = Value->AsObject();
            bool Enabled = false;
            if (!Candidate.IsValid() || !Candidate->TryGetBoolField(TEXT("enabled"), Enabled))
            { UE_LOG(LogSoulCampaign, Error, TEXT("Battlefield candidate requires an explicit enabled flag.")); return; }
            if (!Enabled) continue;
            FString Id, Map;
            const TArray<TSharedPtr<FJsonValue>>* Coordinates = nullptr;
            FSoulBattlefieldTemplate Template;
            if (!Candidate->TryGetStringField(TEXT("id"), Id) || FName(*Id).IsNone() || Seen.Contains(FName(*Id))
                || !Candidate->TryGetStringField(TEXT("map"), Map) || !Map.StartsWith(TEXT("/Game/"))
                || !ReadNameSet(*Candidate, TEXT("biomes"), Template.Biomes)
                || !ReadNameSet(*Candidate, TEXT("landforms"), Template.Landforms)
                || !ReadNameSet(*Candidate, TEXT("features"), Template.Features)
                || !Candidate->TryGetArrayField(TEXT("arena_origin"), Coordinates) || Coordinates->Num() != 3)
            {
                UE_LOG(LogSoulCampaign, Error, TEXT("Invalid enabled battlefield candidate: %s"), *Id);
                return;
            }
            double X, Y, Z;
            if (!(*Coordinates)[0]->TryGetNumber(X) || !(*Coordinates)[1]->TryGetNumber(Y)
                || !(*Coordinates)[2]->TryGetNumber(Z) || !FMath::IsFinite(X) || !FMath::IsFinite(Y) || !FMath::IsFinite(Z))
            { UE_LOG(LogSoulCampaign, Error, TEXT("Invalid battlefield origin: %s"), *Id); return; }
            Template.bPlayable = true;
            Template.Id = FName(*Id);
            Template.MapPackage = FName(*Map);
            Template.ArenaOrigin = FVector(X, Y, Z);
            Seen.Add(Template.Id);
            BattlefieldTemplates.Add(Template);
        }
    }
    ActiveCapPerSide=Config->GetIntegerField(TEXT("active_cap_per_side"));
    PlayerArmy.Reset(); PlayerArmy.Add(PlayerUnitId,Config->GetIntegerField(TEXT("player_strategic_count")));
    EnemyArmies.Reset();
    for (const auto& Pair:World.Regions)
        if (Pair.Value.OwnerFactionId==EnemyFaction) EnemyArmies.Add(Pair.Key,Config->GetIntegerField(TEXT("enemy_strategic_count")));
    // Scenario overlay adds encounters without changing the accepted geography or
    // inventing replacement forces on end-day/load. Saved ownership remains authoritative.
    if (Config->HasField(TEXT("hostile_garrisons")))
    {
        TMap<FName, int32> Garrisons;
        if (!ReadIntMap(*Config, TEXT("hostile_garrisons"), Garrisons))
        {
            UE_LOG(LogSoulCampaign, Error, TEXT("Invalid scenario hostile garrisons."));
            return;
        }
        for (const auto& Garrison : Garrisons)
        {
            if (!World.Regions.Contains(Garrison.Key) || Garrison.Key == PlayerRegion || Garrison.Value <= 0)
            {
                UE_LOG(LogSoulCampaign, Error, TEXT("Invalid hostile garrison region/count: %s"), *Garrison.Key.ToString());
                return;
            }
        }
        for (const auto& Garrison : Garrisons)
        {
            World.Regions.FindChecked(Garrison.Key).OwnerFactionId = EnemyFaction;
            EnemyArmies.Add(Garrison.Key, Garrison.Value);
        }
    }
    Economy=FSoulCampaignEconomy(); Economy.Resources.Add(TEXT("gold"),3000); Economy.DailyIncome.Add(TEXT("gold"),450);
    FSoulRecruitmentPool Pool; Pool.UnitId=PlayerUnitId; Pool.Available=8; Pool.WeeklyGrowth=4;Pool.Capacity=24;
    Pool.CostPerUnit.Add(TEXT("gold"),140); Economy.RecruitmentPools.Add(Pool.UnitId,Pool);
    Hero=FSoulHeroState();Hero.HeroId=TEXT("human_founder_hero"); Hero.UnspentSkillPoints=1;Hero.MaxMana=80;Hero.Mana=80;
    Hero.KnownSpells.Add(TEXT("Magic.Spell.Fire.Firebolt"));
    LastAIReport=EnemyFaction==TEXT("orcs")?TEXT("Orc garrisons hold their regions; strategic reserves feed the active battle."):TEXT("Dwarf garrisons hold their regions; strategic reserves feed the active battle.");
    if(bSixFactionRequested)
    {
        FString Error;
        if(!InitializeSixFactionState(*Starts,*Config,Error)){UE_LOG(LogSoulCampaign,Error,TEXT("SOUL_SIX_FACTION_FAIL %s"),*Error);return;}
    }
    FSoulWorldRules::RefreshVision(World,PlayerFaction,PlayerRegion);bInitialized=true;
    if (bComposition || bAuthoredEnvironmentProof || FParse::Param(FCommandLine::Get(), TEXT("SoulSettlementDevelopmentProof")))
    {
        FString Error;
        auto* DevelopmentScenario = LoadObject<USoulSettlementScenarioData>(nullptr,
            bDwarfEnvironmentProof ? TEXT("/Game/Soul/Data/Settlements/DA_Soul_DwarfHold_DevelopmentProof")
                : TEXT("/Game/Soul/Data/Settlements/DA_Soul_HumanCapital_DevelopmentProof"));
        if (!InitializeSettlementDevelopment(DevelopmentScenario, GetGameInstance()->GetSubsystem<USoulSettlementStateSubsystem>(), Error))
            UE_LOG(LogSoulCampaign, Error, TEXT("SOUL_SETTLEMENT_DEVELOPMENT_UNREADY %s"), *Error);
    }
}

bool USoulFounderPlaytestStateSubsystem::InitializeSettlementDevelopment(
    USoulSettlementScenarioData* Scenario, USoulSettlementStateSubsystem* Authority, FString& OutError)
{
    bSettlementDevelopmentRequested = true;
    SettlementScenario = nullptr;
    SettlementAuthority.Reset();
    auto Reject = [&](const FString& Reason)
    { SettlementDevelopmentError = Reason; OutError = Reason; return false; };
    if (!bInitialized || !Scenario || !Authority || Authority->GetGameInstance() != GetGameInstance())
        return Reject(TEXT("Settlement development requires an initialized campaign, scenario and its GameInstance settlement authority."));
    if (!Scenario->ValidateDefinition(OutError)) return Reject(OutError);
    const auto* Region = World.Regions.Find(Scenario->RegionId);
    if (Scenario->SettlementId != Scenario->RegionId || Scenario->FactionId != PlayerFaction
        || !Region || !Region->bSettlement || Region->OwnerFactionId != PlayerFaction)
        return Reject(TEXT("Development must bind an existing owned settlement region and the player faction."));
    const auto* Tavern = Scenario->FindUniqueServiceDefinition(TEXT("service.tavern_hero"));
    if (!Tavern)
        return Reject(TEXT("The proof scenario requires one building bound to service.tavern_hero."));
    if (!Authority->EnsureScenario(Scenario, OutError)) return Reject(OutError);
    SettlementScenario = Scenario;
    SettlementAuthority = Authority;
    if (!ValidateSettlementDevelopmentBinding(OutError)) return Reject(OutError);
    SettlementDevelopmentError.Reset();
    OutError.Reset();
    ++SettlementDevelopmentRevision;
    return true;
}

bool USoulFounderPlaytestStateSubsystem::IsSettlementDevelopmentReady() const
{
    FString Error;
    return bSettlementDevelopmentRequested && SettlementScenario && SettlementAuthority.IsValid()
        && SettlementDevelopmentError.IsEmpty() && ValidateSettlementDevelopmentBinding(Error);
}

bool USoulFounderPlaytestStateSubsystem::ValidateSettlementDevelopmentBinding(FString& OutError) const
{
    const auto* Settlement = SettlementScenario && SettlementAuthority.IsValid()
        ? SettlementAuthority->FindSettlement(SettlementScenario->SettlementId) : nullptr;
    const auto* Region = SettlementScenario ? World.Regions.Find(SettlementScenario->RegionId) : nullptr;
    if (!Settlement || !Region || Settlement->RegionId != SettlementScenario->RegionId
        || Settlement->FactionId != SettlementScenario->FactionId || Settlement->FactionId != PlayerFaction
        || (!bFourFactionAlpha && Region->OwnerFactionId != PlayerFaction))
    {
        OutError = TEXT("Saved settlement region or ownership differs from the proof binding; saved state was preserved.");
        return false;
    }
    OutError.Reset();
    return true;
}

bool USoulFounderPlaytestStateSubsystem::BeginSettlementConstruction(FName BuildingId, FString& OutError)
{
    if (!IsSettlementDevelopmentReady())
    { OutError = TEXT("Settlement development is unavailable; the proof scenario must load successfully."); return false; }
    const auto* Region = World.Regions.Find(SettlementScenario->RegionId);
    auto* Settlement = SettlementAuthority->FindSettlement(SettlementScenario->SettlementId);
    const auto* Definition = SettlementScenario->FindDevelopmentDefinition(BuildingId);
    if (HasPendingBattle() || bPersistenceBusy || IsAlphaTurnActive() || PlayerRegion != SettlementScenario->RegionId
        || !Region || Region->OwnerFactionId != PlayerFaction || !Settlement
        || Settlement->FactionId != PlayerFaction || Settlement->RegionId != PlayerRegion || !Definition)
    { OutError = TEXT("Construction requires the owned current settlement, a defined building and no active battle or save operation."); return false; }
    if (!FSoulTownRules::BeginConstruction(Economy, *Settlement, Definition->ToDefinition()))
    { OutError = TEXT("Construction prerequisites, resources or building level do not permit this upgrade."); return false; }
    ++SettlementDevelopmentRevision;
    OutError.Reset();
    return true;
}

FName USoulFounderPlaytestStateSubsystem::GetTavernBuildingId() const
{
    const auto* Definition = SettlementScenario
        ? SettlementScenario->FindUniqueServiceDefinition(TEXT("service.tavern_hero")) : nullptr;
    return Definition ? Definition->BuildingId : NAME_None;
}

FName USoulFounderPlaytestStateSubsystem::GetDevelopmentRegion() const
{
    return bSettlementDevelopmentRequested && SettlementScenario ? SettlementScenario->RegionId : FName(TEXT("human_capital"));
}

FString USoulFounderPlaytestStateSubsystem::GetDevelopmentBuildingName() const
{
    const auto* Definition = SettlementScenario ? SettlementScenario->FindDevelopmentDefinition(GetTavernBuildingId()) : nullptr;
    return Definition && !Definition->DisplayName.IsEmpty() ? Definition->DisplayName.ToString() : TEXT("Tavern");
}

bool USoulFounderPlaytestStateSubsystem::IsTavernOperational() const
{
    if (!bSettlementDevelopmentRequested) return true; // Existing founder behavior outside the explicit proof.
    if (!IsSettlementDevelopmentReady()) return false;
    const auto* Definition = SettlementScenario->FindUniqueServiceDefinition(TEXT("service.tavern_hero"));
    const auto* Settlement = SettlementAuthority->FindSettlement(SettlementScenario->SettlementId);
    const auto* Region = World.Regions.Find(SettlementScenario->RegionId);
    return Definition && Definition->UnlockIds.Contains(TEXT("service.tavern_hero")) && Settlement
        && Region && Region->OwnerFactionId == PlayerFaction && Settlement->FactionId == PlayerFaction
        && Settlement->RegionId == SettlementScenario->RegionId
        && FSoulSettlementRules::IsOperational(*Settlement, Definition->BuildingId);
}
bool USoulFounderPlaytestStateSubsystem::IsHostile(FName RegionId) const
{
    const auto* R=World.Regions.Find(RegionId);
    return R&&!R->OwnerFactionId.IsNone()&&R->OwnerFactionId!=PlayerFaction;
}
bool USoulFounderPlaytestStateSubsystem::HasHostileGarrison(FName RegionId) const
{
    if (!IsHostile(RegionId)) return false;
    if(bSixFactionProfile)
    {
        for(const auto& P:OtherFactionStates)
            if(P.Value.Army.RegionId==RegionId && P.Key==World.Regions.FindChecked(RegionId).OwnerFactionId)
                return P.Value.Army.TroopCount>0;
        return !bFourFactionAlpha || !IsAlphaActiveFaction(World.Regions.FindChecked(RegionId).OwnerFactionId); // Alpha explicitly models one field army per faction, no hidden garrisons.
    }
    const int32* Defenders = EnemyArmies.Find(RegionId);
    // Only an explicit exhausted ledger permits occupation. Missing force data
    // must never grant access to hostile territory.
    return !Defenders || *Defenders != 0;
}
bool USoulFounderPlaytestStateSubsystem::MovePlayerTo(FName Target)
{
    InitializeScenario();
    if (IsAlphaTurnActive() || (bFourFactionAlpha && PlayerArmy.FindRef(PlayerUnitId)<=0 && World.Regions.Contains(Target) && World.Regions[Target].OwnerFactionId!=PlayerFaction))return false;
    if (!bInitialized||HasPendingBattle()||bPersistenceBusy||HasHostileGarrison(Target)
        ||(IsHostile(Target)&&PlayerArmy.FindRef(PlayerUnitId)<=0)
        ||!FSoulWorldRules::CanMove(World,PlayerRegion,Target)) return false;
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
    if(bFourFactionAlpha && (IsAlphaTurnActive() || !World.Regions.Contains(Target) || !IsAlphaActiveFaction(World.Regions[Target].OwnerFactionId)))
    {Error=TEXT("Alpha: wait for your turn; Nature and Dark are nonbelligerent.");return false;}
    if (!bInitialized||HasPendingBattle()||bPersistenceBusy||!IsHostile(Target)
        ||!FSoulWorldRules::CanMove(World,PlayerRegion,Target)||PlayerArmy.FindRef(PlayerUnitId)<=0||ArmyCountAtRegion(Target)<=0)
    {Error=TEXT("Encounter requires adjacent hostile forces and a living player army.");return false;}
    Out=FSoulCampaignBattleDescriptor();Out.SourceRegion=PlayerRegion;Out.TargetRegion=Target;
    Out.BattleContext = FSoulWorldRules::BuildBattleContext(World, PlayerRegion, Target, NAME_None, NAME_None, false);
    const auto* Battlefield = FSoulBattlefieldRecipeRules::SelectPlayable(Out.BattleContext, BattlefieldTemplates);
    if (!Battlefield)
    { Error=TEXT("No admitted battlefield supports this encounter."); return false; }
    Out.BattlefieldId = Battlefield->Id;
    Out.PlayerFaction=PlayerFaction;Out.EnemyFaction=World.Regions.FindChecked(Target).OwnerFactionId;
    Out.PlayerUnitId=PlayerUnitId;Out.EnemyUnitId=ArmyUnitAtRegion(Target);Out.MapPackage=Battlefield->MapPackage;Out.ReturnMapPackage=CampaignMap;
    Out.ArenaOrigin=Battlefield->ArenaOrigin;Out.PlayerStrategicCount=PlayerArmy.FindRef(PlayerUnitId);Out.EnemyStrategicCount=ArmyCountAtRegion(Target);
    Out.PlayerMana = Hero.Mana;
    Out.ActiveCapPerSide=ActiveCapPerSide;Out.EncounterOrdinal=EncounterOrdinal+1;
    Out.EncounterId=FName(*FString::Printf(TEXT("encounter.%d.%d.%s.%s"),Economy.Day,Out.EncounterOrdinal,*PlayerRegion.ToString(),*Target.ToString()));
    if(!Out.IsValid())
    {
        Error=FString::Printf(TEXT("Unsupported exact battle binding: %s/%s versus %s/%s. No roster substitution."),
            *Out.PlayerFaction.ToString(),*Out.PlayerUnitId.ToString(),*Out.EnemyFaction.ToString(),*Out.EnemyUnitId.ToString());
        return false;
    }
    Error.Reset();return true;
}
bool USoulFounderPlaytestStateSubsystem::BeginBattle(FName Target,int32 Cap)
{
    FSoulCampaignBattleDescriptor D;FString Error;
    if (Economy.ActionPoints<1||!BuildBattleDescriptor(Target,D,Error)) return false;
    if (Cap>0) D.ActiveCapPerSide=FMath::Clamp(Cap,1,35);
    if (BattleBridge.IsValid()&&!BattleBridge->BeginEncounter(D)) return false;
    if (!FSoulCampaignRules::SpendAction(Economy,1)) return false;
    PendingBattle=D;EncounterOrdinal=D.EncounterOrdinal;
    UE_LOG(LogSoulCampaign,Display,TEXT("SOUL_CAMPAIGN_ENCOUNTER id=%s source=%s target=%s map=%s forces=%d/%d cap=%d recipe=%s landform=%s approach=%s mana=%d"),
        *D.EncounterId.ToString(),*D.SourceRegion.ToString(),*D.TargetRegion.ToString(),*D.MapPackage.ToString(),D.PlayerStrategicCount,D.EnemyStrategicCount,D.ActiveCapPerSide,
        *D.BattlefieldId.ToString(), *D.BattleContext.Landform.ToString(), *D.BattleContext.AttackerApproach.ToString(), D.PlayerMana);
    return true;
}
bool USoulFounderPlaytestStateSubsystem::ApplyBattleResult(const FSoulCampaignBattleResult& R)
{
    if (!HasPendingBattle() || ResolvedEncounters.Contains(R.EncounterId) || !R.IsValidFor(PendingBattle)) return false;
    if (bSixFactionProfile && PendingBattle.PlayerFaction!=PlayerFaction)
    {
        auto* Attacker=OtherFactionStates.Find(PendingBattle.PlayerFaction);
        auto* Defender=PendingBattle.EnemyFaction==PlayerFaction?nullptr:OtherFactionStates.Find(PendingBattle.EnemyFaction);
        if (!Attacker || (PendingBattle.EnemyFaction!=PlayerFaction && !Defender)) return false;
        Attacker->Army.TroopCount=R.PlayerSurvivors;
        if (Defender) Defender->Army.TroopCount=R.EnemySurvivors;
        else
        {
            PlayerArmy.FindOrAdd(PendingBattle.EnemyUnitId)=R.EnemySurvivors;
            if(PendingBattle.TacticalPlayerSide==1) Hero.Mana=R.PlayerManaRemaining;
        }
        Attacker->Army.RegionId=R.bPlayerWon?PendingBattle.TargetRegion:PendingBattle.SourceRegion;
        if (R.bPlayerWon) FSoulWorldRules::Capture(World,PendingBattle.TargetRegion,PendingBattle.PlayerFaction);
        FSoulWorldRules::RefreshVision(World,PendingBattle.PlayerFaction,Attacker->Army.RegionId);
        FSoulWorldRules::RefreshVision(World,PendingBattle.EnemyFaction,PendingBattle.TargetRegion);
        // PlayerRegion/hero/economy remain the real Human state. No temporary faction swapping.
        bBattleWon=R.bPlayerWon; // Existing saved result flag is attacker-side outcome.
        UE_LOG(LogSoulCampaign,Display,TEXT("SOUL_CONTROLLED_RESULT attacker=%s defender=%s region=%s owner=%s victory=%d survivors=%d/%d"),
            *PendingBattle.PlayerFaction.ToString(),*PendingBattle.EnemyFaction.ToString(),*Attacker->Army.RegionId.ToString(),
            *World.Regions.FindChecked(PendingBattle.TargetRegion).OwnerFactionId.ToString(),R.bPlayerWon,R.PlayerSurvivors,R.EnemySurvivors);
        AppendAlphaBattleRecap(R);
        LastBattleResult=R;ResolvedEncounters.Add(R.EncounterId);PendingBattle=FSoulCampaignBattleDescriptor();return true;
    }
    PlayerArmy.FindOrAdd(PendingBattle.PlayerUnitId)=R.PlayerSurvivors;
    if(bSixFactionProfile)OtherFactionStates.FindChecked(PendingBattle.EnemyFaction).Army.TroopCount=R.EnemySurvivors;
    else EnemyArmies.FindOrAdd(PendingBattle.TargetRegion)=R.EnemySurvivors;
    bBattleWon=R.bPlayerWon;
    Hero.Mana = R.PlayerManaRemaining;
    UE_LOG(LogSoulCampaign, Display, TEXT("SOUL_CAMPAIGN_MANA id=%s before=%d after=%d casts=%d"),
        *R.EncounterId.ToString(), PendingBattle.PlayerMana, Hero.Mana, R.MagicCasts);
    if (R.bPlayerWon)
    {
        FSoulWorldRules::Capture(World,PendingBattle.TargetRegion,PendingBattle.PlayerFaction);PlayerRegion=PendingBattle.TargetRegion;
        FSoulHeroRules::AddExperience(Hero,360);
        if (!RewardedRegions.Contains(PlayerRegion)) {RewardedRegions.Add(PlayerRegion);Economy.Resources.FindOrAdd(TEXT("gold"))+=600;}
    }
    else PlayerRegion=PendingBattle.SourceRegion;
    FSoulWorldRules::RefreshVision(World,PlayerFaction,PlayerRegion);
    AppendAlphaBattleRecap(R);
    LastBattleResult=R;ResolvedEncounters.Add(R.EncounterId);PendingBattle=FSoulCampaignBattleDescriptor();
    UE_LOG(LogSoulCampaign,Display,TEXT("SOUL_CAMPAIGN_RESULT id=%s target=%s victory=%d survivors=%d/%d player_region=%s"),
        *R.EncounterId.ToString(),*R.TargetRegion.ToString(),R.bPlayerWon,R.PlayerSurvivors,R.EnemySurvivors,*PlayerRegion.ToString());
    return true;
}
void USoulFounderPlaytestStateSubsystem::HandleBattleResolved(const FSoulCampaignBattleResult& R)
{if(ApplyBattleResult(R)){if(!bFourFactionAlpha)SaveCampaign();}else UE_LOG(LogSoulCampaign,Error,TEXT("Rejected campaign result"));}
// In the playable alpha, returning from battle preserves GameInstance state but does not overwrite the player's F5 checkpoint.
void USoulFounderPlaytestStateSubsystem::AdvanceDay()
{
    InitializeScenario();if(HasPendingBattle()||bPersistenceBusy||IsAlphaTurnActive())return;
    FSoulSettlementState* Settlement = nullptr;
    if (bSettlementDevelopmentRequested)
    {
        if (!IsSettlementDevelopmentReady()) return;
        Settlement = SettlementAuthority->FindSettlement(SettlementScenario->SettlementId);
        if (!Settlement) return;
        // Occupation changes campaign ownership, not the authored building-state identity.
        // Preserve construction while occupied; it resumes only after lawful recapture.
        if(bFourFactionAlpha && World.Regions.FindChecked(SettlementScenario->RegionId).OwnerFactionId!=PlayerFaction) Settlement=nullptr;
    }
    FSoulCampaignRules::AdvanceDay(Economy);
    if(bSixFactionProfile)for(auto& P:OtherFactionStates)FSoulCampaignRules::AdvanceDay(P.Value.Economy);
    if (Settlement) { FSoulSettlementRules::AdvanceDay(*Settlement); ++SettlementDevelopmentRevision; }
    Hero.Mana=FMath::Min(Hero.MaxMana,Hero.Mana+6);AdvanceEnemyAI();
}
void USoulFounderPlaytestStateSubsystem::AdvanceEnemyAI()
{if(bFourFactionAlpha){if(AlphaTurnDay>=Economy.Day)return;AlphaTurnDay=Economy.Day;AlphaNextFaction=0;LastAIReport=FString::Printf(TEXT("Day %d activity"),Economy.Day);return;}if(bSixFactionProfile){LastAIReport=TEXT("Six-faction strategic AI OFF. Only explicit player/qualification actions run.");return;}LastAIReport=EnemyFaction==TEXT("orcs")?TEXT("Orc garrisons hold; no synthetic strategic army bypasses encounter resolution."):TEXT("Dwarf garrisons hold; no synthetic strategic army bypasses encounter resolution.");}
bool USoulFounderPlaytestStateSubsystem::Recruit(FName Id)
{InitializeScenario();if(IsAlphaTurnActive())return false;if(HasPendingBattle()||bPersistenceBusy||PlayerRegion!=TEXT("human_capital")||(bFourFactionAlpha&&World.Regions.FindChecked(PlayerRegion).OwnerFactionId!=PlayerFaction)||!FSoulCampaignRules::Recruit(Economy,Id,1))return false;PlayerArmy.FindOrAdd(Id)++;return true;}
bool USoulFounderPlaytestStateSubsystem::ChooseSkill(FName Id)
{
    if(HasPendingBattle()||bPersistenceBusy||!FSoulHeroRules::SpendSkillPoint(Hero,Id,2))return false;
    if(Id==TEXT("Adventure")){Economy.MaxActionPoints++;Economy.ActionPoints++;}
    else if(Id==TEXT("Magic")){Hero.MaxMana+=10;Hero.Mana+=10;}return true;
}
bool USoulFounderPlaytestStateSubsystem::HireTavernHero()
{
    if(HasPendingBattle()||bPersistenceBusy||PlayerRegion!=GetDevelopmentRegion()||!IsTavernOperational()||bSecondHeroHired||Economy.Resources.FindRef(TEXT("gold"))<1200)return false;
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
    Root->SetNumberField(TEXT("hero_kind"), static_cast<int32>(Hero.Kind));
    Root->SetObjectField(TEXT("hero"),IntMap(HeroNumbers));Root->SetObjectField(TEXT("skills"),IntMap(Hero.Skills));Root->SetArrayField(TEXT("spells"),NameSet(Hero.KnownSpells));
    Root->SetStringField(TEXT("result_id"),LastBattleResult.EncounterId.ToString());Root->SetStringField(TEXT("result_target"),LastBattleResult.TargetRegion.ToString());
    Root->SetBoolField(TEXT("result_won"),LastBattleResult.bPlayerWon);
    TMap<FName,int32> Result={{TEXT("player"),LastBattleResult.PlayerSurvivors},{TEXT("enemy"),LastBattleResult.EnemySurvivors},{TEXT("player_waves"),LastBattleResult.PlayerReinforcements},{TEXT("enemy_waves"),LastBattleResult.EnemyReinforcements},{TEXT("magic"),LastBattleResult.MagicCasts}};
    Root->SetObjectField(TEXT("result"),IntMap(Result));
    Root->SetNumberField(TEXT("result_mana"), LastBattleResult.PlayerManaRemaining);
    if(bSixFactionProfile)CaptureSixFactionState(*Root);
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
    // Explicit profile payload prevents cross-slot copies being interpreted as legacy saves.
    if(Root->HasField(TEXT("profile"))!=bSixFactionProfile)
    {Error=TEXT("Campaign profile mismatch; no migration is permitted.");return false;}
    TMap<FName,FSoulFactionCampaignState> RestoredFactions;FSoulWorldState RestoredWorld;
    if(bSixFactionProfile&&!ValidateSixFactionRestore(*Root,RestoredFactions,RestoredWorld,Error))return false;
    // Restore into temporary values: invalid snapshots must not partially mutate authority.
    TMap<FName,int32> Army,Enemies,Resources,Income,Pools,Numbers,Skills,Result;
    TSet<FName> Rewarded,Resolved,Explored,Spells;
    FString Region,Enemy,PF,EF,ResultId,ResultTarget;
    int32 Day=0,AP=0,MaxAP=0,Ordinal=0,ResultMana=0,HeroKind=0;bool Won=false,Hired=false,ResultWon=false;
    const TSharedPtr<FJsonObject>* Owners=nullptr;
    bool Valid=Root->TryGetStringField(TEXT("player_region"),Region)&&Root->TryGetStringField(TEXT("enemy_region"),Enemy)
        &&World.Regions.Contains(FName(*Region))&&World.Regions.Contains(FName(*Enemy))
        &&Root->TryGetStringField(TEXT("player_faction"),PF)&&FName(*PF)==PlayerFaction
        &&Root->TryGetStringField(TEXT("enemy_faction"),EF)&&FName(*EF)==EnemyFaction
        &&ReadNonNegativeInt(*Root,TEXT("day"),Day)&&Day>0&&ReadNonNegativeInt(*Root,TEXT("ap"),AP)
        &&ReadNonNegativeInt(*Root,TEXT("max_ap"),MaxAP)&&MaxAP>0&&AP<=MaxAP
        &&ReadNonNegativeInt(*Root,TEXT("ordinal"),Ordinal)
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
        if (Root->HasField(TEXT("hero_kind")))
            Valid &= ReadNonNegativeInt(*Root, TEXT("hero_kind"), HeroKind);
        Valid &= HeroKind <= static_cast<int32>(ESoulHeroKind::Paragon);
        // Schema-1 checkpoints predating mana transfer have no result receipt.
        // The hero's saved resource balance remains authoritative on that path.
        ResultMana = Numbers.FindRef(TEXT("mana"));
        if (Root->HasField(TEXT("result_mana")))
            Valid &= ReadNonNegativeInt(*Root, TEXT("result_mana"), ResultMana);
        Valid &= ResultMana <= Numbers.FindRef(TEXT("max_mana"));
        Valid &= Numbers.Num()==5&&Numbers.Contains(TEXT("level"))&&Numbers.Contains(TEXT("xp"))&&Numbers.Contains(TEXT("points"))&&Numbers.Contains(TEXT("mana"))&&Numbers.Contains(TEXT("max_mana"))
            &&Numbers.FindRef(TEXT("level"))>0&&Numbers.FindRef(TEXT("mana"))<=Numbers.FindRef(TEXT("max_mana"))
            &&Result.Num()==5&&Result.Contains(TEXT("player"))&&Result.Contains(TEXT("enemy"))&&Result.Contains(TEXT("player_waves"))&&Result.Contains(TEXT("enemy_waves"))&&Result.Contains(TEXT("magic"));
        // Required pools must be present: otherwise a load into a used subsystem
        // silently retains the previous campaign's remaining recruitment stock.
        Valid &= Pools.Num() == Economy.RecruitmentPools.Num();
        for(const auto& P:Enemies)Valid&=World.Regions.Contains(P.Key);
        for(const auto& P:Pools)
        {
            const auto* Pool = Economy.RecruitmentPools.Find(P.Key);
            Valid &= Pool && P.Value <= Pool->Capacity;
        }
        for(FName R:Rewarded)Valid&=World.Regions.Contains(R);for(FName R:Explored)Valid&=World.Regions.Contains(R);
        for(const auto& P:World.Regions)
        {
            FString Owner;Valid&=(*Owners)->TryGetStringField(P.Key.ToString(),Owner);
            Valid&=Owner==TEXT("None")||(bSixFactionProfile?FSoulCampaignRules::CanonicalFactions().Contains(FName(*Owner)):(FName(*Owner)==PlayerFaction||FName(*Owner)==EnemyFaction));
            // A hostile region needs an explicit ledger, including zero after
            // mutual exhaustion. Captured regions cannot hide surviving enemies.
            if(bSixFactionProfile)Valid &= Enemies.Num()==0;
            else if (FName(*Owner) == EnemyFaction) Valid &= Enemies.Contains(P.Key);
            else Valid &= Enemies.FindRef(P.Key) == 0;
        }
        const FName LastId(*ResultId), LastTarget(*ResultTarget);
        Valid &= Resolved.Num() == Ordinal;
        if (Ordinal == 0)
        {
            Valid &= LastId.IsNone() && LastTarget.IsNone() && !Won && !ResultWon;
            for (const auto& P : Result) Valid &= P.Value == 0;
        }
        else
        {
            Valid &= !LastId.IsNone() && Resolved.Contains(LastId)
                && World.Regions.Contains(LastTarget) && Won == ResultWon;
            if (ResultWon)
                Valid &= Result.FindRef(TEXT("player")) > 0 && Result.FindRef(TEXT("enemy")) == 0;
            else Valid &= Result.FindRef(TEXT("player")) == 0;
        }
    }
    int32 RestoredCursor=3,RestoredSeed=1701,RestoredTurnDay=0;FString RestoredRecap;
    if(bFourFactionAlpha)
        Valid &= ReadNonNegativeInt(*Root,TEXT("alpha_cursor"),RestoredCursor)&&RestoredCursor<=3
            && ReadNonNegativeInt(*Root,TEXT("alpha_turn_day"),RestoredTurnDay)&&RestoredTurnDay<=Day
            && (RestoredTurnDay==Day || (Day==1 && RestoredTurnDay==0 && RestoredCursor==3))
            && ReadNonNegativeInt(*Root,TEXT("alpha_seed"),RestoredSeed)&&RestoredSeed<=1000000
            && Root->TryGetStringField(TEXT("alpha_recap"),RestoredRecap)&&RestoredRecap.Len()<=4096;
    if(!Valid){Error=TEXT("Campaign snapshot failed validation.");return false;}
    if(bFourFactionAlpha){AlphaNextFaction=RestoredCursor;AlphaSeed=RestoredSeed;AlphaTurnDay=RestoredTurnDay;LastAIReport=MoveTemp(RestoredRecap);}
    if(bSixFactionProfile){World=MoveTemp(RestoredWorld);OtherFactionStates=MoveTemp(RestoredFactions);}
    for(auto& P:World.Regions)P.Value.OwnerFactionId=FName(*(*Owners)->GetStringField(P.Key.ToString()));
    Economy.Day=Day;Economy.ActionPoints=AP;Economy.MaxActionPoints=MaxAP;Economy.Resources=MoveTemp(Resources);Economy.DailyIncome=MoveTemp(Income);
    for(const auto& P:Pools)Economy.RecruitmentPools[P.Key].Available=P.Value;
    Hero.Kind = static_cast<ESoulHeroKind>(HeroKind);
    Hero.Level=Numbers[TEXT("level")];Hero.Experience=Numbers[TEXT("xp")];Hero.UnspentSkillPoints=Numbers[TEXT("points")];Hero.Mana=Numbers[TEXT("mana")];Hero.MaxMana=Numbers[TEXT("max_mana")];Hero.Skills=MoveTemp(Skills);Hero.KnownSpells=MoveTemp(Spells);
    PlayerArmy=MoveTemp(Army);EnemyArmies=MoveTemp(Enemies);PlayerRegion=FName(*Region);EnemyRegion=FName(*Enemy);
    EncounterOrdinal=Ordinal;bBattleWon=Won;bSecondHeroHired=Hired;RewardedRegions=MoveTemp(Rewarded);ResolvedEncounters=MoveTemp(Resolved);
    LastBattleResult.EncounterId=FName(*ResultId);LastBattleResult.TargetRegion=FName(*ResultTarget);LastBattleResult.bPlayerWon=ResultWon;
    LastBattleResult.PlayerSurvivors=Result[TEXT("player")];LastBattleResult.EnemySurvivors=Result[TEXT("enemy")];
    LastBattleResult.PlayerReinforcements=Result[TEXT("player_waves")];LastBattleResult.EnemyReinforcements=Result[TEXT("enemy_waves")];LastBattleResult.MagicCasts=Result[TEXT("magic")];
    LastBattleResult.PlayerManaRemaining = ResultMana;
    World.KnowledgeByFaction.FindOrAdd(PlayerFaction).ExploredRegions=MoveTemp(Explored);FSoulWorldRules::RefreshVision(World,PlayerFaction,PlayerRegion);
    Error.Reset();return true;
}
FString USoulFounderPlaytestStateSubsystem::GetCampaignSaveSlotName() const
{
    if(bSixFactionProfile)return SixFactionSaveSlot;
    if(SoulCampaignTerrain::Composition()&&FParse::Param(FCommandLine::Get(),TEXT("SoulVikingMatchupProof")))
        return TEXT("Soul.Composition3500.VikingProof");
    // Presentation profiles retain the same RB Save domains/schema, but a
    // 36-region candidate must never overwrite the qualified founder slot.
    if (SoulCampaignTerrain::Composition() && FParse::Param(FCommandLine::Get(),TEXT("SoulOrcMatchupProof")))
        return TEXT("Soul.Composition3500.OrcProof");
    if (SoulCampaignTerrain::Composition())
        return FParse::Param(FCommandLine::Get(), TEXT("SoulHumanSettlementProof"))
            ? TEXT("Soul.Composition3500.HumanProof") : TEXT("Soul.Composition3500.Founder");
    return TEXT("Soul.VerticalCampaign");
}

void USoulFounderPlaytestStateSubsystem::SaveCampaign()
{
    if(!SaveSubsystem.IsValid()||bPersistenceBusy||HasPendingBattle())return;
    bPersistenceBusy=true;bLastSaveSucceeded=false;
    FRBSaveOperationDelegate Done;Done.BindDynamic(this,&USoulFounderPlaytestStateSubsystem::OnCampaignSaved);
    UE_LOG(LogSoulCampaign,Display,TEXT("SOUL_CAMPAIGN_SAVE_SCOPE domains=Soul.Campaign,Soul.Settlements"));
    SaveSubsystem->SaveSelectedDomainsAsync(GetCampaignSaveSlotName(),CampaignSaveDomains,Done);
}
void USoulFounderPlaytestStateSubsystem::LoadCampaign()
{
    if(!SaveSubsystem.IsValid()||bPersistenceBusy||HasPendingBattle())return;
    bPersistenceBusy=true;bLastLoadSucceeded=false;
    FRBSaveOperationDelegate Done;Done.BindDynamic(this,&USoulFounderPlaytestStateSubsystem::OnCampaignLoaded);
    SaveSubsystem->LoadSelectedDomainsAsync(GetCampaignSaveSlotName(),CampaignSaveDomains,Done);
}
void USoulFounderPlaytestStateSubsystem::OnCampaignSaved(const FRBSaveOperationResult& R)
{
    bPersistenceBusy=false;bLastSaveSucceeded=R.bSuccess;LastPersistenceReport=R.bSuccess?TEXT("Campaign saved with RB Save."):R.Message;
    const FString SavedPath = SaveSubsystem.IsValid()
        ? SaveSubsystem->GetDomainSlotPath(GetCampaignSaveSlotName()) : R.ArtifactPath;
    UE_LOG(LogSoulCampaign,Display,TEXT("SOUL_CAMPAIGN_SAVE success=%d path=%s message=%s"),R.bSuccess,*SavedPath,*R.Message);
}
void USoulFounderPlaytestStateSubsystem::OnCampaignLoaded(const FRBSaveOperationResult& R)
{
    if(R.bSuccess)++CampaignLoadRevision;
    if (R.bSuccess && bSettlementDevelopmentRequested && SettlementScenario && SettlementAuthority.IsValid())
    {
        // Old saves may have no settlement entry. Seed only that absence; never overwrite saved progress.
        if (!SettlementAuthority->EnsureScenario(SettlementScenario, SettlementDevelopmentError)
            || !ValidateSettlementDevelopmentBinding(SettlementDevelopmentError))
            UE_LOG(LogSoulCampaign, Error, TEXT("SOUL_SETTLEMENT_DEVELOPMENT_UNREADY %s"), *SettlementDevelopmentError);
        ++SettlementDevelopmentRevision;
    }
    bPersistenceBusy=false;bLastLoadSucceeded=R.bSuccess;LastPersistenceReport=R.bSuccess?TEXT("Campaign reloaded with RB Save."):R.Message;
    UE_LOG(LogSoulCampaign,Display,TEXT("SOUL_CAMPAIGN_LOAD success=%d region=%s knights=%d xp=%d"),R.bSuccess,*PlayerRegion.ToString(),PlayerArmy.FindRef(PlayerUnitId),Hero.Experience);
}
