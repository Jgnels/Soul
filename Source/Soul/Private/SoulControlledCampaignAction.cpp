#include "SoulFounderPlaytestStateSubsystem.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

bool USoulFounderPlaytestStateSubsystem::ValidateControlledAction(FName Id, FName ArmyId,
    FName Source, FName Target, FSoulCampaignBattleDescriptor& Encounter, FString& Error) const
{
    Encounter = FSoulCampaignBattleDescriptor();
    auto Reject = [&Error](const TCHAR* Reason) { Error = Reason; return false; };
    if (!IsInGameThread()) return Reject(TEXT("REJECT_NOT_GAME_THREAD"));
    if (!bInitialized || !bSixFactionProfile || bPersistenceBusy || HasPendingBattle()
        || (BattleBridge.IsValid() && BattleBridge->GetPendingEncounter()))
        return Reject(TEXT("REJECT_CAMPAIGN_NOT_READY"));
    if (!FSoulCampaignRules::CanonicalFactions().Contains(Id))
        return Reject(TEXT("REJECT_UNKNOWN_FACTION"));
    FSoulFactionCampaignState F;
    if (!InspectFactionArmy(Id, F) || F.Army.ArmyId != ArmyId)
        return Reject(TEXT("REJECT_MISSING_ARMY"));
    if (F.Army.FactionId != Id) return Reject(TEXT("REJECT_ARMY_OWNER"));
    if (F.Army.RegionId != Source) return Reject(TEXT("REJECT_STALE_SOURCE"));
    const auto* Origin = World.Regions.Find(Source);
    const auto* Destination = World.Regions.Find(Target);
    if (!Origin || !Destination) return Reject(TEXT("REJECT_INVALID_REGION"));
    const bool EmptyWithdrawal=bFourFactionAlpha && F.Army.TroopCount==0 && Destination->OwnerFactionId==Id;
    if(F.Army.TroopCount<=0 && !EmptyWithdrawal)return Reject(TEXT("REJECT_MISSING_ARMY"));
    if (Origin->OwnerFactionId != Id && !EmptyWithdrawal) return Reject(TEXT("REJECT_SOURCE_OWNER"));
    if (!Destination->OwnerFactionId.IsNone() && !FSoulCampaignRules::CanonicalFactions().Contains(Destination->OwnerFactionId))
        return Reject(TEXT("REJECT_DESTINATION_OWNER"));
    if (!FSoulWorldRules::CanMove(World, Source, Target)) return Reject(TEXT("REJECT_NONADJACENT"));
    const bool Hostile = !Destination->OwnerFactionId.IsNone() && Destination->OwnerFactionId != Id;
    if(Hostile&&!DiplomacyAllowsHostility(Id,Destination->OwnerFactionId))return Reject(TEXT("REJECT_TREATY_NO_MILITARY_ACCESS"));
    if(bFourFactionAlpha && (!IsAlphaActiveFaction(Id) || (Hostile&&!IsAlphaActiveFaction(Destination->OwnerFactionId))))
        return Reject(TEXT("REJECT_ALPHA_PASSIVE_FACTION"));
    if(bFourFactionAlpha && Id==PlayerFaction && IsAlphaTurnActive())return Reject(TEXT("REJECT_AI_TURN_ACTIVE"));
    const int32 Cost = !Hostile && Id == PlayerFaction && FSoulHeroRules::IsAvailable(Hero) && Hero.Skills.FindRef(TEXT("Adventure")) >= 2 ? 0 : 1;
    // Evaluate the same spend rule against a copy; admission never debits live state.
    auto Funds = F.Economy;
    if (!FSoulCampaignRules::SpendAction(Funds, Cost)) return Reject(TEXT("REJECT_INSUFFICIENT_AP"));
    if (Hostile && !(bFourFactionAlpha && ArmyCountAtRegion(Target)==0))
    {
        if (!BuildFactionBattleDescriptor(Id, Target, Encounter, Error)) return false;
        const auto* Recipe = BattlefieldTemplates.FindByPredicate([&](const FSoulBattlefieldTemplate& T) { return T.Id==Encounter.BattlefieldId; });
        // Playable Alpha uses the same already-qualified field fallback as Human attacks.
        // This admits a provisional battlefield, not a fabricated directed approach or roster.
        // Earlier controlled qualification profiles keep their strict geography requirement.
        const bool AlphaFieldFallback=bFourFactionAlpha && Recipe && Recipe->bPlayable
            && Recipe->Id==TEXT("dragon_graveyard") && Recipe->MapPackage==BattleMap && Recipe->ArenaOrigin==BattleOrigin;
        const auto* City=EnvironmentRegistry.Find(Target);
        const bool RegisteredCity=bHeartlandEnabled&&City&&City->bBattleEnabled
            && (Encounter.bSiege ? (!City->SiegeEnvironment.IsEmpty()&&Encounter.MapPackage==FName(*City->SiegeEnvironment)&&Encounter.ArenaOrigin==City->SiegeOrigin)
                : (Encounter.MapPackage==FName(*City->BattleEnvironment())&&Encounter.ArenaOrigin==City->ArenaOrigin));
        if (!RegisteredCity && (!Recipe || (!AlphaFieldFallback && !Recipe->Biomes.Contains(Encounter.BattleContext.Biome)
            && !Recipe->Landforms.Contains(Encounter.BattleContext.Landform)
            && !Recipe->Features.Contains(Encounter.BattleContext.Feature))))
            return Reject(TEXT("REJECT_NO_GEOGRAPHIC_APPROACH"));
        if (!BattleBridge.IsValid() || BattleBridge->GetPendingEncounter())
            return Reject(TEXT("REJECT_BATTLE_BRIDGE_UNAVAILABLE"));
    }
    Error.Reset();
    return true;
}

