#include "SoulRealtimeBattleArena.h"
#include "SoulCampaignBattleBridge.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Animation/AnimationAsset.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "InputKeyEventArgs.h"

bool ASoulRealtimeArenaGameMode::IsSiegeGate(FRBHostIdentity I) const
{return bSiege&&SiegeGateId.IsValid()&&I.Domain==TEXT("Soul.Battle")&&I.Id==SiegeGateId;}
bool ASoulRealtimeArenaGameMode::CanDamageSiegeGate(FRBHostIdentity I) const
{return IsSiegeGate(I)&&!bFinished&&!bBattlePaused&&SiegeState.GateIntegrityPermille>0;}

bool ASoulRealtimeArenaGameMode::SetupSiege()
{
 UStaticMeshComponent* Gate=nullptr;
 for(TActorIterator<AActor> It(GetWorld());It;++It)
 {
  TArray<UStaticMeshComponent*> Parts;It->GetComponents(Parts);
  for(auto* Part:Parts)if(Part->GetStaticMesh()&&Part->GetStaticMesh()->GetPathName()==TEXT("/Game/CastleTown/Static_Mesh/Castle_Kit/Buildings/Castle_Gate/SM_Portcullis.SM_Portcullis"))
  {if(Gate){UE_LOG(LogTemp,Error,TEXT("SOUL_SIEGE_SETUP_FAIL ambiguous native gate"));return false;}Gate=Part;SiegeGateActor=*It;}
 }
 if(!Gate){UE_LOG(LogTemp,Error,TEXT("SOUL_SIEGE_SETUP_FAIL native gate missing"));return false;}
 SiegeGateBase=Gate->GetComponentTransform().TransformPosition(FVector(201,0,0));
 SiegeForward=-Gate->GetRightVector().GetSafeNormal2D();SiegeSide=Gate->GetForwardVector().GetSafeNormal2D();
 if(!SiegeGateBase.Equals(ArenaOrigin,60)){UE_LOG(LogTemp,Error,TEXT("SOUL_SIEGE_SETUP_FAIL gate moved %s expected %s"),*SiegeGateBase.ToCompactString(),*ArenaOrigin.ToCompactString());return false;}
 FHitResult Floor;FCollisionQueryParams FloorQuery(SCENE_QUERY_STAT(SoulSiegeGateFloor),false,SiegeGateActor);
 if(!GetWorld()->LineTraceSingleByChannel(Floor,SiegeGateBase+FVector(0,0,100),SiegeGateBase-FVector(0,0,700),ECC_Visibility,FloorQuery)||Floor.ImpactNormal.Z<.7f)return false;
 const double Drop=Floor.ImpactPoint.Z+2-SiegeGateBase.Z;
 if(Drop>20||Drop< -350){UE_LOG(LogTemp,Error,TEXT("SOUL_SIEGE_SETUP_FAIL unreviewed threshold drop %.1f"),Drop);return false;}
 Gate->SetMobility(EComponentMobility::Movable);SiegeGateActor->AddActorWorldOffset(FVector(0,0,Drop),false);
 SiegeGateBase.Z+=Drop;
 SiegeObjective=SiegeGateBase+SiegeForward*950;
 // Ground-relative human capsule survey ignores only the gate, never walls.
 int32 RouteMisses=0,RouteBlocks=0;FVector Previous=FVector::ZeroVector;
 FCollisionObjectQueryParams GroundObjects;GroundObjects.AddObjectTypesToQuery(ECC_WorldStatic);
 for(int32 D=-1800;D<=1500;D+=100)
 {
  const FVector P=SiegeGateBase+SiegeForward*D;FHitResult Ground;
  if(!GetWorld()->LineTraceSingleByObjectType(Ground,FVector(P.X,P.Y,SiegeGateBase.Z+180),P-FVector(0,0,1000),GroundObjects,FloorQuery)){++RouteMisses;continue;}
  const FVector At=Ground.ImpactPoint+FVector(0,0,110);
  if(D>-1800)
  {
   FHitResult Block;
   if(GetWorld()->SweepSingleByChannel(Block,Previous,At,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(40,90),FloorQuery))
   {++RouteBlocks;UE_LOG(LogTemp,Warning,TEXT("SOUL_SIEGE_ROUTE_BLOCK d=%d actor=%s component=%s point=%s"),D,*GetNameSafe(Block.GetActor()),*GetNameSafe(Block.GetComponent()),*Block.ImpactPoint.ToCompactString());}
  }
  Previous=At;
 }
 UE_LOG(LogTemp,Display,TEXT("SOUL_SIEGE_ROUTE_SURVEY misses=%d blocks=%d gate_ignored_only=1"),RouteMisses,RouteBlocks);
 if(RouteMisses||RouteBlocks)return false;

 SiegeGateId=FGuid::NewGuid();SiegeGateActor->SetCanBeDamaged(true);
 SiegeGateBinding=NewObject<USoulRealtimeArenaBinding>(SiegeGateActor);SiegeGateActor->AddInstanceComponent(SiegeGateBinding);SiegeGateBinding->RegisterComponent();
 FRBHostIdentity Identity;Identity.Domain=TEXT("Soul.Battle");Identity.Id=SiegeGateId;
 if(!SiegeGateBinding->BindCombatant(Identity.Core()))return false;
 Gate->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Gate->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block);
 if(SiegeState.GateIntegrityPermille>0)
 {
  FHitResult Closed;FCollisionQueryParams Q(SCENE_QUERY_STAT(SoulSiegeClosedGate),false);
  const FVector Center=SiegeGateBase+FVector(0,0,110);
  const bool Blocks=GetWorld()->SweepSingleByChannel(Closed,Center-SiegeForward*220,Center+SiegeForward*220,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(40,90),Q)&&Closed.GetActor()==SiegeGateActor;
  UE_LOG(LogTemp,Display,TEXT("SOUL_SIEGE_CLOSED_GATE human_capsule_blocked=%d actor=%s"),Blocks,*GetNameSafe(Closed.GetActor()));
  if(!Blocks)return false;
 }
 if(SiegeState.GateIntegrityPermille==0){SiegeState.bGatehouseActive=false;FSoulSiegeRules::OpenBreach(SiegeState);OpenSiegeGate();}
 UE_LOG(LogTemp,Display,TEXT("SOUL_SIEGE_SETUP native=%s gate=%s forward=%s objective=%s integrity=%d/%d donor_writes=0"),*Gate->GetStaticMesh()->GetPathName(),*SiegeGateBase.ToCompactString(),*SiegeForward.ToCompactString(),*SiegeObjective.ToCompactString(),SiegeState.GateIntegrityPermille,SiegeState.GateMaximumIntegrity);
 return true;
}

