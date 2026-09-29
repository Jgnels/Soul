#include "SoulCampaignWorldActor.h"
#include "Engine/StaticMesh.h"
#include "Math/RandomStream.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

namespace
{
float RawHeight(float X, float Y)
{
    const float Rolling = 100.f + 55.f*FMath::Sin(X/800.f)*FMath::Cos(Y/650.f) + 25.f*FMath::Sin((X+Y)/310.f);
    const float North = 1050.f*FMath::Exp(-FMath::Square((Y-2850.f)/680.f))*(.6f+.4f*FMath::Square(FMath::Sin(X/590.f)));
    const float DistantSouth = 1150.f*FMath::Exp(-FMath::Square((Y+4800.f)/750.f))*(.65f+.35f*FMath::Square(FMath::Sin(X/640.f)));
    const float East = 750.f*FMath::Exp(-FMath::Square((X-3950.f)/550.f))*(.6f+.4f*FMath::Square(FMath::Cos(Y/580.f)));
    const float Shrine = 310.f*FMath::Exp(-(FMath::Square((X-800.f)/850.f)+FMath::Square((Y+2300.f)/850.f)));
    const float Quarry = 220.f*FMath::Exp(-(FMath::Square((X+950.f)/750.f)+FMath::Square((Y+1600.f)/750.f)));
    const float PassRidges = 570.f*FMath::Exp(-FMath::Square((FMath::Abs(X-1700.f)-530.f)/270.f)-FMath::Square((Y+1350.f)/950.f))*(.8f+.2f*FMath::Cos(Y/170.f));
    const float Channel = FMath::Exp(-FMath::Square((X-ASoulCampaignWorldActor::RiverX(Y))/165.f));
    return FMath::Lerp(Rolling+North+DistantSouth+East+Shrine+Quarry+PassRidges,-32.f,Channel);
}
FName TerritoryAt(const FVector& P)
{
    FName Best; float Distance=MAX_flt;
    for(const auto& Pair:ASoulCampaignWorldActor::Locations())
    {
        const float D=FVector::DistSquaredXY(P,Pair.Value);
        if(D<Distance) { Distance=D; Best=Pair.Key; }
    }
    return Best;
}
void AddInstance(UInstancedStaticMeshComponent* Mesh,FVector P,FVector Scale,FRotator R=FRotator::ZeroRotator)
{ Mesh->AddInstance(FTransform(R,P,Scale)); }
}
ASoulCampaignWorldActor::ASoulCampaignWorldActor()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.bStartWithTickEnabled=false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("WorldRoot")));
    Terrain=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Terrain"));
    Terrain->SetupAttachment(RootComponent);
    Terrain->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    River=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("River"));
    River->SetupAttachment(RootComponent);
    River->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Party=CreateDefaultSubobject<USceneComponent>(TEXT("PlayerParty"));
    Party->SetupAttachment(RootComponent);
}
float ASoulCampaignWorldActor::RiverX(float Y) { return -200.f+270.f*FMath::Sin((Y-1200.f)/1000.f); }
const TMap<FName,FVector>& ASoulCampaignWorldActor::Locations()
{
    static const TMap<FName,FVector> Positions={
        {TEXT("human_capital"),FVector(-3000,0,105)},
        {TEXT("crossroads"),FVector(-1600,0,80)},
        {TEXT("old_quarry"),FVector(-700,-1500,250)},
        {TEXT("river_ford"),FVector(-200,1200,10)},
        {TEXT("forest_edge"),FVector(200,-500,100)},
        {TEXT("ancient_shrine"),FVector(700,-2100,370)},
        {TEXT("orc_watch"),FVector(1600,800,150)},
        {TEXT("north_pass"),FVector(1700,-1300,150)},
        {TEXT("orc_camp"),FVector(3100,0,260)}
    };
    return Positions;
}
float ASoulCampaignWorldActor::HeightAt(float X,float Y)
{
    float Height=RawHeight(X,Y);
    for(const auto& Pair:Locations())
    {
        const float D=FVector2D(X-Pair.Value.X,Y-Pair.Value.Y).Size();
        const float Blend=1.f-FMath::SmoothStep(220.f,570.f,D);
        Height=FMath::Lerp(Height,static_cast<float>(Pair.Value.Z),Blend);
    }
    // Preserve a continuous channel through location terraces; the ford bridge spans it.
    const float Channel=1.f-FMath::SmoothStep(130.f,260.f,FMath::Abs(X-RiverX(Y)));
    return FMath::Lerp(Height,-32.f,Channel);
}
UMaterialInstanceDynamic* ASoulCampaignWorldActor::MakeMaterial(UObject* Outer,const FLinearColor& Color)
{
    auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Soul/Campaign/M_CampaignSurface.M_CampaignSurface"));
    if(!Base) { UE_LOG(LogTemp,Error,TEXT("Owned campaign surface material missing")); return nullptr; }
    auto* Material=UMaterialInstanceDynamic::Create(Base,Outer);
    Material->SetVectorParameterValue(TEXT("Tint"),Color);
    Material->SetVectorParameterValue(TEXT("Color"),Color);
    return Material;
}
UInstancedStaticMeshComponent* ASoulCampaignWorldActor::MakeInstances(AActor* Owner,const TCHAR* Shape,const FLinearColor& Color)
{
    auto* Mesh=NewObject<UInstancedStaticMeshComponent>(Owner);
    Mesh->SetupAttachment(Owner->GetRootComponent());
    const FString Kind(Shape);
    const TCHAR* Path=Kind==TEXT("Cone")?TEXT("/Engine/BasicShapes/Cone.Cone"):
        Kind==TEXT("Cylinder")?TEXT("/Engine/BasicShapes/Cylinder.Cylinder"):TEXT("/Engine/BasicShapes/Cube.Cube");
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Path));
    Mesh->SetMaterial(0,MakeMaterial(Owner,Color));
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetGenerateOverlapEvents(false);
    Mesh->RegisterComponent(); Owner->AddInstanceComponent(Mesh);
    return Mesh;
}
void ASoulCampaignWorldActor::Build(USoulFounderPlaytestStateSubsystem* InState)
{
    const double Started=FPlatformTime::Seconds();
    State=InState;
    BuildTerrain(); BuildRoads(); BuildDressing(); BuildParty();
    RefreshKnowledge(); PresentPlayerLocation(State->PlayerRegion,false);
    UE_LOG(LogTemp,Display,TEXT("SOUL_WORLD_BUILT vertices=%d routes=%d elapsed_ms=%.2f"),Vertices.Num(),Roads.Num(),(FPlatformTime::Seconds()-Started)*1000.0);
}
void ASoulCampaignWorldActor::BuildTerrain()
{
    constexpr int32 NX=350, NY=350;
    for(int32 Y=0;Y<=NY;++Y) for(int32 X=0;X<=NX;++X)
    {
        const float PX=-HalfWidth+2*HalfWidth*X/NX, PY=-HalfDepth+2*HalfDepth*Y/NY;
        const float H=HeightAt(PX,PY);
        Vertices.Add(FVector(PX,PY,H));
        Normals.Add(FVector(HeightAt(PX-5,PY)-HeightAt(PX+5,PY),HeightAt(PX,PY-5)-HeightAt(PX,PY+5),10).GetSafeNormal());
        UVs.Add(FVector2D(PX/1000,PY/1000));
        FLinearColor C=FMath::Lerp(FLinearColor(.17f,.235f,.095f),FLinearColor(.31f,.34f,.19f),.5f+.5f*FMath::Sin(PX/570.f)*FMath::Cos(PY/400.f));
        if(H>450) C=FMath::Lerp(C,FLinearColor(.36f,.37f,.33f),FMath::Clamp((H-450)/450.f,0.f,1.f));
        if(H>930) C=FMath::Lerp(C,FLinearColor(.72f,.75f,.69f),FMath::Clamp((H-930)/220.f,0.f,1.f));
        if(FMath::Abs(PX-RiverX(PY))<180) C=FLinearColor(.24f,.25f,.17f);
        BaseColors.Add(C);
        if(X<NX&&Y<NY)
        {
            int32 A=Y*(NX+1)+X,B=A+1,CIndex=A+NX+1,D=CIndex+1;
            Triangles.Append({A,CIndex,B,B,CIndex,D});
        }
    }
    auto* Mat=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Soul/Campaign/M_CampaignTerrain.M_CampaignTerrain"));
    Terrain->CreateMeshSection_LinearColor(0,Vertices,Triangles,Normals,UVs,BaseColors,{},false);
    Terrain->SetMaterial(0,Mat);
    TArray<FVector> WaterV,WaterN; TArray<FVector2D> WaterUV; TArray<int32> WaterT; TArray<FLinearColor> WaterC;
    for(int32 I=0;I<=320;++I)
    {
        float Y=-HalfDepth+2.f*HalfDepth*I/320.f;
        for(float Side:{-1.f,1.f})
        {
            WaterV.Add(FVector(RiverX(Y)+Side*(105.f+10.f*FMath::Sin(Y/450)),Y,-8.f));
            WaterN.Add(FVector::UpVector); WaterUV.Add(FVector2D(Side,I/10.f));
            WaterC.Add(FLinearColor(.08f,.25f,.29f));
        }
        if(I<320) {int32 A=I*2; WaterT.Append({A,A+2,A+1,A+1,A+2,A+3});}
    }
    River->CreateMeshSection_LinearColor(0,WaterV,WaterT,WaterN,WaterUV,WaterC,{},false);
    River->SetMaterial(0,Mat);
}
FVector ASoulCampaignWorldActor::RoadPoint(FName From,FName To,float Alpha)
{
    // Stable unordered-pair orientation gives the same physical path in both directions.
    bool Reverse=From.ToString()>To.ToString();
    FVector A=Locations().FindRef(Reverse?To:From), B=Locations().FindRef(Reverse?From:To);
    float T=Reverse?1.f-Alpha:Alpha;
    FVector P=FMath::Lerp(A,B,T);
    FVector Side=FVector::CrossProduct((B-A).GetSafeNormal2D(),FVector::UpVector);
    P+=Side*(FMath::Sin(T*PI)*110.f);
    P.Z=FMath::Max(HeightAt(P.X,P.Y)+9.f,3.f);
    return P;
}
void ASoulCampaignWorldActor::BuildRoads()
{
    TSet<FString> Seen;
    for(const auto& Region:State->World.Regions) for(FName Neighbor:Region.Value.Neighbors)
    {
        if(!Locations().Contains(Region.Key)||!Locations().Contains(Neighbor))continue;
        FString A=Region.Key.ToString(),B=Neighbor.ToString(),Key=A<B?A+TEXT("|")+B:B+TEXT("|")+A;
        if(Seen.Contains(Key))continue; Seen.Add(Key);
        auto* Road=NewObject<UProceduralMeshComponent>(this);
        Road->SetupAttachment(RootComponent); Road->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Road->RegisterComponent(); AddInstanceComponent(Road);
        TArray<FVector> V,N; TArray<FVector2D> UV; TArray<int32> T; TArray<FLinearColor> C;
        constexpr int32 Steps=48;
        for(int32 I=0;I<=Steps;++I)
        {
            float Alpha=static_cast<float>(I)/Steps;
            FVector P=RoadPoint(Region.Key,Neighbor,Alpha);
            FVector Tangent=RoadPoint(Region.Key,Neighbor,FMath::Min(Alpha+.01f,1.f))-RoadPoint(Region.Key,Neighbor,FMath::Max(Alpha-.01f,0.f));
            FVector Side=FVector::CrossProduct(Tangent.GetSafeNormal2D(),FVector::UpVector)*38.f;
            for(float Sign:{-1.f,1.f})
            {
                FVector Edge=P+Side*Sign; Edge.Z=FMath::Max(HeightAt(Edge.X,Edge.Y)+12.f,10.f);
                V.Add(Edge); N.Add(FVector::UpVector); UV.Add(FVector2D(Sign,Alpha*20)); C.Add(FLinearColor(.43f,.35f,.22f));
            }
            if(I<Steps){int32 K=I*2;T.Append({K,K+2,K+1,K+1,K+2,K+3});}
        }
        Road->CreateMeshSection_LinearColor(0,V,T,N,UV,C,{},false);
        Road->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Soul/Campaign/M_CampaignTerrain.M_CampaignTerrain")));
        Roads.Add(Road); RoadRegions.Add({Region.Key,Neighbor});
    }
}
void ASoulCampaignWorldActor::BuildDressing()
{
    FRandomStream Random(90229);
    auto* Trunks=MakeInstances(this,TEXT("Cylinder"),FLinearColor(.13f,.09f,.055f));
    auto* Pine=MakeInstances(this,TEXT("Cone"),FLinearColor(.055f,.14f,.075f));
    auto* PineLight=MakeInstances(this,TEXT("Cone"),FLinearColor(.11f,.22f,.10f));
    TArray<FVector> RockVertices,RockNormals;
    TArray<int32> RockTriangles;
    TArray<FVector2D> RockUVs;
    TArray<FLinearColor> RockColors;
    auto Rock=[&](FVector Position,FVector Size,FRotator Rotation)
    {
        // Irregular faceted boulders, combined into one lightweight decorative mesh.
        constexpr float P=1.618f;
        const FVector Points[]={FVector(-1,P,0),FVector(1,P,0),FVector(-1,-P,0),FVector(1,-P,0),
            FVector(0,-1,P),FVector(0,1,P),FVector(0,-1,-P),FVector(0,1,-P),
            FVector(P,0,-1),FVector(P,0,1),FVector(-P,0,-1),FVector(-P,0,1)};
        const int32 Faces[]={0,11,5,0,5,1,0,1,7,0,7,10,0,10,11,1,5,9,5,11,4,11,10,2,10,7,6,7,1,8,
            3,9,4,3,4,2,3,2,6,3,6,8,3,8,9,4,9,5,2,4,11,6,2,10,8,6,7,9,8,1};
        for(int32 F=0;F<60;F+=3)
        {
            FVector V[3];
            for(int32 I=0;I<3;++I) V[I]=Position+Rotation.RotateVector(Points[Faces[F+I]]*Size*(.48f+.045f*(Faces[F+I]%3)));
            FVector Normal=FVector::CrossProduct(V[1]-V[0],V[2]-V[0]).GetSafeNormal();
            const int32 Base=RockVertices.Num();
            for(int32 I=0;I<3;++I){RockTriangles.Add(Base+(I==1?2:I==2?1:0));RockVertices.Add(V[I]);RockNormals.Add(Normal);RockUVs.Add(FVector2D::ZeroVector);RockColors.Add(FLinearColor(.27f,.29f,.26f)*(1.f+.05f*(F%4)));}
        }
    };
    auto* Crops=MakeInstances(this,TEXT("Cube"),FLinearColor(.42f,.38f,.12f));
    for(int32 I=0;I<820;++I)
    {
        float X=Random.FRandRange(-4450,4400),Y=Random.FRandRange(-3200,3100);
        FVector P(X,Y,HeightAt(X,Y));
        bool Near=false;
        for(const auto& L:Locations())if(FVector::DistSquaredXY(P,L.Value)<FMath::Square(330.f)){Near=true;break;}
        if(Near||FMath::Abs(X-RiverX(Y))<220)continue;
        bool Forest=(X>-200&&X<1300&&Y<250&&Y>-1500)||(X<-3500)||(Y>1650&&X<800);
        if(Forest&&P.Z<650)
        {
            float H=Random.FRandRange(120,245),W=Random.FRandRange(65,100);
            AddInstance(Trunks,P+FVector(0,0,H*.25f),FVector(.15f,.15f,H*.005f));
            AddInstance(I%3?Pine:PineLight,P+FVector(0,0,H*.66f),FVector(W*.01f,W*.01f,H*.01f));
            AddInstance(I%3?Pine:PineLight,P+FVector(0,0,H*.95f),FVector(W*.007f,W*.007f,H*.008f));
        }
        else if(P.Z>320||X>3300)
        {
            float Size=Random.FRandRange(.5f,2.f);
            Rock(P+FVector(0,0,20),FVector(Size*115.f,Size*85.f,Size*70.f),FRotator(Random.FRandRange(-25,25),Random.FRandRange(0,180),Random.FRandRange(-20,20)));
        }
    }
    // Cultivated strips make the capital's hinterland read as inhabited countryside.
    for(int32 Field=0;Field<7;++Field)for(int32 Row=0;Row<9;++Row)
    {
        float X=-3450.f+Field*185.f,Y=-530.f-Row*22.f;
        AddInstance(Crops,FVector(X,Y,HeightAt(X,Y)+5),FVector(1.4f,.065f,.04f));
    }
    auto* Boulders=NewObject<UProceduralMeshComponent>(this);
    Boulders->SetupAttachment(RootComponent);Boulders->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Boulders->RegisterComponent();AddInstanceComponent(Boulders);
    Boulders->CreateMeshSection_LinearColor(0,RockVertices,RockTriangles,RockNormals,RockUVs,RockColors,{},false);
    Boulders->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Soul/Campaign/M_CampaignTerrain.M_CampaignTerrain")));

}
void ASoulCampaignWorldActor::BuildParty()
{
    Soldiers=MakeInstances(this,TEXT("Cube"),FLinearColor(.19f,.28f,.43f));
    Soldiers->AttachToComponent(Party,FAttachmentTransformRules::KeepRelativeTransform);
    for(int32 I=0;I<5;++I)AddInstance(Soldiers,FVector(-55+(I%3)*45,-25+(I/3)*45,35),FVector(.24f,.20f,.65f));
    auto* Pole=MakeInstances(this,TEXT("Cylinder"),FLinearColor(.30f,.24f,.13f));
    Pole->AttachToComponent(Party,FAttachmentTransformRules::KeepRelativeTransform);
    AddInstance(Pole,FVector(60,0,110),FVector(.045f,.045f,2.2f));
    auto* Banner=MakeInstances(this,TEXT("Cube"),FLinearColor(.16f,.43f,.75f));
    Banner->AttachToComponent(Party,FAttachmentTransformRules::KeepRelativeTransform);
    AddInstance(Banner,FVector(95,0,190),FVector(.65f,.055f,.45f));
}
void ASoulCampaignWorldActor::RefreshKnowledge()
{
    if(!State)return;
    if(Soldiers) Soldiers->SetVisibility(State->PlayerArmy.FindRef(State->PlayerUnitId)>0);
    FString Signature;
    TArray<FName> Keys;Locations().GetKeys(Keys);Keys.Sort(FNameLexicalLess());
    for(FName Id:Keys)Signature+=FString::Printf(TEXT("%d%d"),FSoulWorldRules::IsExplored(State->World,State->PlayerFaction,Id),FSoulWorldRules::IsVisible(State->World,State->PlayerFaction,Id));
    if(Signature==KnowledgeSignature)return;KnowledgeSignature=Signature;
    TArray<FLinearColor> Colors;Colors.Reserve(Vertices.Num());
    for(int32 I=0;I<Vertices.Num();++I)
    {
        // Blend only the environmental veil. Locations and force information retain
        // the exact authoritative explored/visible gates in the location actor.
        const FName Nearest=TerritoryAt(Vertices[I]);
        const float NearestDistance=FVector::DistXY(Vertices[I],Locations().FindRef(Nearest));
        float WeightSum=0.f,Veil=0.f;
        for(FName Id:Keys)
        {
            const float Distance=FVector::DistXY(Vertices[I],Locations().FindRef(Id));
            const float Weight=1.f-FMath::SmoothStep(0.f,620.f,Distance-NearestDistance);
            const bool Explored=FSoulWorldRules::IsExplored(State->World,State->PlayerFaction,Id);
            const bool Visible=FSoulWorldRules::IsVisible(State->World,State->PlayerFaction,Id);
            Veil+=Weight*(Visible?0.f:Explored?.60f:.94f);WeightSum+=Weight;
        }
        Colors.Add(FMath::Lerp(BaseColors[I],FLinearColor(.10f,.14f,.18f),Veil/WeightSum));
    }
    Terrain->UpdateMeshSection_LinearColor(0,Vertices,Normals,UVs,Colors,{},false);
    for(int32 I=0;I<Roads.Num();++I)
        Roads[I]->SetVisibility(FSoulWorldRules::IsExplored(State->World,State->PlayerFaction,RoadRegions[I].Key)&&FSoulWorldRules::IsExplored(State->World,State->PlayerFaction,RoadRegions[I].Value));
}
FVector ASoulCampaignWorldActor::PartyAnchor(FName Region)
{
    FVector P=Locations().FindRef(Region);
    // Station the company in front of fortified silhouettes, rather than inside walls.
    if(Region==TEXT("human_capital")||Region==TEXT("orc_camp")||Region==TEXT("orc_watch")||Region==TEXT("north_pass")) P.Y+=290.f;
    else if(Region!=TEXT("river_ford")) P.Y+=160.f;
    P.Z=Region==TEXT("river_ford")?38.f:HeightAt(P.X,P.Y)+12.f;
    return P;
}
void ASoulCampaignWorldActor::PresentPlayerLocation(FName RegionId,bool bAnimate)
{
    if(RegionId==PresentedRegion)return;
    if(bAnimate&&Locations().Contains(PresentedRegion)&&State&&FSoulWorldRules::CanMove(State->World,PresentedRegion,RegionId))
    { TravelFrom=PresentedRegion;TravelTo=RegionId;TravelStart=Party->GetRelativeLocation();TravelAlpha=0;SetActorTickEnabled(true); }
    else {TravelAlpha=1;Party->SetRelativeLocation(PartyAnchor(RegionId));}
    PresentedRegion=RegionId;
}
void ASoulCampaignWorldActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if(TravelAlpha>=1){SetActorTickEnabled(false);return;}
    TravelAlpha=FMath::Min(1.f,TravelAlpha+DeltaSeconds/.95f);
    const float Eased=FMath::SmoothStep(0.f,1.f,TravelAlpha);
    FVector Route=RoadPoint(TravelFrom,TravelTo,Eased)+FVector(0,0,12);
    Route=FMath::Lerp(TravelStart,Route,FMath::SmoothStep(0.f,.25f,Eased));
    Route=FMath::Lerp(Route,PartyAnchor(TravelTo),FMath::SmoothStep(.75f,1.f,Eased));
    Party->SetRelativeLocation(Route);
}