bool USoulFounderPlaytestStateSubsystem::PrepareControlledAction(FName Id, FName ArmyId,
    FName Source, FName Target, FSoulControlledCampaignAction& Out, FString& Error) const
{
    Out = FSoulControlledCampaignAction();
    FSoulCampaignBattleDescriptor Encounter;
    if (!ValidateControlledAction(Id, ArmyId, Source, Target, Encounter, Error)) return false;
    FRBSaveDomainState Snapshot;
    if (!CaptureRBSaveDomain_Implementation(Snapshot, Error) || Snapshot.Fields.Num() != 1) return false;
    Out.FactionId = Id; Out.ArmyId = ArmyId; Out.SourceRegion = Source; Out.TargetRegion = Target;
    Out.ExpectedProfile = GetCampaignSaveSlotName();
    Out.ExpectedLoadRevision = CampaignLoadRevision;
    Out.ExpectedCampaignState = Snapshot.Fields[0].StringValue;
    return true;
}

bool USoulFounderPlaytestStateSubsystem::ExecuteControlledAction(const FSoulControlledCampaignAction& Action, FString& Error)
{
    if(Action.RecruitQuantity>0)return ExecuteControlledRecruitment(Action,Error);
    // No authority is granted by a proposal. Revalidate everything on the game thread,
    // including battle admission, immediately before the normal authority executes.
    if (!IsInGameThread()) { Error=TEXT("REJECT_NOT_GAME_THREAD"); return false; }
    if (Action.ExpectedProfile != GetCampaignSaveSlotName() || Action.ExpectedLoadRevision != CampaignLoadRevision)
    { Error = TEXT("REJECT_STALE_PROFILE_OR_LOAD"); return false; }
    FSoulCampaignBattleDescriptor Encounter;
    if (!ValidateControlledAction(Action.FactionId, Action.ArmyId, Action.SourceRegion, Action.TargetRegion, Encounter, Error)) return false;
    FRBSaveDomainState Current;
    if (!CaptureRBSaveDomain_Implementation(Current, Error) || Current.Fields.Num() != 1) return false;
    if (Action.ExpectedCampaignState.IsEmpty() || Current.Fields[0].StringValue != Action.ExpectedCampaignState)
    { Error = TEXT("REJECT_STALE_CAMPAIGN_STATE"); return false; }
    if (!Encounter.EncounterId.IsNone())
    {
        if (Action.FactionId == PlayerFaction)
        {
            if (!BeginBattle(Action.TargetRegion)) { Error = TEXT("REJECT_BATTLE_COMMIT"); return false; }
        }
        else
        {
            auto* F = OtherFactionStates.Find(Action.FactionId);
            auto Paid = F->Economy;
            if (!FSoulCampaignRules::SpendAction(Paid,1) || !BattleBridge.IsValid() || !BattleBridge->BeginEncounter(Encounter))
            { Error = TEXT("REJECT_BATTLE_COMMIT"); return false; }
            // All fallible admission is complete. No observer/callback runs between these assignments.
            F->Economy = MoveTemp(Paid); PendingBattle = Encounter; EncounterOrdinal = Encounter.EncounterOrdinal;
            UE_LOG(LogTemp,Display,TEXT("SOUL_CONTROLLED_ENCOUNTER faction=%s from=%s to=%s recipe=%s ap=%d"),
                *Action.FactionId.ToString(),*Encounter.SourceRegion.ToString(),*Encounter.TargetRegion.ToString(),*Encounter.BattlefieldId.ToString(),F->Economy.ActionPoints);
            if(bFourFactionAlpha && Encounter.BattlefieldId==TEXT("dragon_graveyard"))
                UE_LOG(LogTemp,Display,TEXT("SOUL_ALPHA_FIELD_FALLBACK target=%s exact_rosters=1 provisional_environment=1 directed_approach_unchanged=1"),*Encounter.TargetRegion.ToString());
        }
        Error.Reset(); return true;
    }
    return MoveFactionArmy(Action.FactionId, Action.TargetRegion, Error);
}