FVector ASoulRealtimeArenaGameMode::SiegeDeployment(int32 Side,int32 Group) const
{
 // Assemble attackers on the measured solid approach, before its bridge joints.
 // The old rear slot intersected a small authored deck gap and seated a soldier
 // on water below it. No city geometry or collision is removed to avoid that gap.
 const float Along=Side==0?-(450.f+Group*300.f):(800.f+Group*420.f);
 return SiegeGateBase+SiegeForward*Along+FVector(0,0,100);
}

void ASoulRealtimeArenaGameMode::OpenSiegeGate()
{
 if(!SiegeGateActor||SiegeState.GateIntegrityPermille>0)return;
 // Only the blocking native portcullis is removed in this transient siege world.
 // Its surrounding gatehouse, walls, collision and donor packages remain intact.
 SiegeGateActor->SetActorEnableCollision(false);SiegeGateActor->SetActorHiddenInGame(true);
 PushBattleNotice(TEXT("GATE BREACHED - enter the courtyard. Breach alone does not win."),INDEX_NONE);
 UE_LOG(LogTemp,Display,TEXT("SOUL_SIEGE_BREACH hits=%d victory=%d"),SiegeHitCount,SiegeState.bVictory);
}

bool ASoulRealtimeArenaGameMode::CommitSiegeHit(const FRBHostHit& H,FString& Error)
{
 const int32 A=Index(H.Attacker);
 if(!CanDamageSiegeGate(H.Victim)||!Combatants.IsValidIndex(A)||Combatants[A].Side!=0||Combatants[A].Health<=0||Combatants[A].bRanged||!Actors[A]
  ||!H.ContactId.IsValid()||AcceptedContacts.Contains(H.ContactId)||!H.bHasExactImpact||!FMath::IsFinite(H.AcceptedDamage)||H.AcceptedDamage<=0
  ||FVector::Dist2D(Actors[A]->GetActorLocation(),H.ImpactPoint)>Profile(A).Reach+100)
 {Error=TEXT("Rejected invalid siege contact");return false;}
 if(!FSoulSiegeRules::ApplyGateDamage(SiegeState,FMath::Max(1,FMath::RoundToInt(H.AcceptedDamage))))return false;
 AcceptedContacts.Add(H.ContactId);++SiegeHitCount;
 PlayBattleSound(ESoulBattleSound::SwordHit2,H.ImpactPoint,A);
 Status=FString::Printf(TEXT("Gate struck: %d / %d integrity"),SiegeState.GateIntegrityPermille,SiegeState.GateMaximumIntegrity);
 UE_LOG(LogTemp,Display,TEXT("SOUL_SIEGE_GATE_HIT RBCombat=1 hero=%d integrity=%d/%d contact=%s"),Combatants[A].bPlayerHero,SiegeState.GateIntegrityPermille,SiegeState.GateMaximumIntegrity,*H.ContactId.ToString());
 if(SiegeState.GateIntegrityPermille==0)OpenSiegeGate();
 Error.Reset();return true;
}

