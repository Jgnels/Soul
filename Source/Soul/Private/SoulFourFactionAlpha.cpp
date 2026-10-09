#include "SoulFounderPlaytestStateSubsystem.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Crc.h"

namespace
{
const TArray<FName>& ActiveAI()
{
    static const TArray<FName> Ids={TEXT("dwarves"),TEXT("orcs"),TEXT("vikings")};return Ids;
}
FName Capital(FName Id)
{
    if(Id==TEXT("dwarves"))return TEXT("dwarf_hold");
    if(Id==TEXT("orcs"))return TEXT("orc_camp");
    if(Id==TEXT("vikings"))return TEXT("viking_harbour");
    return TEXT("human_capital");
}
}

bool USoulFounderPlaytestStateSubsystem::IsAlphaActiveFaction(FName Id) const
{return Id==PlayerFaction || ActiveAI().Contains(Id);}

void USoulFounderPlaytestStateSubsystem::InitializeFourFactionAlpha()
{
    bFourFactionAlpha=true;SixFactionSaveSlot=TEXT("Soul.Composition3500.FourFactionAlpha");
    FParse::Value(FCommandLine::Get(),TEXT("SoulAlphaSeed="),AlphaSeed);
    AlphaSeed=FMath::Clamp(AlphaSeed,0,1000000);AlphaNextFaction=3;
    for(FName Id:ActiveAI())
    {
        auto& F=OtherFactionStates.FindChecked(Id);
        // Same finite core-infantry pool/cost and daily economy as the Human founder.
        F.Economy.DailyIncome=Economy.DailyIncome;
        auto Pool=Economy.RecruitmentPools.FindChecked(PlayerUnitId);Pool.UnitId=F.Army.UnitId;
        F.Economy.RecruitmentPools.Add(Pool.UnitId,Pool);
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("SoulAlphaDefenseProof")))
    {
        // Explicit isolated start-force fixture, not a live reinforcement or ownership override.
        SixFactionSaveSlot+=TEXT(".DefenseProof");PlayerArmy.FindOrAdd(PlayerUnitId)=24;
    }
    else if(FParse::Param(FCommandLine::Get(),TEXT("SoulAlphaAttackProof")))
        SixFactionSaveSlot+=TEXT(".AttackProof");
    LastAIReport=TEXT("Four-faction alpha. Humans: your turn. Nature and Dark remain neutral to this conflict.");
}

bool USoulFounderPlaytestStateSubsystem::ValidateControlledRecruitment(FName Id,FName ArmyId,FName Region,int32 Quantity,FString& Error) const
{
    FSoulFactionCampaignState F;
    if(!IsInGameThread() || !bFourFactionAlpha || !bInitialized || bPersistenceBusy || HasPendingBattle()
        || !IsAlphaActiveFaction(Id) || !InspectFactionArmy(Id,F) || F.Army.ArmyId!=ArmyId || F.Army.RegionId!=Region)
    {Error=TEXT("REJECT_RECRUITMENT_STATE");return false;}
    const auto* R=World.Regions.Find(Region);
    if(!R || !R->bSettlement || R->OwnerFactionId!=Id || Quantity<1 || Quantity>4 || F.Army.TroopCount+Quantity>60)
    {Error=TEXT("REJECT_RECRUITMENT_LOCATION_OR_CAPACITY");return false;}
    auto Paid=F.Economy;
    if(!FSoulCampaignRules::SpendAction(Paid) || !FSoulCampaignRules::Recruit(Paid,F.Army.UnitId,Quantity))
    {Error=TEXT("REJECT_RECRUITMENT_RESOURCES_OR_POOL");return false;}
    Error.Reset();return true;
}

bool USoulFounderPlaytestStateSubsystem::PrepareControlledRecruitment(FName Id,int32 Quantity,FSoulControlledCampaignAction& Out,FString& Error) const
{
    Out={};FSoulFactionCampaignState F;
    if(!InspectFactionArmy(Id,F) || !ValidateControlledRecruitment(Id,F.Army.ArmyId,F.Army.RegionId,Quantity,Error))return false;
    FRBSaveDomainState S;if(!CaptureRBSaveDomain_Implementation(S,Error)||S.Fields.Num()!=1)return false;
    Out.FactionId=Id;Out.ArmyId=F.Army.ArmyId;Out.SourceRegion=Out.TargetRegion=F.Army.RegionId;
    Out.RecruitQuantity=Quantity;Out.ExpectedProfile=GetCampaignSaveSlotName();Out.ExpectedLoadRevision=CampaignLoadRevision;
    Out.ExpectedCampaignState=S.Fields[0].StringValue;return true;
}

