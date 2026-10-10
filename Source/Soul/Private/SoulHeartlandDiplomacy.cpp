#include "SoulFounderPlaytestStateSubsystem.h"
#include "Dom/JsonObject.h"

bool USoulFounderPlaytestStateSubsystem::IsDiplomacyTarget(FName F) const
{return bHeartlandEnabled&&(F==TEXT("dwarves")||F==TEXT("orcs")||F==TEXT("vikings"));}
FSoulDiplomaticRelation USoulFounderPlaytestStateSubsystem::DiplomaticRelation(FName F) const
{return HumanRelations.FindRef(F);}
bool USoulFounderPlaytestStateSubsystem::DiplomacyAllowsHostility(FName A,FName D) const
{
 if(!bHeartlandEnabled||D.IsNone()||A==D)return true;
 const FName Other=A==PlayerFaction?D:D==PlayerFaction?A:NAME_None;
 return !IsDiplomacyTarget(Other)||FSoulDiplomacyRules::EffectiveStance(DiplomaticRelation(Other),Economy.Day)==ESoulDiplomaticStance::War;
}
FSoulDiplomaticDecision USoulFounderPlaytestStateSubsystem::PreviewDiplomacy(FName F,ESoulDiplomaticAction A) const
{
 FSoulDiplomaticDecision D;
 auto Reject=[&](const TCHAR* Why){D.Reasons.Add(Why);return D;};
 if(!IsInGameThread())return Reject(TEXT("Diplomacy must execute on the game thread."));
 if(!bInitialized||!IsDiplomacyTarget(F)||!OtherFactionStates.Contains(F))return Reject(TEXT("Diplomacy V0 supports Dwarves, Orcs and Vikings in Heartland."));
 if(bPersistenceBusy||HasPendingBattle()||IsAlphaTurnActive())return Reject(TEXT("Wait for the current battle, AI turn or save/load."));
 if(!FSoulHeroRules::CanPerformDiplomacy(Hero))return Reject(TEXT("A wounded or captured hero cannot conduct diplomacy."));
 if(Economy.ActionPoints<1)return Reject(TEXT("Diplomacy requires 1 movement."));
 if(A==ESoulDiplomaticAction::Gift&&(Economy.Resources.FindRef(TEXT("gold"))<250||OtherFactionStates.FindChecked(F).Economy.Resources.FindRef(TEXT("gold"))>MAX_int32-250))return Reject(TEXT("Gift requires 250 real gold and a valid recipient treasury."));
 return FSoulDiplomacyRules::Evaluate(DiplomaticRelation(F),A,Economy.Day);
}
bool USoulFounderPlaytestStateSubsystem::ExecuteDiplomacy(FName F,ESoulDiplomaticAction A,FString& Message)
{
 const auto D=PreviewDiplomacy(F,A);Message=FString::Join(D.Reasons,TEXT(" "));
 if(!D.bAccepted)return false;
 auto Relation=DiplomaticRelation(F);auto Paid=Economy;
 if(!FSoulDiplomacyRules::Apply(Relation,A,Economy.Day)||!FSoulCampaignRules::SpendAction(Paid))return false;
 if(A==ESoulDiplomaticAction::Gift)Paid.Resources.FindOrAdd(TEXT("gold"))-=250;
 // All admission is complete before the existing campaign state changes.
 Economy=MoveTemp(Paid);HumanRelations.Add(F,Relation);
 if(A==ESoulDiplomaticAction::Gift)OtherFactionStates.FindChecked(F).Economy.Resources.FindOrAdd(TEXT("gold"))+=250;
 Message=FString::Printf(TEXT("%s: %s; relations %+d. %s"),*F.ToString(),*FSoulDiplomacyRules::StanceName(FSoulDiplomacyRules::EffectiveStance(Relation,Economy.Day)),Relation.RelationPermille,*Message);
 UE_LOG(LogTemp,Display,TEXT("SOUL_DIPLOMACY action=%d faction=%s stance=%d relation=%d day=%d ap=%d gold=%d"),static_cast<int32>(A),*F.ToString(),static_cast<int32>(Relation.Stance),Relation.RelationPermille,Economy.Day,Economy.ActionPoints,Economy.Resources.FindRef(TEXT("gold")));
 return true;
}

void USoulFounderPlaytestStateSubsystem::CaptureDiplomacy(FJsonObject& Root) const
{
 // No field before the first interaction: legacy Heartland checkpoints keep their exact default state.
 if(!bHeartlandEnabled||HumanRelations.IsEmpty())return;
 auto Rows=MakeShared<FJsonObject>();TArray<FName> Keys;HumanRelations.GetKeys(Keys);Keys.Sort(FNameLexicalLess());
 for(FName Key:Keys){const auto& R=HumanRelations[Key];auto V=MakeShared<FJsonObject>();
  V->SetNumberField(TEXT("stance"),static_cast<int32>(R.Stance));V->SetNumberField(TEXT("relation"),R.RelationPermille);
  V->SetNumberField(TEXT("pact_until"),R.PactUntilDay);V->SetNumberField(TEXT("betrayal_day"),R.BetrayalDay);V->SetNumberField(TEXT("last_action_day"),R.LastActionDay);
  Rows->SetObjectField(Key.ToString(),V);}
 Root.SetObjectField(TEXT("heartland_diplomacy"),Rows);
}
bool USoulFounderPlaytestStateSubsystem::ValidateDiplomacy(const FJsonObject& Root,int32 Day,TMap<FName,FSoulDiplomaticRelation>& Out,FString& Error) const
{
 Out.Reset();if(!Root.HasField(TEXT("heartland_diplomacy")))return true;
 const TSharedPtr<FJsonObject>* Rows=nullptr;
 if(!bHeartlandEnabled||!Root.TryGetObjectField(TEXT("heartland_diplomacy"),Rows)||(*Rows)->Values.Num()>3){Error=TEXT("Invalid diplomacy domain data.");return false;}
 auto Int=[](const FJsonObject& O,const TCHAR* K,int32 Min,int32 Max,int32& V){double N=0;if(!O.TryGetNumberField(K,N)||!FMath::IsFinite(N)||N<Min||N>Max||FMath::FloorToDouble(N)!=N)return false;V=static_cast<int32>(N);return true;};
 for(const auto& P:(*Rows)->Values)
 {
  const FName Id(*P.Key);const TSharedPtr<FJsonObject>* V=nullptr;FSoulDiplomaticRelation R;int32 Stance=0;
  if(!IsDiplomacyTarget(Id)||!P.Value.IsValid()||!P.Value->TryGetObject(V)||(*V)->Values.Num()!=5
   ||!Int(**V,TEXT("stance"),0,2,Stance)||!Int(**V,TEXT("relation"),-1000,1000,R.RelationPermille)
   ||!Int(**V,TEXT("pact_until"),0,1000003,R.PactUntilDay)||!Int(**V,TEXT("betrayal_day"),0,1000000,R.BetrayalDay)
   ||!Int(**V,TEXT("last_action_day"),0,1000000,R.LastActionDay)){Error=TEXT("Malformed diplomacy relation.");return false;}
  R.Stance=static_cast<ESoulDiplomaticStance>(Stance);
  if(!FSoulDiplomacyRules::Valid(R,Day)){Error=TEXT("Invalid diplomacy dates or stance.");return false;}
  Out.Add(Id,R);
 }
 return true;
}