bool ASoulRealtimeArenaGameMode::PerformGateMelee(int32 I)
{
 if(!bSiege||bFinished||bBattlePaused||SiegeState.GateIntegrityPermille<=0||!Combatants.IsValidIndex(I)||!Actors[I])return false;
 auto& C=Combatants[I];auto* A=Actors[I].Get();
 if(C.Side!=0||C.Health<=0||C.bRanged||C.MeleeCooldown>0||FVector::Dist2D(A->GetActorLocation(),SiegeGateBase)>Profile(I).Reach+90)return false;
 auto* Clip=bVisualUnits?ResolveVisualAttack(C.Side,C.Role,I+C.VisualAttackSequence++):nullptr;
 const float Duration=Clip?FMath::Clamp(Clip->GetPlayLength(),.8f,2.8f):1.5f;
 Bindings[I]->SetGuardIntent(false);C.MeleeCooldown=Duration;C.PendingMeleeSeconds=Duration*.35f;
 C.PendingMeleeTarget.Domain=TEXT("Soul.Battle");C.PendingMeleeTarget.Id=SiegeGateId;
 A->SetActorRotation((SiegeGateBase-A->GetActorLocation()).GetSafeNormal2D().Rotation());A->GetCharacterMovement()->StopMovementImmediately();
 PlayBattleSound(ESoulBattleSound::SwordSwing1,A->GetActorLocation(),I);
 if(Clip){A->GetMesh()->PlayAnimation(Clip,false);A->GetMesh()->SetPlayRate(Clip->GetPlayLength()/Duration);C.bVisualAttackPlaying=true;}
 return true;
}

bool ASoulRealtimeArenaGameMode::CommitGateMelee(int32 I)
{
 if(!Combatants.IsValidIndex(I)||!Actors[I]||Combatants[I].Health<=0||Combatants[I].Side!=0||bBattlePaused||SiegeState.GateIntegrityPermille<=0||!SiegeGateBinding)return false;
 auto* A=Actors[I].Get();const FVector Direction=(SiegeGateBase-A->GetActorLocation()).GetSafeNormal2D();
 const auto Weapon=Profile(I);FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(SoulSiegeMelee),false,A);
 const FVector Start=A->GetActorLocation()+Direction*35+FVector(0,0,35);
 if(!GetWorld()->SweepSingleByChannel(Hit,Start,Start+Direction*(Weapon.Reach+85),FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(18),Q)||Hit.GetActor()!=SiegeGateActor)return false;
 FRBCombatHit E;E.ContactId=Bindings[I]->BeginNativeAttackContact();E.Attacker=Bindings[I]->GetCombatant();E.Victim=SiegeGateBinding->GetCombatant();E.Weapon=Weapon;E.AcceptedDamage=Weapon.BaseDamage;E.ImpactPoint=Hit.ImpactPoint;E.bHasExactImpact=true;
 FString Error;const bool Accepted=SiegeGateBinding->ReceiveProducedImpact(E,Hit,Direction,A,Error);Bindings[I]->EndNativeAttackContact();
 if(!Accepted)UE_LOG(LogTemp,Warning,TEXT("SOUL_SIEGE_CONTACT_REJECT %s"),*Error);
 return Accepted;
}