bool USoulFounderPlaytestStateSubsystem::ExecuteControlledRecruitment(const FSoulControlledCampaignAction& A,FString& Error)
{
    if(A.ExpectedProfile!=GetCampaignSaveSlotName() || A.ExpectedLoadRevision!=CampaignLoadRevision)
    {Error=TEXT("REJECT_STALE_PROFILE_OR_LOAD");return false;}
    if(!ValidateControlledRecruitment(A.FactionId,A.ArmyId,A.SourceRegion,A.RecruitQuantity,Error))return false;
    FRBSaveDomainState S;if(!CaptureRBSaveDomain_Implementation(S,Error)||S.Fields.Num()!=1||S.Fields[0].StringValue!=A.ExpectedCampaignState)
    {Error=TEXT("REJECT_STALE_CAMPAIGN_STATE");return false;}
    FSoulFactionCampaignState F;InspectFactionArmy(A.FactionId,F);
    if(!FSoulCampaignRules::SpendAction(F.Economy)||!FSoulCampaignRules::Recruit(F.Economy,F.Army.UnitId,A.RecruitQuantity))return false;
    if(A.FactionId==PlayerFaction){Economy=MoveTemp(F.Economy);PlayerArmy.FindOrAdd(PlayerUnitId)+=A.RecruitQuantity;}
    else {auto& Live=OtherFactionStates.FindChecked(A.FactionId);Live.Economy=MoveTemp(F.Economy);Live.Army.TroopCount+=A.RecruitQuantity;}
    Error.Reset();return true;
}

