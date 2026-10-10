#include "SoulRealtimeBattleArena.h"
#include "SoulHeartlandBridgeGeometry.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "UObject/Package.h"
bool ASoulRealtimeArenaGameMode::IsHeartlandBridgeBattle() const
{
 return GetWorld()&&GetWorld()->GetOutermost()->GetName().Contains(TEXT("/Soul/Maps/Battles/L_Heartland_RiverBridge"));
}
bool ASoulRealtimeArenaGameMode::SetupHeartlandBridgeCollision()
{
 auto Box=[&](FVector Center,FVector Extent,float Yaw,bool Water)
 {
  auto* A=GetWorld()->SpawnActor<AActor>();if(!A)return false;
  auto* C=NewObject<UBoxComponent>(A);if(!C){A->Destroy();return false;}
  A->SetRootComponent(C);A->AddInstanceComponent(C);C->InitBoxExtent(Extent);
  C->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
  C->SetCollisionObjectType(Water?ECC_WorldDynamic:ECC_WorldStatic);
  C->SetCollisionResponseToAllChannels(Water?ECR_Ignore:ECR_Block);
  C->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block);
  C->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);
  // Water constrains walking, not arrows, spell rays or ground traces. The
  // existing visible river is the boundary; these are never rendered artwork.
  if(Water)C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);
  C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);C->SetHiddenInGame(true);
  C->RegisterComponent();A->SetActorLocation(ArenaOrigin+Center);A->SetActorRotation(FRotator(0,Yaw,0));
  A->Tags.Add(Water?TEXT("Soul.Battle.RiverBoundary"):TEXT("Soul.Battle.BridgeDeck"));BattlefieldBounds.Add(A);return true;
 };
 // Query-only render triangles in the strategic donor do not supply a simple
 // walking deck in this runtime. This measured collision surface is local to
 // the battle copy; the original bridge/map/assets remain unchanged.
 if(!Box(FVector(0,0,-25),FVector(1300,245,25),-15.74778799f,false))return false;
 // The same donor also lacks simple parapet collision. Seat blockers inside
 // its existing stone rails so melee avoidance cannot push bodies into water.
 for(float Side:{-1.f,1.f})
  if(!Box(SoulHeartlandBridge::FromDeck(FVector(0,Side*245,60)),FVector(1250,25,80),-15.74778799f,false))return false;
 int32 WaterBoxes=0;const auto& River=SoulHeartlandBridge::River();
 for(int32 I=1;I<River.Num();++I)
 {
  const FVector A=River[I-1],B=River[I];const float Length=FVector::Dist2D(A,B);
  const int32 Parts=FMath::Max(1,FMath::CeilToInt(Length/100.f));
  for(int32 J=0;J<Parts;++J)
  {
   const FVector C=FMath::Lerp(A,B,(J+.5f)/Parts);const FVector D=SoulHeartlandBridge::ToDeck(C);
   if(FMath::Abs(C.X)>2800||FMath::Abs(C.Y)>2400||(FMath::Abs(D.X)<1350&&FMath::Abs(D.Y)<320))continue;
   if(!Box(C+FVector(0,0,200),FVector(Length/Parts*.5f+3,200,500),(B-A).Rotation().Yaw,true))return false;
   ++WaterBoxes;
  }
 }
 UE_LOG(LogTemp,Display,TEXT("SOUL_HEARTLAND_BRIDGE_PHYSICS deck=1 parapets=2 river_boundaries=%d frozen_world_unchanged=1"),WaterBoxes);
 Status=TEXT("River crossing: infantry must use the bridge. Form up at its entrance; HOLD controls the frontage.");
 return WaterBoxes>0;
}