bool ASoulRealtimeArenaGameMode::SiegeDriveGroup(int32 G,FRBCombatGroup& Out) const
{
 if(!bSiege||Out.Members.IsEmpty())return true;
 const int32 I=Index(FRBHostIdentity::From(Out.Members[0]));if(!Combatants.IsValidIndex(I))return false;
 const int32 Side=Combatants[I].Side;const FVector Center=GroupCenter(G);const float Along=FVector::DotProduct(Center-SiegeGateBase,SiegeForward);
 const bool Closed=SiegeState.GateIntegrityPermille>0;
 const int32 State=FindFormationState(G);
 if(Out.Command!=ERBGroupCommand::Advance&&Out.Command!=ERBGroupCommand::Charge)return true;
 if(Closed&&Side==1)
 {
  // A defender may reposition behind the wall; only an order through the closed
  // gate is held at the inner defense line. Never erase a valid interior MOVE.
  if(Out.Command==ERBGroupCommand::Charge||FVector::DotProduct(Out.Anchor-SiegeGateBase,SiegeForward)<120)
  {Out.Command=ERBGroupCommand::Hold;Out.Anchor=TacticalFormations.IsValidIndex(State)?TacticalFormations[State].SpawnAnchor:Groups[G].Anchor;}
  return true;
 }
 const bool Manual=TacticalFormations.IsValidIndex(State)&&TacticalFormations[State].ManualOverrideUntil>BattleElapsed;
 const bool Assault=Side==0&&(Out.Command==ERBGroupCommand::Charge||!Manual);
 if(Assault||((Along<0)!=(FVector::DotProduct(Out.Anchor-SiegeGateBase,SiegeForward)<0)))
 {
  FVector Waypoint;
  if(Closed){if(Side!=0)return true;Waypoint=SiegeGateBase-SiegeForward*125;}
  // The target must clear the aperture by more than a formation row plus the
  // native arrival radius. A target before the opening can strand its centroid.
  else if(Assault)Waypoint=Along<150?SiegeGateBase+SiegeForward*400:SiegeObjective;
  else Waypoint=SiegeGateBase+SiegeForward*(Along<0?400.f:-400.f);
  // Once inside, restore native Charge pursuit so soldiers fight defenders
  // rather than stopping at a formation slot just outside melee reach.
  Out.Command=!Closed&&Assault&&Along>=150?ERBGroupCommand::Charge:ERBGroupCommand::Advance;
  Out.Anchor=Waypoint+FVector(0,0,96);Out.Facing=(Waypoint-Center).GetSafeNormal2D();
 }
 return true;
}

