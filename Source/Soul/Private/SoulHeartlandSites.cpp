#include "SoulHeartlandSites.h"
#include "SoulCampaignWorldActor.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
bool DressHeartlandSite(AActor* Owner,FName Region)
{
    const auto* GI=Owner->GetWorld()->GetGameInstance();
    const auto* State=GI?GI->GetSubsystem<USoulFounderPlaytestStateSubsystem>():nullptr;
    if(!State||!State->IsHeartlandEnabled())return false;
    if(Region!=TEXT("crossroads")&&Region!=TEXT("old_quarry")&&Region!=TEXT("ancient_shrine"))return false;
    const FVector Anchor=Owner->GetActorLocation();
    // Natural bench beside the canonical node. No terrain or anchor mutation.
    // Absolute transforms keep the old marker scale out of the licensed art.
    const float Radius=Region==TEXT("crossroads")?650.f:450.f;
    FVector Bench=Anchor;float BestRelief=MAX_flt;
    for(const FVector2D Offset:{FVector2D(1700,1700),FVector2D(-1700,1700),FVector2D(1700,-1700),FVector2D(-1700,-1700)})
    {
        const FVector P=Anchor+FVector(Offset,0);float Lo=MAX_flt,Hi=-MAX_flt;
        for(float X:{-Radius,0.f,Radius})for(float Y:{-Radius,0.f,Radius})
        {const float Z=ASoulCampaignWorldActor::HeightAt(P.X+X,P.Y+Y);Lo=FMath::Min(Lo,Z);Hi=FMath::Max(Hi,Z);}
        if(Hi-Lo<BestRelief){BestRelief=Hi-Lo;Bench=FVector(P.X,P.Y,(Hi+Lo)*.5f);}
    }
    int32 Placed=0;
    auto Place=[&](const TCHAR* Path,FVector Offset,float Scale,float Yaw)
    {
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,Path);
        if(!Mesh){UE_LOG(LogTemp,Error,TEXT("SOUL_HEARTLAND_SITE_MISSING region=%s mesh=%s"),*Region.ToString(),Path);return;}
        FVector P=Bench+Offset;
        P.Z=ASoulCampaignWorldActor::HeightAt(P.X,P.Y)-Mesh->GetBoundingBox().Min.Z*Scale;
        auto* C=NewObject<UStaticMeshComponent>(Owner);C->SetStaticMesh(Mesh);
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCanEverAffectNavigation(false);C->ComponentTags.Add(TEXT("Soul.HeartlandSite"));
        C->SetupAttachment(Owner->GetRootComponent());Owner->AddInstanceComponent(C);C->RegisterComponent();
        C->SetWorldTransform(FTransform(FRotator(0,Yaw,0),P,FVector(Scale)));++Placed;
    };
    if(Region==TEXT("crossroads"))
        Place(TEXT("/Game/Soul/CampaignProxies/Heartland/SM_SM_CrossroadsWindmill_r1"),FVector::ZeroVector,.65f,30);
    else if(Region==TEXT("ancient_shrine"))
    {
        Place(TEXT("/Game/Hidden_shrine/Meshes/Building/SM_building"),FVector::ZeroVector,.55f,15);
        Place(TEXT("/Game/Hidden_shrine/Meshes/Stairs/SM_stairs"),FVector(0,-480,0),.35f,15);
    }
    else
    {
        // Small quarry works from the owned rock, scaffolding and haul-cart kits.
        Place(TEXT("/Game/Forest_village/Meshes/Rocks/SM_rock_03"),FVector(0,1000,0),9.f,65);
        for(float X:{-500.f,0.f,500.f})
            Place(TEXT("/Game/Medieval_Megapack/Meshes/Scaffolding/SM_Wooden_Support"),FVector(X,0,0),3.f,0);
        Place(TEXT("/Game/Medieval_Megapack/Meshes/Scaffolding/SM_Ladder_A"),FVector(220,210,0),1.5f,90);
        Place(TEXT("/Game/Medieval_Megapack/Meshes/Misc/SM_cart1"),FVector(-450,-550,0),1.1f,-35);
        Place(TEXT("/Game/Medieval_Megapack/Meshes/Misc/SM_cart2"),FVector(450,-600,0),.9f,25);
    }
    UE_LOG(LogTemp,Display,TEXT("SOUL_HEARTLAND_SITE_READY region=%s parts=%d bench=%s relief_cm=%.1f authority=existing_site"),*Region.ToString(),Placed,*Bench.ToString(),BestRelief);
    return Placed>0;
}
