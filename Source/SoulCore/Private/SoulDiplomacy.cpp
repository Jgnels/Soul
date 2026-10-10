#include "SoulDiplomacy.h"
ESoulDiplomaticStance FSoulDiplomacyRules::EffectiveStance(const FSoulDiplomaticRelation& R,int32 Day)
{return R.Stance==ESoulDiplomaticStance::NonAggression&&Day>R.PactUntilDay?ESoulDiplomaticStance::Peace:R.Stance;}
bool FSoulDiplomacyRules::Valid(const FSoulDiplomaticRelation& R,int32 Day)
{
 return Day>0&&Day<=1000000&&static_cast<uint8>(R.Stance)<=2&&R.RelationPermille>=-1000&&R.RelationPermille<=1000
  &&R.PactUntilDay>=0&&R.PactUntilDay<=Day+3&&R.BetrayalDay>=0&&R.BetrayalDay<=Day
  &&R.LastActionDay>=0&&R.LastActionDay<=Day
  &&(R.Stance==ESoulDiplomaticStance::NonAggression?R.PactUntilDay>0:R.PactUntilDay==0);
}
FString FSoulDiplomacyRules::StanceName(ESoulDiplomaticStance S)
{return S==ESoulDiplomaticStance::War?TEXT("WAR"):S==ESoulDiplomaticStance::Peace?TEXT("PEACE"):TEXT("NON-AGGRESSION");}
FSoulDiplomaticDecision FSoulDiplomacyRules::Evaluate(const FSoulDiplomaticRelation& R,ESoulDiplomaticAction A,int32 Day)
{
 FSoulDiplomaticDecision D;
 if(!Valid(R,Day)||static_cast<uint8>(A)>4){D.Reasons.Add(TEXT("Invalid diplomatic state or action."));return D;}
 const auto S=EffectiveStance(R,Day);
 auto Reject=[&](const TCHAR* Why){D.Reasons.Add(Why);return D;};
 if(A==ESoulDiplomaticAction::Peace&&S!=ESoulDiplomaticStance::War)return Reject(TEXT("Already at peace."));
 if(A==ESoulDiplomaticAction::NonAggression&&S!=ESoulDiplomaticStance::Peace)return Reject(TEXT("A non-aggression pact requires peace first."));
 if(A==ESoulDiplomaticAction::DeclareWar&&S==ESoulDiplomaticStance::War)return Reject(TEXT("Already at war."));
 if(A==ESoulDiplomaticAction::DeclareWar&&S==ESoulDiplomaticStance::NonAggression)return Reject(TEXT("Active pact: use Break treaty; betrayal is recorded."));
 if(A==ESoulDiplomaticAction::BreakTreaty&&S!=ESoulDiplomaticStance::NonAggression)return Reject(TEXT("There is no active pact to break."));
 if(A==ESoulDiplomaticAction::Peace||A==ESoulDiplomaticAction::NonAggression)
 {
  // Fixed-scale, explainable V0. No hidden army/resource information or random roll.
  D.Components.Add(TEXT("base"),400);
  D.Components.Add(TEXT("relation"),R.RelationPermille/2);
  const int32 Memory=R.BetrayalDay>0?-FMath::Max(0,250-FMath::Min(10,Day-R.BetrayalDay)*25):0;
  D.Components.Add(TEXT("recent_betrayal"),Memory);
  D.ScorePermille=FMath::Clamp(400+R.RelationPermille/2+Memory,0,1000);
  D.bAccepted=D.ScorePermille>=500;
  D.Reasons.Add(FString::Printf(TEXT("Base 400; relation %+d; recent betrayal %+d. Score %d / 1000; needs 500."),R.RelationPermille/2,Memory,D.ScorePermille));
  D.Reasons.Add(D.bAccepted?TEXT("Terms accepted; no military access is granted."):TEXT("Terms refused; improve relations. No resources or movement spent."));
 }
 else
 {
  D.bAccepted=true;D.ScorePermille=1000;
  D.Reasons.Add(A==ESoulDiplomaticAction::Gift?TEXT("Transfer 250 gold to this faction; relations +100. Costs 1 movement."):
   A==ESoulDiplomaticAction::BreakTreaty?TEXT("Break the pact and declare war. Relations -300; betrayal memory decays over ten days. Costs 1 movement."):
   TEXT("Declare war. Relations -100. Costs 1 movement."));
 }
 return D;
}
bool FSoulDiplomacyRules::Apply(FSoulDiplomaticRelation& R,ESoulDiplomaticAction A,int32 Day)
{
 if(!Evaluate(R,A,Day).bAccepted)return false;
 R.Stance=EffectiveStance(R,Day);if(R.Stance!=ESoulDiplomaticStance::NonAggression)R.PactUntilDay=0;
 switch(A)
 {
 case ESoulDiplomaticAction::Gift:R.RelationPermille=FMath::Min(1000,R.RelationPermille+100);break;
 case ESoulDiplomaticAction::Peace:R.Stance=ESoulDiplomaticStance::Peace;R.PactUntilDay=0;break;
 case ESoulDiplomaticAction::NonAggression:R.Stance=ESoulDiplomaticStance::NonAggression;R.PactUntilDay=Day+3;break;
 case ESoulDiplomaticAction::DeclareWar:R.Stance=ESoulDiplomaticStance::War;R.PactUntilDay=0;R.RelationPermille=FMath::Max(-1000,R.RelationPermille-100);break;
 case ESoulDiplomaticAction::BreakTreaty:R.Stance=ESoulDiplomaticStance::War;R.PactUntilDay=0;R.RelationPermille=FMath::Max(-1000,R.RelationPermille-300);R.BetrayalDay=Day;break;
 }
 R.LastActionDay=Day;return true;
}