void ASoulRealtimeArenaGameMode::TickSiege(float Seconds)
{
 if(!bSiege||bFinished||bBattlePaused)return;
 for(int32 I=0;I<Combatants.Num();++I)
 {
  const auto& C=Combatants[I];if(C.Health<=0||!Actors[I])continue;
  if(!C.bPlayerHero)
  {
   const FVector Offset=Actors[I]->GetActorLocation()-SiegeGateBase;
   const float Along=FVector::DotProduct(Offset,SiegeForward);
   // Broad-field reciprocal avoidance can deadlock opposing ranks at an
   // aperture. In the measured gate corridor, native capsule/world collision
   // supplies separation; normal avoidance resumes outside it.
   const bool Avoid=!(Along> -650&&Along<1000&&FMath::Abs(FVector::DotProduct(Offset,SiegeSide))<450);
   auto* Movement=Actors[I]->GetCharacterMovement();
   if(bool(Movement->bUseRVOAvoidance)!=Avoid)Movement->SetAvoidanceEnabled(Avoid);
  }
  if(C.Side==0&&!C.bPlayerHero&&SiegeState.GateIntegrityPermille>0)
  {
   const int32 S=FindFormationState(C.GroupIndex);const bool Manual=TacticalFormations.IsValidIndex(S)&&TacticalFormations[S].ManualOverrideUntil>BattleElapsed;
   if(!Manual||TacticalFormations[S].ManualOrder==ERBHostGroupOrder::Charge)PerformGateMelee(I);
  }
  if(C.Side==0&&!C.bPlayerHero&&FMath::Abs(Actors[I]->GetActorLocation().Z-(SiegeGateBase.Z+90))<150&&SiegeState.GateIntegrityPermille==0&&FVector::DotProduct(Actors[I]->GetActorLocation()-SiegeGateBase,SiegeForward)>400&&!bSiegeTroopsEntered)
  {bSiegeTroopsEntered=true;UE_LOG(LogTemp,Display,TEXT("SOUL_SIEGE_TROOPS_TRAVERSED actor=%s company=%s physical=%s"),*Actors[I]->GetName(),*C.CampaignCompany.ToString(),*Actors[I]->GetActorLocation().ToCompactString());}
  if(C.Side==0&&FMath::Abs(Actors[I]->GetActorLocation().Z-(SiegeGateBase.Z+90))<150&&SiegeState.GateIntegrityPermille==0&&FVector::DotProduct(Actors[I]->GetActorLocation()-SiegeGateBase,SiegeForward)>400&&!bSiegeEntered)
  {bSiegeEntered=true;UE_LOG(LogTemp,Display,TEXT("SOUL_SIEGE_TRAVERSED actor=%s hero=%d physical=%s"),*Actors[I]->GetName(),C.bPlayerHero,*Actors[I]->GetActorLocation().ToCompactString());}
 }
 int32 Attackers=0,Defenders=0;
 for(int32 I=0;I<Combatants.Num();++I)if(Combatants[I].Health>0&&Actors[I]&&FVector::Dist2D(Actors[I]->GetActorLocation(),SiegeObjective)<420&&FMath::Abs(Actors[I]->GetActorLocation().Z-(SiegeGateBase.Z+90))<150)
  (Combatants[I].Side==0?Attackers:Defenders)++;
 if(FParse::Param(FCommandLine::Get(),TEXT("SoulSiegeQualification"))&&BattleElapsed>=SiegeReportNext)
 {
  SiegeReportNext=BattleElapsed+10;
  UE_LOG(LogTemp,Display,TEXT("SOUL_SIEGE_PROGRESS seconds=%.1f alive=%d/%d reserve=%d/%d courtyard=%d/%d control=%d gate=%d hero=%.0f"),BattleElapsed,AliveForSide(0),AliveForSide(1),ReserveBodiesForSide(0),ReserveBodiesForSide(1),Attackers,Defenders,SiegeState.CourtyardControlMillis,SiegeState.GateIntegrityPermille,PlayerHealth());
  for(const auto& F:TacticalFormations)if(AliveInGroup(F.GroupIndex)>0)
   UE_LOG(LogTemp,Display,TEXT("SOUL_SIEGE_GROUP g=%d side=%d alive=%d center=%s order=%d"),F.GroupIndex,F.Side,AliveInGroup(F.GroupIndex),*GroupCenter(F.GroupIndex).ToCompactString(),int32(Groups[F.GroupIndex].Order));
 }
 if(FSoulSiegeRules::AdvanceCourtyard(SiegeState,FMath::Clamp(FMath::RoundToInt(Seconds*1000),1,1000),Attackers,Defenders)&&AliveForSide(0)>0)
 {
  // Capture is a real physical displacement objective. Existing rout semantics
  // remove defeated strategic cohesion; no synthetic casualties are reported as kills.
  bSideMoraleDefeated[1]=true;UE_LOG(LogTemp,Display,TEXT("SOUL_SIEGE_OBJECTIVE_CAPTURE attackers=%d defenders=%d physicalDefenders=%d"),Attackers,Defenders,AliveForSide(1));FinishBattle();
 }
}

