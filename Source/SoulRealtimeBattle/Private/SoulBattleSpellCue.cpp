#include "SoulBattleSpellCue.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"

ASoulBattleSpellCue::ASoulBattleSpellCue()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.TickInterval=.04f;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("SpellRoot")));
    Strands=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SpellRibbons"));
    Shards=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("IceShards"));
    Strands->SetupAttachment(RootComponent); Shards->SetupAttachment(RootComponent);
    Strands->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Shards->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cone.Cone")));
    for(auto* Mesh:{Strands.Get(),Shards.Get()})
    {
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCanEverAffectNavigation(false);
        Mesh->SetCastShadow(false);
    }
}
void ASoulBattleSpellCue::Chain(const TArray<FVector>& Contacts)
{
    Kind=0; Points=Contacts; Duration=1.05f; Color=FLinearColor(7,18,35);
    UpdateGeometry(); SetLifeSpan(Duration);
}
void ASoulBattleSpellCue::Blizzard(FVector Center,float AreaRadius,float Seconds)
{
    Kind=1; Points={Center}; Radius=FMath::Clamp(AreaRadius,50.f,1800.f);
    Duration=FMath::Clamp(Seconds,.1f,20.f); Color=FLinearColor(3,9,16);
    UpdateGeometry(); SetLifeSpan(Duration);
}
void ASoulBattleSpellCue::Aura(const TArray<AActor*>& Recipients,bool bWard,float Seconds)
{
    Kind=bWard?2:3;
    for(auto* Recipient:Recipients) if(IsValid(Recipient)) Anchors.Add(Recipient);
    Duration=FMath::Clamp(Seconds,.1f,10.f);
    Color=bWard?FLinearColor(2,10,22):FLinearColor(3,14,8);
    UpdateGeometry(); SetLifeSpan(Duration);
}
void ASoulBattleSpellCue::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); Age+=DeltaSeconds; UpdateGeometry();
}
void ASoulBattleSpellCue::UpdateGeometry()
{
    if(!Material)
    {
        Material=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Soul/Campaign/M_CampaignSurface.M_CampaignSurface")),this);
        Strands->SetMaterial(0,Material); Shards->SetMaterial(0,Material);
    }
    const float Fade=FMath::Clamp((Duration-Age)/.35f,0.f,1.f);
    Material->SetVectorParameterValue(TEXT("Tint"),Color*Fade);
    TArray<FTransform> Lines,Ice;
    auto Segment=[&](FVector A,FVector B,float Width)
    {
        const FVector Delta=B-A;
        Lines.Emplace(FQuat::FindBetweenNormals(FVector::UpVector,Delta.GetSafeNormal()),
            (A+B)*.5,FVector(Width*.01f,Width*.01f,Delta.Size()*.01f));
    };
    if(Kind==0)
    {
        for(int32 Link=1;Link<Points.Num();++Link)
        {
            const FVector A=Points[Link-1],B=Points[Link];
            const FVector Side=FVector::CrossProduct((B-A).GetSafeNormal(),FVector::UpVector).GetSafeNormal();
            FVector Previous=A;
            for(int32 I=1;I<=12;++I)
            {
                const float T=I/12.f;
                const float Jitter=I==12 ? 0 : FMath::Sin(I*7.3f+Age*70)*45;
                const FVector Next=FMath::Lerp(A,B,T)+Side*Jitter+
                    FVector(0,0,I==12?0:FMath::Cos(I*4.7f+Age*55)*28);
                Segment(Previous,Next,3.5f+Fade*1.5f); Previous=Next;
            }
        }
    }
    else if(Kind==1 && !Points.IsEmpty())
    {
        const FVector Center=Points[0]-FVector(0,0,65);
        for(int32 I=0;I<72;++I)
        {
            FRandomStream Random(42+I*7919);
            const float Angle=Random.FRand()*2*PI;
            const float R=FMath::Sqrt(Random.FRand())*Radius;
            const float Phase=FMath::Frac(Age*1.8f+Random.FRand());
            const FVector Position=Center+FVector(FMath::Cos(Angle)*R,FMath::Sin(Angle)*R,650*(1-Phase));
            Ice.Emplace(FRotator(180,0,0),Position,FVector(.09f,.09f,.75f));
        }
        // A low broken frost rim communicates the actual affected ground.
        for(int32 I=0;I<48;++I)
        {
            const float A=I*2*PI/48+Age*.15f,B=A+2*PI/48*.62f;
            Segment(Center+FVector(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius,18),
                Center+FVector(FMath::Cos(B)*Radius,FMath::Sin(B)*Radius,18),2.4f);
        }
    }
    else
    {
        for(const auto& Weak:Anchors)
        {
            if(!Weak.IsValid()) continue;
            const FVector Center=Weak->GetActorLocation();
            const int32 Count=Kind==2?36:12;
            const float R=Kind==2?100:75;
            for(int32 I=0;I<Count;++I)
            {
                const float A=I*2*PI/Count+Age*(Kind==2?1.8f:4.f);
                const float B=A+2*PI/Count*.75f;
                const float Z=Kind==2 ? -65+float(I%12)*12 : -55+float(I%3)*24;
                Segment(Center+FVector(FMath::Cos(A)*R,FMath::Sin(A)*R,Z),
                    Center+FVector(FMath::Cos(B)*R,FMath::Sin(B)*R,Z+12),Kind==2?3.f:2.5f);
            }
        }
    }
    auto Upload=[](UInstancedStaticMeshComponent* Mesh,const TArray<FTransform>& Transforms)
    {
        if(Mesh->GetInstanceCount()!=Transforms.Num())
        { Mesh->ClearInstances(); if(!Transforms.IsEmpty()) Mesh->AddInstances(Transforms,false); }
        else if(!Transforms.IsEmpty()) Mesh->BatchUpdateInstancesTransforms(0,Transforms,false,true,true);
    };
    Upload(Strands,Lines); Upload(Shards,Ice);
}