bool USoulFounderPlaytestStateSubsystem::BuildFactionBattleDescriptor(FName Id, FName Target,
    FSoulCampaignBattleDescriptor& Out, FString& Error) const
{
    if (Id==PlayerFaction) return BuildBattleDescriptor(Target,Out,Error);
    Out=FSoulCampaignBattleDescriptor();
    FSoulFactionCampaignState Attacker,Defender;
    const auto* Region=World.Regions.Find(Target);
    if (!bInitialized || !bSixFactionProfile || bPersistenceBusy || HasPendingBattle() || !Region
        || !InspectFactionArmy(Id,Attacker) || Attacker.Army.TroopCount<=0
        || Region->OwnerFactionId==Id || Region->OwnerFactionId.IsNone() || !DiplomacyAllowsHostility(Id,Region->OwnerFactionId)
        || !InspectFactionArmy(Region->OwnerFactionId,Defender) || Defender.Army.RegionId!=Target || Defender.Army.TroopCount<=0
        || !FSoulWorldRules::CanMove(World,Attacker.Army.RegionId,Target))
    { Error=TEXT("REJECT_INVALID_HOSTILE_FORCES_OR_EDGE"); return false; }
    if (!FSoulCampaignBattleDescriptor::SupportsExactPair(Id,Attacker.Army.UnitId,Defender.Army.FactionId,Defender.Army.UnitId))
    { Error=TEXT("REJECT_UNSUPPORTED_ORDERED_PAIR"); return false; }
    Out.SourceRegion=Attacker.Army.RegionId;Out.TargetRegion=Target;
    Out.BattleContext=FSoulWorldRules::BuildBattleContext(World,Out.SourceRegion,Target,NAME_None,NAME_None,false);
    const auto* Recipe=FSoulBattlefieldRecipeRules::SelectPlayable(Out.BattleContext,BattlefieldTemplates);
    if (!Recipe) { Error=TEXT("REJECT_NO_APPROACH"); return false; }
    Out.BattlefieldId=Recipe->Id;Out.MapPackage=Recipe->MapPackage;Out.ArenaOrigin=Recipe->ArenaOrigin;Out.ReturnMapPackage=CampaignMap;
    Out.PlayerFaction=Id;Out.PlayerUnitId=Attacker.Army.UnitId;Out.PlayerStrategicCount=Attacker.Army.TroopCount;
    Out.EnemyFaction=Defender.Army.FactionId;Out.EnemyUnitId=Defender.Army.UnitId;Out.EnemyStrategicCount=Defender.Army.TroopCount;
    Out.bAutoResolve=bFourFactionAlpha && Defender.Army.FactionId!=PlayerFaction;
    Out.PlayerMana=0;
    if (Defender.Army.FactionId==PlayerFaction && (bFourFactionAlpha || FParse::Param(FCommandLine::Get(),TEXT("SoulHumanDefenseProof"))))
    { Out.TacticalPlayerSide=1; Out.PlayerMana=Hero.Mana; }
    Out.ActiveCapPerSide=ActiveCapPerSide;Out.EncounterOrdinal=EncounterOrdinal+1;
    Out.EncounterId=FName(*FString::Printf(TEXT("encounter.%d.%d.%s.%s"),Attacker.Economy.Day,Out.EncounterOrdinal,*Out.SourceRegion.ToString(),*Target.ToString()));
    if(!ApplySettlementEnvironment(Out,Error))return false;
    ConfigureHeartlandMagic(Out);
    if (!Out.IsValid()) { Error=TEXT("REJECT_INVALID_EXACT_DESCRIPTOR"); return false; }
    Error.Reset();return true;
}
