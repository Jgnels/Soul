#include "SoulRealtimeBattleArena.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"

void ASoulRealtimeArenaGameMode::DressDragonBattlefield()
{
    // Runtime set dressing only. The licensed showcase and all donor assets remain untouched.
    // Keep the authored combat/entry lanes clear; decorative bones do not own navigation.
    struct FRelic { const TCHAR* Path; FVector Offset; float Yaw; float LongestSide; };
    const FRelic Relics[]={
        {TEXT("/Game/Dragon_graveyard/Meshes/Skeleton/SM_Skeleton_01"),FVector(-200,3700,0),25,1300},
        {TEXT("/Game/Dragon_graveyard/Meshes/Skeleton/SM_Skeleton_03"),FVector(600,-3800,0),-35,1100},
        {TEXT("/Game/Dragon_graveyard/Meshes/Skulls/SM_skull_01"),FVector(-2850,1350,0),-35,850},
        {TEXT("/Game/Dragon_graveyard/Meshes/Skulls/SM_skull_04"),FVector(2850,-1400,0),145,900},
        {TEXT("/Game/Dragon_graveyard/Meshes/Skeleton/SM_back_bone_01"),FVector(-1200,-3700,0),65,750}};
    for(const auto& Relic:Relics)
    {
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,Relic.Path);
        if(!Mesh) continue;
        FVector Location=ArenaOrigin+Relic.Offset;
        FHitResult Ground;
        if(!GetWorld()->LineTraceSingleByChannel(Ground,Location+FVector(0,0,8000),Location-FVector(0,0,8000),ECC_WorldStatic)) continue;
        // Avoid balancing a landmark on the showcase's steep enclosing cliffs.
        if(Ground.ImpactNormal.Z<.65f) continue;
        const auto Bounds=Mesh->GetBounds();
        const float Scale=Relic.LongestSide/FMath::Max(1.f,float(Bounds.BoxExtent.GetMax()*2));
        auto* Prop=GetWorld()->SpawnActor<AActor>();
        if(!Prop) continue;
        auto* Component=NewObject<UStaticMeshComponent>(Prop);
        Prop->SetRootComponent(Component);Prop->AddInstanceComponent(Component);
        Component->SetStaticMesh(Mesh);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetCanEverAffectNavigation(false);
        Component->SetMobility(EComponentMobility::Movable);
        Component->RegisterComponent();
        Prop->SetActorScale3D(FVector(Scale));
        Prop->SetActorRotation(FRotator(0,Relic.Yaw,0));
        const FVector Pivot=FRotator(0,Relic.Yaw,0).RotateVector(FVector(Bounds.Origin.X,Bounds.Origin.Y,0)*Scale);
        Location=Ground.ImpactPoint-Pivot;
        Location.Z-=(Bounds.Origin.Z-Bounds.BoxExtent.Z)*Scale;
        Prop->SetActorLocation(Location);
        Prop->Tags.Add(TEXT("SoulBattleRelic"));
        UE_LOG(LogTemp,Display,TEXT("SOUL_BATTLE_RELIC mesh=%s position=%s scale=%.3f"),Relic.Path,*Location.ToCompactString(),Scale);
    }
}