void USoulFounderPlaytestStateSubsystem::RunNextAlphaAction()
{
    if(!bFourFactionAlpha || !IsAlphaTurnActive() || HasPendingBattle() || bPersistenceBusy)return;
    const FName Id=ActiveAI()[AlphaNextFaction];FSoulFactionCampaignState F;InspectFactionArmy(Id,F);
    FString Error,Verb=F.Army.TroopCount>0?TEXT("held position"):TEXT("field army destroyed");FSoulControlledCampaignAction Best;int32 BestScore=MIN_int32;
    auto Consider=[&](const FSoulControlledCampaignAction& A,int32 Score)
    {
        // Stable seed/day/name tie break; no hidden mutable random stream to lose on restore.
        const uint32 Tie=FCrc::StrCrc32(*(A.TargetRegion.ToString()+Id.ToString()+FString::FromInt(AlphaSeed)+FString::FromInt(Economy.Day)))%11;
        Score=Score*16+Tie;if(Score>BestScore){Best=A;BestScore=Score;}
    };
    if(F.Army.TroopCount<30)
    {
        const auto* Pool=F.Economy.RecruitmentPools.Find(F.Army.UnitId);
        int32 Count=Pool?FMath::Min(4,Pool->Available):0;
        FSoulControlledCampaignAction A;
        while(Count>0 && !PrepareControlledRecruitment(Id,Count,A,Error))--Count;
        if(Count>0)Consider(A,F.Army.TroopCount<18?350:130);
    }
    const auto* Here=World.Regions.Find(F.Army.RegionId);
    if(Here)
    {
        // Route planning uses explored topology, ownership only when currently visible.
        auto Distance=[&](FName Start,bool Supply)
        {
            TArray<FName> Queue{Start};TMap<FName,int32> D;D.Add(Start,0);
            for(int32 I=0;I<Queue.Num();++I)
            {
                FName R=Queue[I];const auto& Node=World.Regions.FindChecked(R);const int32 Depth=D[R];
                // Passive owners are static in this profile; explored passive land is not a frontier.
                if(!Node.OwnerFactionId.IsNone() && Node.OwnerFactionId!=Id && !IsAlphaActiveFaction(Node.OwnerFactionId))continue;
                if(Supply && Node.OwnerFactionId!=Id)continue;
                if(FSoulWorldRules::IsVisible(World,Id,R) && !Node.OwnerFactionId.IsNone() && Node.OwnerFactionId!=Id
                    && F.Army.TroopCount*100<ArmyCountAtRegion(R)*85)continue;
                // Own recruitment sites and unexplored frontiers are known without revealing hidden enemy forces.
                if(Supply && Node.OwnerFactionId==Id && Node.bSettlement)return Depth;
                if(!Supply && Node.Neighbors.ContainsByPredicate([&](FName N){return !FSoulWorldRules::IsExplored(World,Id,N);}))return Depth;
                if(FSoulWorldRules::IsVisible(World,Id,R))
                {
                    if(!Supply && (Node.OwnerFactionId.IsNone() || (Node.OwnerFactionId!=Id && IsAlphaActiveFaction(Node.OwnerFactionId))))return Depth;
                }
                TArray<FName> Ns=Node.Neighbors;Ns.Sort(FNameLexicalLess());
                for(FName N:Ns)
                {
                    // A proposed forward step must find its objective without first doubling back.
                    if(N==F.Army.RegionId || D.Contains(N)||!FSoulWorldRules::IsExplored(World,Id,N))continue;
                    const auto& Next=World.Regions.FindChecked(N);
                    if(FSoulWorldRules::IsVisible(World,Id,N) && !Next.OwnerFactionId.IsNone() && Next.OwnerFactionId!=Id && !IsAlphaActiveFaction(Next.OwnerFactionId))continue;
                    D.Add(N,Depth+1);Queue.Add(N);
                }
            }
            return 100;
        };
        bool CapitalThreat=false;
        if(const auto* Home=World.Regions.Find(Capital(Id)))for(FName N:Home->Neighbors)
            if(FSoulWorldRules::IsVisible(World,Id,N))
            {
                const auto& R=World.Regions.FindChecked(N);
                CapitalThreat|=!R.OwnerFactionId.IsNone()&&R.OwnerFactionId!=Id&&IsAlphaActiveFaction(R.OwnerFactionId)&&ArmyCountAtRegion(N)>0;
            }
        TArray<FName> Ns=Here->Neighbors;Ns.Sort(FNameLexicalLess());
        for(FName N:Ns)
        {
            if(!FSoulWorldRules::IsVisible(World,Id,N))continue;
            const auto& R=World.Regions.FindChecked(N);
            if(F.Army.TroopCount==0 && R.OwnerFactionId!=Id)continue; // Withdrawal cannot capture or initiate combat.
            if(!R.OwnerFactionId.IsNone()&&!IsAlphaActiveFaction(R.OwnerFactionId))continue;
            const bool Hostile=!R.OwnerFactionId.IsNone()&&R.OwnerFactionId!=Id;
            const int32 Defenders=Hostile?ArmyCountAtRegion(N):0;
            if(Hostile&&Defenders>0&&F.Army.TroopCount*100<Defenders*85)continue;
            FSoulControlledCampaignAction A;
            if(!PrepareControlledAction(Id,F.Army.ArmyId,F.Army.RegionId,N,A,Error))
            {UE_LOG(LogTemp,Display,TEXT("SOUL_ALPHA_REJECT day=%d faction=%s target=%s reason=%s"),Economy.Day,*Id.ToString(),*N.ToString(),*Error);continue;}
            int32 Score=Hostile?160:R.OwnerFactionId.IsNone()?100:30-Distance(N,false)*3;
            if(F.Army.TroopCount<18)Score=R.OwnerFactionId==Id?220-Distance(N,true)*10:Score-180;
            if(CapitalThreat&&N==Capital(Id))Score=300;
            // Avoid owned wandering when no known objective is reachable.
            if(!Hostile&&R.OwnerFactionId==Id&&Distance(N,F.Army.TroopCount<18)>=100)continue;
            Consider(A,Score);
        }
    }
    const FName PriorOwner=World.Regions.Contains(Best.TargetRegion)?World.Regions[Best.TargetRegion].OwnerFactionId:NAME_None;
    if(BestScore!=MIN_int32 && ExecuteControlledAction(Best,Error))
    {
        if(Best.RecruitQuantity>0)Verb=FString::Printf(TEXT("recruited %d infantry at %s"),Best.RecruitQuantity,*RegionDisplayNames.FindRef(Best.SourceRegion));
        else if(HasPendingBattle())Verb=TEXT("attacked ")+RegionDisplayNames.FindRef(Best.TargetRegion);
        else Verb=(F.Army.TroopCount==0?TEXT("withdrew to "):PriorOwner!=Id?TEXT("captured "):TEXT("moved to "))+RegionDisplayNames.FindRef(Best.TargetRegion);
    }
    else if(BestScore!=MIN_int32)Verb=TEXT("held: ")+Error;
    ++AlphaNextFaction;
    const FString Line=Id.ToString()+TEXT(": ")+Verb+TEXT(".");
    LastAIReport+=TEXT("\n")+Line;
    InspectFactionArmy(Id,F);
    UE_LOG(LogTemp,Display,TEXT("SOUL_ALPHA_ACTION day=%d faction=%s action=%s region=%s troops=%d gold=%d ap=%d cursor=%d"),
        Economy.Day,*Id.ToString(),*Verb,*F.Army.RegionId.ToString(),F.Army.TroopCount,F.Economy.Resources.FindRef(TEXT("gold")),F.Economy.ActionPoints,AlphaNextFaction);
}