void ASoulRealtimeArenaGameMode::TickSiegeQualification()
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("SoulSiegeQualification"))||bFinished)return;
 auto* PC=GetWorld()->GetFirstPlayerController();if(!PC||!PlayerHero)return;
 if(bBattlePaused)ToggleBattlePause();
 auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/Name,true,false);};
 if(SiegeProofStage==0)
 {
  HandleGamepadAction(TEXT("All"));HandleBattleAction(TEXT("Hold"));
  if(!bTacticalCameraActive)ToggleBattleCamera();
  TacticalFocus=SiegeGateBase-SiegeForward*200+FVector(0,0,100);TacticalDistance=2700;TacticalRotation=FRotator(-35,SiegeForward.Rotation().Yaw,0);
  SiegeProofStage=1;return;
 }
 if(SiegeProofStage==1&&BattleElapsed>4){Shot(TEXT("Siege_Outside_Intact.png"));SiegeProofStage=2;return;}
 if(SiegeProofStage==2&&BattleElapsed>5){TacticalFocus=SiegeGateBase+SiegeForward*350+FVector(0,0,180);TacticalDistance=1600;TacticalRotation=FRotator(-24,SiegeForward.Rotation().Yaw+180,0);SiegeProofStage=3;return;}
 if(SiegeProofStage==3&&BattleElapsed>7){Shot(TEXT("Siege_Defenders_Inside.png"));SiegeProofStage=4;return;}
 if(SiegeProofStage==4&&BattleElapsed>8)
 {HandleBattleAction(TEXT("Charge"));if(bTacticalCameraActive)ToggleBattleCamera();if(!bFirstPersonCamera)ToggleFirstPersonCamera();SiegeProofStage=5;return;}
 // Qualification cameras observe troop/objective progress even if Aurora falls.
 // They never change actors, combat health, ownership or the victory result.
 if(SiegeProofStage>=5&&(bSiegeTroopsEntered||PlayerHealth()<=0)&&SiegeProofStage<11)
 {
  if(!bTacticalCameraActive)ToggleBattleCamera();
  TacticalFocus=SiegeGateBase+SiegeForward*550+FVector(0,0,100);TacticalDistance=1900;
  TacticalRotation=FRotator(-48,SiegeForward.Rotation().Yaw+100,0);
  SiegeProofStage=11;SiegeProofNext=BattleElapsed+2;return;
 }
 if(SiegeProofStage==11){if(BattleElapsed<SiegeProofNext)return;Shot(TEXT("Siege_Gate_Combat.png"));SiegeProofStage=12;}
 if(SiegeProofStage==12&&bSiegeTroopsEntered){Shot(TEXT("Siege_Troops_Through_Gate.png"));SiegeProofStage=13;}
 if(SiegeProofStage==13&&SiegeState.CourtyardControlMillis>1000){Shot(TEXT("Siege_Courtyard_Control.png"));SiegeProofStage=14;}
 if(SiegeProofStage<5||PlayerHealth()<=0)return;
 if(SiegeProofStage==5&&BattleElapsed>10){Shot(TEXT("Siege_Hero_Intact.png"));SiegeProofStage=6;}
 FVector Goal=SiegeState.GateIntegrityPermille>0?SiegeGateBase-SiegeForward*130:SiegeObjective;
 if(SiegeState.GateIntegrityPermille==0)
 {
  const float Along=FVector::DotProduct(PlayerHero->GetActorLocation()-SiegeGateBase,SiegeForward);
  if(Along<150)Goal=SiegeGateBase+SiegeForward*400;
 }
 const FVector Direction=(Goal-PlayerHero->GetActorLocation()).GetSafeNormal2D();PC->SetControlRotation(Direction.Rotation());PlayerHero->SetActorRotation(Direction.Rotation());
 if(FVector::Dist2D(PlayerHero->GetActorLocation(),Goal)>80)PlayerHero->AddMovementInput(Direction,1);
 if(BattleElapsed>SiegeProofNext){SiegeProofNext=BattleElapsed+1.6;PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton,IE_Pressed,1));PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton,IE_Released,0));}
 if(SiegeProofStage==6&&SiegeState.GateIntegrityPermille<SiegeState.GateMaximumIntegrity*.75&&SiegeState.GateIntegrityPermille>0){SiegeProofStage=7;Shot(TEXT("Siege_Damaged.png"));}
 if(SiegeProofStage<8&&SiegeState.GateIntegrityPermille==0){SiegeProofStage=8;Shot(TEXT("Siege_Breached.png"));}
 if(SiegeProofStage==8&&bSiegeEntered){SiegeProofStage=9;Shot(TEXT("Siege_Inside_Combat.png"));}
 if(SiegeProofStage==9&&SiegeState.CourtyardControlMillis>1000){SiegeProofStage=10;Shot(TEXT("Siege_Courtyard_Control.png"));}
}
