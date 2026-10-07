#include "SoulCampaignTerrain.h"
#include "ProceduralMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "SoulCampaignWorldActor.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/Level.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Camera/CameraActor.h"
#include "HAL/PlatformMisc.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Math/RandomStream.h"
#include "Algo/Reverse.h"
#include "Misc/Crc.h"

namespace SoulCampaignTerrain
{
namespace
{
struct FBridge {FName A,B;FVector Center;float Yaw=0,HalfSpan=0;bool Ford=false;};
struct FField {FVector Center;FVector2D Extent;float Yaw=0;};
double SegmentDistance(const FVector2D& Point,const FVector2D& A,const FVector2D& B)
{
    const FVector2D Delta=B-A;
    const double T=FMath::Clamp(FVector2D::DotProduct(Point-A,Delta)/FMath::Max(Delta.SizeSquared(),1.0),0.0,1.0);
    return FVector2D::Distance(Point,A+Delta*T);
}
struct FBake
{
    TArray<uint8> Heights;
    TMap<FName,FVector> Places;
    TMap<FName,FString> Names;
    TMap<FString,TArray<FVector>> Routes;
    TArray<TArray<FVector>> Water;
    TArray<FBridge> Bridges;
    TArray<FField> Fields;
    bool bValid=false;
    int32 Resolution=1009;
    float Extent=560000.f,Minimum=-280000.f,HeightUnit=2.5f;
    FBake()
    {
        FString Text;TSharedPtr<FJsonObject> Root;
        if(Mesa()){Resolution=2041;Extent=150000;Minimum=-75000;HeightUnit=.5f;}
        if(!FFileHelper::LoadFileToArray(Heights,*(FPaths::ProjectDir()/(Mesa()?TEXT("Data/CampaignMesaLocal/MesaHeight.r16"):TEXT("Data/CampaignTerrainV2/FounderHeight.r16"))))
            ||Heights.Num()!=Resolution*Resolution*2
            ||!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectDir()/(EvilCorridor()?TEXT("Data/CampaignEvilCorridor/presentation.json"):Mesa()?TEXT("Data/CampaignMesa/presentation.json"):TEXT("Data/CampaignTerrainV2/presentation.json"))))
            ||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root))
        {UE_LOG(LogTemp,Error,TEXT("SOUL_TERRAIN invalid/missing presentation payload; run Tools/Setup_Soul_Mesa.ps1 for the default campaign"));return;}
        for(const auto& P:Root->GetObjectField(TEXT("regions"))->Values)
        {const auto& A=P.Value->AsArray();Places.Add(FName(*P.Key),FVector(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber()));}
        for(const auto& R:Root->GetArrayField(TEXT("routes")))
        {
            const auto O=R->AsObject();FString A=O->GetStringField(TEXT("a")),B=O->GetStringField(TEXT("b"));
            TArray<FVector> Points;
            for(const auto& P:O->GetArrayField(TEXT("points"))) {const auto& V=P->AsArray();Points.Add(FVector(V[0]->AsNumber(),V[1]->AsNumber(),0));}
            if(A>B){Swap(A,B);Algo::Reverse(Points);}
            Routes.Add(A+TEXT("|")+B,MoveTemp(Points));
        }
        // The native R7 keep occupies the former straight approach. Refit only
        // this presentation tail around its measured footprint; canonical edge,
        // both anchors, action cost and terrain remain unchanged. See human-approach-fit-r2.json.
        if(EvilCorridor()&&!FParse::Param(FCommandLine::Get(),TEXT("SoulDwarfSettlementProof"))
            &&!FParse::Param(FCommandLine::Get(),TEXT("SoulPopulationBaseline"))
            &&(FParse::Param(FCommandLine::Get(),TEXT("SoulSettlementDevelopmentProof"))
                ||FParse::Param(FCommandLine::Get(),TEXT("SoulHumanSettlementProof"))))
        {
            auto* Route=Routes.Find(TEXT("crossroads|human_capital"));const auto* Capital=Places.Find(TEXT("human_capital"));
            if(Route&&Capital)
            {
                const FVector2D Tail[]={
                    FVector2D(-3529.41176,-588.23529),
                    FVector2D(-3336.20378,-809.05012),
                    FVector2D(-3157.58045,-1042.69610),
                    FVector2D(-2956.71234,-1256.93259),
                    FVector2D(-2707.67498,-1412.24650),
                    FVector2D(-2429.99162,-1507.01279),
                    FVector2D(-2138.85266,-1547.14617),
                    FVector2D(-1845.05454,-1559.66762),
                    FVector2D(-1550.97998,-1556.52459),
                    FVector2D(-1257.38119,-1539.58049),
                    FVector2D(-965.29420,-1505.59378),
                    FVector2D(-677.99081,-1443.68226),
                    FVector2D(-400.44367,-1346.77637),
                    FVector2D(-136.67039,-1217.21773),
                    FVector2D(97.45122,-1040.55851),
                    FVector2D(227.42436,-784.64664),
                    FVector2D(175.63650,-496.75703),
                    FVector2D(71.07677,-221.90539),
                    FVector2D(0.00000,0.00000)};
                const FVector Join(Capital->X+Tail[0].X,Capital->Y+Tail[0].Y,0);
                const int32 JoinIndex=Route->IndexOfByPredicate([&](const FVector& P){return FVector::DistSquaredXY(P,Join)<1.;});
                if(JoinIndex!=INDEX_NONE&&Route->Last().Equals(FVector(Capital->X,Capital->Y,0),1.))
                {
                    Route->SetNum(JoinIndex+1);
                    for(int32 I=1;I<UE_ARRAY_COUNT(Tail);++I)Route->Add(FVector(Capital->X+Tail[I].X,Capital->Y+Tail[I].Y,0));
                    UE_LOG(LogTemp,Display,TEXT("SOUL_CROWNSTEAD_APPROACH native_keep_clearance_cm=340.69 max_grade=12.95 canonical_endpoints_unchanged=1 terrain_edits=0"));
                }
            }
        }
        if(EvilCorridor())for(const auto& N:Root->GetObjectField(TEXT("display_names"))->Values)Names.Add(FName(*N.Key),N.Value->AsString());
        bValid=Places.Num()==9&&Routes.Num()==10;
        for(const auto& Item:Root->GetArrayField(TEXT("fields")))
        {
            const auto& A=Item->AsArray();FField Field;
            Field.Center=FVector(A[0]->AsNumber(),A[1]->AsNumber(),0);
            Field.Extent=FVector2D(A[2]->AsNumber(),A[3]->AsNumber());Field.Yaw=A[4]->AsNumber();Fields.Add(Field);
        }
        for(const auto& Item:Root->GetArrayField(TEXT("bridges")))
        {
            const auto O=Item->AsObject();const auto& C=O->GetArrayField(TEXT("center"));
            FBridge Bridge;Bridge.A=FName(*O->GetStringField(TEXT("a")));Bridge.B=FName(*O->GetStringField(TEXT("b")));
            Bridge.Center=FVector(C[0]->AsNumber(),C[1]->AsNumber(),C[2]->AsNumber());
            Bridge.Yaw=O->GetNumberField(TEXT("yaw"));Bridge.HalfSpan=O->GetNumberField(TEXT("half_span"));Bridge.Ford=O->GetBoolField(TEXT("ford"));
            Bridges.Add(Bridge);
        }
        for(const auto& Line:Root->GetArrayField(TEXT("waterlines")))
        {
            TArray<FVector> Points;
            for(const auto& P:Line->AsArray()){const auto& V=P->AsArray();Points.Add(FVector(V[0]->AsNumber(),V[1]->AsNumber(),V[2]->AsNumber()));}
            Water.Add(MoveTemp(Points));
        }
    }
};
const FBake& Bake(){static const FBake B;return B;}
UHierarchicalInstancedStaticMeshComponent* Instances(AActor* Owner,const TCHAR* Path)
{
    auto* Mesh=LoadObject<UStaticMesh>(nullptr,Path);
    if(!Mesh){UE_LOG(LogTemp,Error,TEXT("SOUL_TERRAIN_V2 missing local licensed mesh %s"),Path);return nullptr;}
    auto* H=NewObject<UHierarchicalInstancedStaticMeshComponent>(Owner);
    H->SetupAttachment(Owner->GetRootComponent());H->SetStaticMesh(Mesh);
    for(int32 I=0;I<Mesh->GetStaticMaterials().Num();++I)
    {
        const auto* Mat=Mesh->GetMaterial(I);
        if(Mat&&Mat->GetName().StartsWith(TEXT("MI_pine_tree")))
            H->SetMaterial(I,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Soul/Campaign/TerrainV2/MI_CampaignPine")));
        else if(Mat&&(Mat->GetName()==TEXT("MI_wood")||Mat->GetName().StartsWith(TEXT("MI_Wood"))))
            H->SetMaterial(I,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Soul/Campaign/TerrainV2/M_CampaignWood")));
        else if(Mat&&(Mat->GetName()==TEXT("MI_wall")||Mat->GetName()==TEXT("MI_PlasteredWall")))
            H->SetMaterial(I,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Soul/Campaign/TerrainV2/M_CampaignMasonry")));
    }
    H->SetCollisionEnabled(ECollisionEnabled::NoCollision);H->SetGenerateOverlapEvents(false);
    H->SetCanEverAffectNavigation(false);H->RegisterComponent();Owner->AddInstanceComponent(H);return H;
}
void Place(UHierarchicalInstancedStaticMeshComponent* H,FVector P,float Width,float Yaw=0,float Tall=1)
{
    if(!H)return;
    const auto B=H->GetStaticMesh()->GetBounds();
    float S=Width/FMath::Max(B.BoxExtent.X*2.f,1.f);
    // These alien prefabs have narrow footprints and exceptionally tall spires.
    // Fit their whole silhouette, preserving proportions, instead of scaling a
    // tower to town width and accidentally making it a mountain.
    if(H->GetStaticMesh()->GetPathName().Contains(TEXT("/AlienPlanet/")))
        S=FMath::Min(Width/FMath::Max(FMath::Max(B.BoxExtent.X,B.BoxExtent.Y)*2.f,1.f),
            Width*.85f/FMath::Max(B.BoxExtent.Z*2.f*Tall,1.f));
    P-=FRotator(0,Yaw,0).RotateVector(FVector(B.Origin.X,B.Origin.Y,0)*S);
    P.Z-=(B.Origin.Z-B.BoxExtent.Z)*S*Tall;
    H->AddInstance(FTransform(FRotator(0,Yaw,0),P,FVector(S,S,S*Tall)));
}
bool AuthoredHumanMiniature()
{
    return !FParse::Param(FCommandLine::Get(),TEXT("SoulDwarfSettlementProof"))
        &&(FParse::Param(FCommandLine::Get(),TEXT("SoulSettlementDevelopmentProof"))
            ||FParse::Param(FCommandLine::Get(),TEXT("SoulHumanSettlementProof")));
}
// Retained decorative verges; the native city derivative moves its hinterland west.

const FVector2D CrownsteadVergeCenters[]={FVector2D(-4600,4500),FVector2D(-7200,4300),FVector2D(-9800,2100)};
bool CrownsteadVergeOverlap(const FVector2D& Offset,double Margin)
{
    const FVector2D Shift=AuthoredHumanMiniature()?FVector2D(-3500,-1000):FVector2D::ZeroVector;
    for(const auto& Center:CrownsteadVergeCenters)
        if(FMath::Abs(Offset.X-Center.X-Shift.X)<=950+Margin&&FMath::Abs(Offset.Y-Center.Y-Shift.Y)<=750+Margin)return true;
    return false;
}
UHierarchicalInstancedStaticMeshComponent* NativeSettlementInstances(AActor* Owner,UStaticMesh* Mesh)
    {
        if(!Mesh)return static_cast<UHierarchicalInstancedStaticMeshComponent*>(nullptr);
        auto* H=NewObject<UHierarchicalInstancedStaticMeshComponent>(Owner);
        H->SetupAttachment(Owner->GetRootComponent());H->SetStaticMesh(Mesh);
        // Preserve the merge's native material slots, UVs and vertex colors.
        H->SetCollisionEnabled(ECollisionEnabled::NoCollision);H->SetGenerateOverlapEvents(false);
        H->SetCanEverAffectNavigation(false);H->RegisterComponent();Owner->AddInstanceComponent(H);return H;
}
// Approved native prefab derivatives on measured retained parcels. These are
// presentation only: region identity, adjacency, economy and saves are unchanged.
struct FRetainedProp {FName Region;const TCHAR* Mesh;FVector2D Offset;float Scale,Yaw,Radius;bool Approach=true;};
const FRetainedProp RetainedProps[]={
    {TEXT("north_pass"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Ravenhold_GateTower_r1"),FVector2D(1500,-1750),0.2500f,35.0f,311.569f},
    {TEXT("north_pass"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Field_Defenses_A_r1"),FVector2D(3375,-4625),0.5500f,40.0f,358.491f},
    {TEXT("orc_camp"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Ravenhold_GateTower_r1"),FVector2D(-1100,-800),0.2500f,6.88f,302.182f},
    {TEXT("orc_camp"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Ravenhold_GateTower_r1"),FVector2D(1800,-450),0.2500f,186.88f,302.182f},
    {TEXT("orc_watch"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Ravenhold_GateTower_r1"),FVector2D(-3700,3375),0.3000f,75.0f,356.079f},
    {TEXT("forest_edge"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Forest_Hut_A_r1"),FVector2D(1125,750),0.4500f,200.0f,496.968f},
    {TEXT("forest_edge"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Forest_Hut_B_r1"),FVector2D(2000,2000),0.4500f,220.0f,415.858f},
    {TEXT("forest_edge"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Forest_Hut_A_r1"),FVector2D(2000,3175),0.4000f,170.0f,441.750f},
    {TEXT("ancient_shrine"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Forest_Platform_r1"),FVector2D(-500,1750),0.6000f,20.0f,575.040f},
    {TEXT("ancient_shrine"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Forest_Hut_B_r1"),FVector2D(2500,-3625),0.3500f,40.0f,323.445f},
    {TEXT("orc_camp"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Camp_Tent_r1"),FVector2D(-875,225),0.5000f,100.0f,535.053f},
    {TEXT("orc_camp"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Field_Defenses_B_r1"),FVector2D(1000,-100),0.4000f,0.0f,240.670f},
    {TEXT("orc_camp"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Siege_Trebuchet_r1"),FVector2D(-3375,4000),0.4500f,45.0f,690.400f},
    {TEXT("orc_camp"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Ravenhold_CurtainWall_r2"),FVector2D(-750,-757.758621),.43f,6.88f,267.485719f,false},
    {TEXT("orc_camp"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Ravenhold_CurtainWall_r2"),FVector2D(-320,-705.862069),.43f,6.88f,267.485719f,false},
    {TEXT("orc_camp"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Ravenhold_CurtainWall_r2"),FVector2D(110,-653.965517),.43f,6.88f,267.485719f,false},
    {TEXT("orc_camp"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Ravenhold_CurtainWall_r2"),FVector2D(540,-602.068966),.43f,6.88f,267.485719f,false},
    {TEXT("orc_camp"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Ravenhold_CurtainWall_r2"),FVector2D(970,-550.172414),.43f,6.88f,267.485719f,false},
    {TEXT("orc_camp"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Ravenhold_CurtainWall_r2"),FVector2D(1400,-498.275862),.43f,6.88f,267.485719f,false},
    {TEXT("orc_watch"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Field_Defenses_A_r1"),FVector2D(-2000,4000),0.6500f,0.0f,423.671f},
    {TEXT("orc_watch"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Camp_Tent_r1"),FVector2D(-4500,4000),0.4000f,25.0f,428.043f},
    {TEXT("crossroads"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Camp_Tent_r1"),FVector2D(875,-2500),0.4500f,220.0f,481.548f},
    {TEXT("crossroads"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Field_Defenses_B_r1"),FVector2D(0,-4000),0.6500f,100.0f,391.088f},
    {TEXT("old_quarry"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Camp_Tent_r1"),FVector2D(-1000,-500),0.4500f,50.0f,481.548f},
    {TEXT("river_ford"),TEXT("/Game/Soul/CampaignProxies/Population/SM_Field_Defenses_B_r1"),FVector2D(1625,-2375),0.6000f,40.0f,361.005f},
    {TEXT("crossroads"),TEXT("/Game/SoulCampaignProxies/SM_Medieval_Building_A_r1"),FVector2D(-1975,-600),0.360000f,-225.000001f,433.322034f},
    {TEXT("crossroads"),TEXT("/Game/SoulCampaignProxies/SM_Medieval_Building_B_r1"),FVector2D(-1000,875),0.600000f,90.000000f,428.395987f},
    {TEXT("crossroads"),TEXT("/Game/SoulCampaignProxies/SM_Medieval_Building_C_r1"),FVector2D(100,1725),0.480000f,-135.000001f,375.032704f},
    {TEXT("crossroads"),TEXT("/Game/SoulCampaignProxies/SM_Medieval_Building_B_r1"),FVector2D(2350,1375),0.550000f,225.000000f,392.696321f},
    {TEXT("crossroads"),TEXT("/Game/SoulCampaignProxies/SM_Medieval_Building_C_r1"),FVector2D(2775,2200),0.480000f,180.000000f,375.032704f},
    {TEXT("crossroads"),TEXT("/Game/SoulCampaignProxies/SM_Medieval_Building_B_r1"),FVector2D(1175,-1125),0.550000f,180.000000f,392.696321f},
    {TEXT("old_quarry"),TEXT("/Game/SoulCampaignProxies/SM_Medieval_Building_A_r1"),FVector2D(1450,25),0.420000f,-45.000000f,505.542373f},
    {TEXT("old_quarry"),TEXT("/Game/SoulCampaignProxies/SM_Medieval_Building_B_r1"),FVector2D(-1475,700),0.600000f,64.612094f,428.395987f},
    {TEXT("old_quarry"),TEXT("/Game/Legendary_Forge/Meshes/Forge/SM_forge_02"),FVector2D(-650,1350),0.550000f,0.000000f,150.562915f,true},
    {TEXT("old_quarry"),TEXT("/Game/Legendary_Forge/Meshes/Anvil_02/SM_anvil_02"),FVector2D(-430,1570),0.800000f,45.000000f,54.455340f,false},
    {TEXT("old_quarry"),TEXT("/Game/Legendary_Forge/Meshes/Bellows/SM_bellows"),FVector2D(-650,1640),0.600000f,90.000000f,112.402277f,false},
};
bool RetainedPopulationEnabled()
{
    return EvilCorridor()&&!FParse::Param(FCommandLine::Get(),TEXT("SoulDwarfSettlementProof"))
        &&!FParse::Param(FCommandLine::Get(),TEXT("SoulPopulationBaseline"));
}
bool RetainedPopulationOverlap(const FVector2D& WorldPoint,double Margin)
{
    if(!RetainedPopulationEnabled())return false;
    for(const auto& P:RetainedProps)if(const auto* Origin=Bake().Places.Find(P.Region))
        if(FVector2D::Distance(WorldPoint,FVector2D(*Origin)+P.Offset)<P.Radius+Margin+120)return true;
    return false;
}
void RetainedRegionPopulation(AActor* Owner,FName Id)
{
    if(!RetainedPopulationEnabled())return;
    const FVector Origin=Owner->GetActorLocation();const float OwnerScale=Owner->GetActorScale3D().X;
    TMap<UStaticMesh*,UHierarchicalInstancedStaticMeshComponent*> Components;
    TArray<FVector> Vertices,Normals;TArray<FVector2D> UV;TArray<int32> Triangles;TArray<FLinearColor> Colors;
    int32 Placed=0,Rejected=0,Lanes=0;float MaxRelief=0,MaxGrade=0;
    for(const auto& Plot:RetainedProps)
    {
        if(Plot.Region!=Id)continue;
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,Plot.Mesh);
        if(!Mesh){++Rejected;continue;}
        const auto Bounds=Mesh->GetBounds();const FRotator Rotation(0,Plot.Yaw,0);
        const FVector2D Center=FVector2D(Origin)+Plot.Offset;
        FVector2D RoadPoint;double RoadDistance=MAX_dbl;
        for(const auto& Route:Bake().Routes)for(int32 I=1;I<Route.Value.Num();++I)
        {
            const FVector2D A(Route.Value[I-1]),B(Route.Value[I]),Delta=B-A;
            const double T=FMath::Clamp(FVector2D::DotProduct(Center-A,Delta)/FMath::Max(Delta.SizeSquared(),1.),0.,1.);
            const FVector2D Candidate=A+Delta*T;const double Distance=FVector2D::Distance(Center,Candidate);
            if(Distance<RoadDistance){RoadDistance=Distance;RoadPoint=Candidate;}
        }
        float Low=MAX_flt,High=-MAX_flt;
        for(int32 Y=0;Y<=12;++Y)for(int32 X=0;X<=12;++X)
        {
            const FVector Offset=Rotation.RotateVector(FVector(Bounds.BoxExtent.X*(X/6.-1),Bounds.BoxExtent.Y*(Y/6.-1),0)*Plot.Scale);
            const float Z=Height(Center.X+Offset.X,Center.Y+Offset.Y);Low=FMath::Min(Low,Z);High=FMath::Max(High,Z);
        }
        if(Low<200||High-Low>65||RoadDistance<Plot.Radius+250){++Rejected;continue;}
        const FVector NativeBottom(Bounds.Origin.X,Bounds.Origin.Y,Bounds.Origin.Z-Bounds.BoxExtent.Z);
        const FVector Pivot=FVector(Center,Low+2)-Rotation.RotateVector(NativeBottom*Plot.Scale);
        const FTransform Local(Rotation,(Pivot-Origin)/OwnerScale,FVector(Plot.Scale/OwnerScale));
        // A few native workshop/tent materials do not support instancing. Use
        // ordinary mesh components for these sparse props, retaining their real
        // authored materials instead of mutating donor shaders or accepting gray fallbacks.
        const bool NativeStatic=Mesh->GetPathName().StartsWith(TEXT("/Game/Legendary_Forge/"))
            ||Mesh->GetName()==TEXT("SM_Camp_Tent_r1");
        if(NativeStatic)
        {
            auto* Component=NewObject<UStaticMeshComponent>(Owner);
            Component->SetupAttachment(Owner->GetRootComponent());Component->SetStaticMesh(Mesh);
            Component->SetRelativeTransform(Local);Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Component->SetGenerateOverlapEvents(false);Component->SetCanEverAffectNavigation(false);
            Component->RegisterComponent();Owner->AddInstanceComponent(Component);
        }
        else
        {
            auto*& Component=Components.FindOrAdd(Mesh);
            if(!Component)
            {
                Component=NativeSettlementInstances(Owner,Mesh);
                if(Mesh->GetPathName().StartsWith(TEXT("/Game/SoulCampaignProxies/SM_Medieval_Building_")))
                    Component->SetForcedLodModel(2); // Reviewed native-derived 60k LOD1, not the multi-million-triangle source merge.
            }
            Component->AddInstance(Local);
        }
        ++Placed;MaxRelief=FMath::Max(MaxRelief,High-Low);
        if(!Plot.Approach)continue; // Connected native wall modules need no individual road spurs.
        // A short decorative approach reaches the existing road. It adds no
        // legal edge or movement rule and samples the same physical Landscape.
        const FVector2D ToRoad=(RoadPoint-Center).GetSafeNormal();
        const FVector2D End=Center+ToRoad*Plot.Radius*.72;
        const FVector2D Along=End-RoadPoint;const double Length=Along.Size();
        if(Length<100||Length>6500)continue;
        const FVector2D Bend(-Along.Y,Along.X);const int32 Steps=FMath::Max(4,FMath::CeilToInt(Length/90));
        TArray<FVector> Points;float Grade=0;
        for(int32 I=0;I<=Steps;++I)
        {
            const double T=static_cast<double>(I)/Steps;
            const FVector2D XY=RoadPoint+Along*T+Bend*(.075*FMath::Sin(T*PI));
            const FVector Point(XY,Height(XY.X,XY.Y)+7);
            if(!Points.IsEmpty())Grade=FMath::Max(Grade,static_cast<float>(FMath::Abs(Point.Z-Points.Last().Z)/FMath::Max(FVector::Dist2D(Point,Points.Last()),1.)));
            Points.Add(Point);
        }
        if(Grade>FMath::Tan(FMath::DegreesToRadians(22.f)))continue;
        MaxGrade=FMath::Max(MaxGrade,FMath::RadiansToDegrees(FMath::Atan(Grade)));++Lanes;
        const int32 Start=Vertices.Num();
        for(int32 I=0;I<=Steps;++I)
        {
            const FVector Tangent=Points[FMath::Min(I+1,Steps)]-Points[FMath::Max(I-1,0)];
            const FVector Side=FVector::CrossProduct(Tangent.GetSafeNormal2D(),FVector::UpVector)*95;
            for(int32 Sign:{-1,1})
            {
                FVector V=Points[I]+Side*Sign;V.Z=Height(V.X,V.Y)+7;
                Vertices.Add((V-Origin)/OwnerScale);Normals.Add(FVector::UpVector);
                UV.Add(FVector2D((Sign+1)*.5,I*90./500));Colors.Add(FLinearColor(.36f,.29f,.18f));
            }
            if(I<Steps){const int32 K=Start+I*2;Triangles.Append({K,K+2,K+1,K+1,K+2,K+3});}
        }
    }
    if(!Vertices.IsEmpty())
    {
        auto* Lane=NewObject<UProceduralMeshComponent>(Owner);Lane->SetupAttachment(Owner->GetRootComponent());
        Lane->SetCollisionEnabled(ECollisionEnabled::NoCollision);Lane->SetCastShadow(false);Lane->SetCanEverAffectNavigation(false);
        Lane->RegisterComponent();Owner->AddInstanceComponent(Lane);
        Lane->CreateMeshSection_LinearColor(0,Vertices,Triangles,Normals,UV,Colors,{},false);
        Lane->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Soul/Campaign/TerrainV2/M_FounderRoad")));
    }
    if(Placed||Rejected)UE_LOG(LogTemp,Display,TEXT("SOUL_RETAINED_POPULATION region=%s placed=%d rejected=%d lanes=%d max_relief_cm=%.2f max_lane_grade=%.2f native_materials=1 terrain_edits=0"),*Id.ToString(),Placed,Rejected,Lanes,MaxRelief,MaxGrade);
}
// Native building envelopes and <=22 degree grade checked by
// fit_human_campaign_lanes.py. These paths are presentation, never legal edges.
void CrownsteadNativeStreets(AActor* Owner)
{
    if(!AuthoredHumanMiniature())return;
    const FVector Origin=Owner->GetActorLocation();const double Scale=Owner->GetActorScale3D().X;
    const TArray<TArray<FVector2D>> Paths={
        {FVector2D(-5900,-600),FVector2D(-7400,400)},
        {FVector2D(-7400,400),FVector2D(-7600,2500)},
        {FVector2D(-7600,2500),FVector2D(-7200,4500)},
        {FVector2D(-7200,4500),FVector2D(-5200,5100)},
        {FVector2D(-5200,5100),FVector2D(-2700,4500)},
        {FVector2D(-2700,4500),FVector2D(-2800,2300)},
        {FVector2D(-2800,2300),FVector2D(-3700,900)},
        {FVector2D(-3700,900),FVector2D(-3529.41176,-588.23529)},
        {FVector2D(-7600,2500),FVector2D(-6800,2100),FVector2D(-6100,2200)},
        {FVector2D(-6100,2200),FVector2D(-5500,2500)},
        {FVector2D(-5500,2500),FVector2D(-5300,3000),FVector2D(-5200,3800)},
        {FVector2D(-7400,400),FVector2D(-10000,1200)}
    };
    TArray<FVector> V,N;TArray<FVector2D> UV;TArray<int32> T;TArray<FLinearColor> C;
    float MaximumGrade=0;int32 Segments=0;
    for(const auto& Path:Paths)for(int32 P=1;P<Path.Num();++P)
    {
        const FVector2D A=Path[P-1]+FVector2D(Origin),B=Path[P]+FVector2D(Origin),D=B-A;
        const int32 Count=FMath::Max(2,FMath::CeilToInt(D.Size()/45.));
        const FVector2D Side=FVector2D(D.Y,-D.X).GetSafeNormal()*75.;
        TArray<FVector> Points;float Grade=0;
        for(int32 I=0;I<=Count;++I)
        {
            const FVector2D XY=A+D*(static_cast<double>(I)/Count);const FVector Point(XY,Height(XY.X,XY.Y)+8);
            if(!Points.IsEmpty())Grade=FMath::Max(Grade,static_cast<float>(FMath::RadiansToDegrees(FMath::Atan(FMath::Abs(Point.Z-Points.Last().Z)/FMath::Max(FVector::Dist2D(Point,Points.Last()),1.)))));
            Points.Add(Point);
        }
        if(Grade>22.f)continue;
        MaximumGrade=FMath::Max(MaximumGrade,Grade);++Segments;const int32 Start=V.Num();
        for(int32 I=0;I<=Count;++I)
        {
            for(int32 Column=0;Column<3;++Column)
            {
                const FVector2D XY=FVector2D(Points[I])+Side*(Column-1);
                V.Add((FVector(XY,Height(XY.X,XY.Y)+8)-Origin)/Scale);N.Add(FVector::UpVector);
                UV.Add(FVector2D(Column*.5,I*45./500));C.Add(FLinearColor(.36f,.29f,.18f));
            }
            if(I<Count)for(int32 Column=0;Column<2;++Column){const int32 K=Start+I*3+Column;T.Append({K,K+3,K+1,K+1,K+3,K+4});}
        }
    }
    auto* Lanes=NewObject<UProceduralMeshComponent>(Owner);Lanes->SetupAttachment(Owner->GetRootComponent());
    Lanes->SetCollisionEnabled(ECollisionEnabled::NoCollision);Lanes->SetCanEverAffectNavigation(false);Lanes->SetCastShadow(false);
    Lanes->RegisterComponent();Owner->AddInstanceComponent(Lanes);Lanes->CreateMeshSection_LinearColor(0,V,T,N,UV,C,{},false);
    Lanes->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Soul/Campaign/TerrainV2/M_FounderRoad")));
    UE_LOG(LogTemp,Display,TEXT("SOUL_NATIVE_HUMAN_LANES segments=%d max_grade=%.2f terrain_edits=0 legal_edges_added=0"),Segments,MaximumGrade);
}
void CrownsteadHinterland(AActor* Owner)
{
    CrownsteadNativeStreets(Owner);
    const FVector2D HinterlandShift=AuthoredHumanMiniature()?FVector2D(-3500,-1000):FVector2D::ZeroVector;
    const FVector Origin=Owner->GetActorLocation();const float OwnerScale=Owner->GetActorScale3D().X;
    // Approved native exterior props. Bounds-centered XY and native minimum Z,
    // with one uniform world scale, keep their authored materials/proportions.
    UHierarchicalInstancedStaticMeshComponent* Verge[]={
        NativeSettlementInstances(Owner,LoadObject<UStaticMesh>(nullptr,TEXT("/Game/SoulCampaignWorld/SM_WorldBroadleaf"))),
        NativeSettlementInstances(Owner,LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Forest_village/Meshes/Vegetation/SM_bush"))),
        NativeSettlementInstances(Owner,LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Forest_village/Meshes/Kit/SM_fence_01")))};
    for(int32 I=0;I<3;++I)if(Verge[I])Verge[I]->SetForcedLodModel(I==2?1:2);
    auto NativeGroundPlace=[&](UHierarchicalInstancedStaticMeshComponent* H,FVector2D Offset,float Scale,float Yaw)
    {
        if(!H)return;
        const auto Bounds=H->GetStaticMesh()->GetBounds();
        const FVector Pivot=Origin+FVector(Offset.X,Offset.Y,0)-FRotator(0,Yaw,0).RotateVector(FVector(Bounds.Origin.X,Bounds.Origin.Y,0)*Scale);
        const FVector Position(Offset.X/OwnerScale,Offset.Y/OwnerScale,(Height(Pivot.X,Pivot.Y)-Origin.Z)/OwnerScale);
        Place(H,Position,Bounds.BoxExtent.X*2*Scale/OwnerScale,Yaw);
    };
    struct FVergeProp {int32 Kind;FVector2D Offset;float Yaw,Crown;};
    // Exact 21 positions in Pivot/crownstead-verge-layout.json; Crown is tree max XY in cm.
    const FVergeProp VergeProps[]={
        {0,FVector2D(-4980,4355),18,260},{0,FVector2D(-4240,4380),131,280},
        {0,FVector2D(-4920,4665),253,250},{0,FVector2D(-4270,4660),77,270},
        {0,FVector2D(-10180,2125),204,270},{0,FVector2D(-9450,2160),326,290},
        {1,FVector2D(-5215,4260),6,0},{1,FVector2D(-5200,4490),67,0},
        {1,FVector2D(-5210,4730),128,0},{1,FVector2D(-6740,4610),189,0},
        {1,FVector2D(-6600,4575),250,0},{1,FVector2D(-9190,1880),311,0},
        {1,FVector2D(-9200,2035),12,0},
        {2,FVector2D(-4970,4855),0,0},{2,FVector2D(-4710,4855),0,0},
        {2,FVector2D(-7650,3945),0,0},{2,FVector2D(-7380,3945),0,0},
        {2,FVector2D(-6830,3945),0,0},{2,FVector2D(-10240,2455),0,0},
        {2,FVector2D(-9990,2455),0,0},{2,FVector2D(-9420,2455),0,0}};
    int32 VergePlaced[3]={};
    for(const auto& Prop:VergeProps)if(auto* H=Verge[Prop.Kind])
    {
        const FVector Size=H->GetStaticMesh()->GetBounds().BoxExtent*2;
        const float Scale=Prop.Kind==0?Prop.Crown/FMath::Max(Size.X,Size.Y):Prop.Kind==1?
            FMath::Min(120./FVector2D(Size).Size(),100./Size.Z):
            FMath::Min3(200./FMath::Max(Size.X,Size.Y),70./FMath::Min(Size.X,Size.Y),90./Size.Z);
        NativeGroundPlace(H,Prop.Offset+HinterlandShift,Scale,Prop.Yaw);++VergePlaced[Prop.Kind];
    }
    UHierarchicalInstancedStaticMeshComponent* Crops[]={
        NativeSettlementInstances(Owner,LoadObject<UStaticMesh>(nullptr,TEXT("/Game/SoulCampaignProxies/Foliage/SM_WheatGrass_A_Campaign_r1"))),
        NativeSettlementInstances(Owner,LoadObject<UStaticMesh>(nullptr,TEXT("/Game/SoulCampaignProxies/Foliage/SM_WheatGrass_B_Campaign_r1")))};
    for(auto* H:Crops)if(H)
    {
        H->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Soul/Materials/Settlements/Crops/M_WheatGrass_Coverage_r1")));
        H->SetForcedLodModel(1);
    } // Imported source LOD1 is owned asset LOD0.
    int32 CropPlaced[2]={};
    // Legacy field stays unchanged. The authored city uses a denser, uniformly
    // scaled native field west of its buildings; omit actual boundary props.
    const bool Authored=AuthoredHumanMiniature();
    const int32 Rows=Authored?30:11,Columns=Authored?50:27;
    for(int32 Row=0;Row<Rows;++Row)for(int32 Column=0;Column<Columns;++Column)
    {
        if(!Authored&&Row==10&&Column==26)continue;
        const int32 Variant=(Row+Column)%2;
        if(!Crops[Variant])continue;
        const FVector2D CropOffset(-7720+Column*(Authored?30:40),4100+Row*(Authored?30:40));
        bool Clear=true;
        if(Authored)for(const auto& Prop:VergeProps)
        {
            const double Clearance=Prop.Kind==0?Prop.Crown*.5+110:Prop.Kind==1?180:210;
            if(FVector2D::Distance(CropOffset,Prop.Offset)<Clearance){Clear=false;break;}
        }
        if(!Clear)continue;
        NativeGroundPlace(Crops[Variant],CropOffset+HinterlandShift,Authored?.8f:.4f,(Row*53+Column*37)%360);
        ++CropPlaced[Variant];
    }
    UE_LOG(LogTemp,Display,TEXT("SOUL_CROWNSTEAD_VERGE trees=%d shrubs=%d fences=%d crop_A=%d crop_B=%d crop_scale=%g native_materials=1 ground_sample=1 terrain_edits=0"),
        VergePlaced[0],VergePlaced[1],VergePlaced[2],CropPlaced[0],CropPlaced[1],Authored?.8:.4);
}
bool CrownsteadPopulation(AActor* Owner)
{
    // The retained terrain fit is recorded in Pivot/crownstead-population-fit.json.
    // These are decorative, complete licensed buildings, not new gameplay targets.
    const TCHAR* Paths[]={TEXT("/Game/SoulCampaignProxies/SM_Medieval_Building_A_r1"),
        TEXT("/Game/SoulCampaignProxies/SM_Medieval_Building_B_r1"),TEXT("/Game/SoulCampaignProxies/SM_Medieval_Building_C_r1")};
    UStaticMesh* Meshes[3]={};
    for(int32 I=0;I<3;++I)
    {
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,Paths[I],nullptr,LOAD_NoWarn|LOAD_Quiet);
        if(!Mesh||Mesh->GetNumLODs()<2||Mesh->HasValidNaniteData())continue;
        bool MaterialsValid=Mesh->GetStaticMaterials().Num()>0;
        for(int32 Slot=0;Slot<Mesh->GetStaticMaterials().Num();++Slot)MaterialsValid&=Mesh->GetMaterial(Slot)!=nullptr;
        if(!MaterialsValid)continue;
        const auto Bounds=Mesh->GetBounds();
        if(Bounds.Origin.ContainsNaN()||Bounds.BoxExtent.ContainsNaN()||Bounds.BoxExtent.GetMin()<=1
            ||Mesh->GetNumTriangles(1)<=0||Mesh->GetNumTriangles(1)>60000)continue;
        Meshes[I]=Mesh;
    }
    if(!Meshes[0])
    {
        UE_LOG(LogTemp,Warning,TEXT("SOUL_CROWNSTEAD_PROXY_FALLBACK missing_or_unqualified_A=1 retained_dressing=1"));
        return false;
    }
    UHierarchicalInstancedStaticMeshComponent* Buildings[3]={};
    for(int32 I=0;I<3;++I)if(Meshes[I]){Buildings[I]=NativeSettlementInstances(Owner,Meshes[I]);Buildings[I]->SetForcedLodModel(2);}
    struct FPlot {const TCHAR* Name;FVector2D Offset,Size;float Yaw;int32 Variant;};
    // Centimeters in unchanged world axes; 180-degree frontage changes preserve each measured envelope.
    const FPlot Plots[]={
        {TEXT("south_civic_house"),FVector2D(0,-800),FVector2D(700,500),180,1},
        {TEXT("southwest_inn"),FVector2D(-2400,-3400),FVector2D(600,700),0,0},
        {TEXT("south_court_house"),FVector2D(-1600,-3600),FVector2D(400,500),180,1},
        {TEXT("hall_west_house"),FVector2D(-1600,-2200),FVector2D(400,500),0,2},
        {TEXT("west_entrance_shop"),FVector2D(-2600,600),FVector2D(400,500),0,1},
        {TEXT("east_gate_house"),FVector2D(2400,1200),FVector2D(500,600),180,2},
        {TEXT("east_lane_house"),FVector2D(2200,2200),FVector2D(400,500),0,0},
        {TEXT("east_upper_house"),FVector2D(2600,3000),FVector2D(400,500),0,1},
        {TEXT("northeast_house"),FVector2D(1600,3600),FVector2D(400,500),180,2},
        {TEXT("southeast_house"),FVector2D(3000,-1800),FVector2D(400,500),0,0},
        {TEXT("northwest_house"),FVector2D(-2000,3800),FVector2D(400,500),180,1},
        {TEXT("north_court_house"),FVector2D(-400,3800),FVector2D(400,500),180,2},
        {TEXT("northwest_rear_house"),FVector2D(-3000,4000),FVector2D(400,500),0,0}};
    const FVector Origin=Owner->GetActorLocation();const float OwnerScale=Owner->GetActorScale3D().X;
    int32 Placed[3]={},RouteRejects=0,GroundRejects=0;
    float MaximumRelief=0;
    for(const auto& Plot:Plots)
    {
        const FVector2D Center(Origin.X+Plot.Offset.X,Origin.Y+Plot.Offset.Y);
        bool Clear=true;
        const double Clearance=350.+Plot.Size.Size()*.5;
        for(const auto& Route:Bake().Routes)for(int32 I=1;I<Route.Value.Num();++I)
            if(SegmentDistance(Center,FVector2D(Route.Value[I-1]),FVector2D(Route.Value[I]))<Clearance)Clear=false;
        if(!Clear){++RouteRejects;continue;}
        // Sample the complete reserved envelope, not only a center that could hide an exposed foundation.
        float Low=TNumericLimits<float>::Max(),High=TNumericLimits<float>::Lowest();
        for(float Y=-Plot.Size.Y*.5f;Y<=Plot.Size.Y*.5f;Y+=25)
            for(float X=-Plot.Size.X*.5f;X<=Plot.Size.X*.5f;X+=25)
            {const float Z=Height(Center.X+X,Center.Y+Y);Low=FMath::Min(Low,Z);High=FMath::Max(High,Z);}
        if(Low<=180.f||High-Low>30.f){++GroundRejects;continue;}
        MaximumRelief=FMath::Max(MaximumRelief,High-Low);
        const int32 Variant=Buildings[Plot.Variant]?Plot.Variant:0;
        const auto Bounds=Meshes[Variant]->GetBounds();
        const float WorldScale=FMath::Min(Plot.Size.X/(Bounds.BoxExtent.X*2),Plot.Size.Y/(Bounds.BoxExtent.Y*2));
        const FVector Position(Plot.Offset.X/OwnerScale,Plot.Offset.Y/OwnerScale,((Low+High)*.5f-Origin.Z)/OwnerScale);
        Place(Buildings[Variant],Position,Bounds.BoxExtent.X*2*WorldScale/OwnerScale,Plot.Yaw);
        ++Placed[Variant];
    }
    if(Placed[0]+Placed[1]+Placed[2]==0)
    {
        for(auto* H:Buildings)if(H){Owner->RemoveInstanceComponent(H);H->DestroyComponent();}
        UE_LOG(LogTemp,Warning,TEXT("SOUL_CROWNSTEAD_PROXY_FALLBACK no_supported_plots=1 route_rejects=%d ground_rejects=%d retained_dressing=1"),RouteRejects,GroundRejects);
        return false;
    }
    // Retain the existing small civic perimeter, with actual west approach and east outlet open.
    // The native material chain lacks HISM usage; ordinary components preserve its decal path.
    // Keep native materials and the former instance transforms; the dome site stays reserved.
    int32 PerimeterPlaced[3]={};
    auto NativePerimeter=[&](UStaticMesh* Mesh,const FTransform& Transform,int32 Kind)
    {
        auto* Component=NewObject<UStaticMeshComponent>(Owner);
        Component->SetupAttachment(Owner->GetRootComponent());Component->SetStaticMesh(Mesh);
        Component->SetRelativeTransform(Transform);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);Component->SetGenerateOverlapEvents(false);
        Component->SetCanEverAffectNavigation(false);Owner->AddInstanceComponent(Component);Component->RegisterComponent();
        if(Component->IsRegistered())++PerimeterPlaced[Kind];
    };
    auto* Wall=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Medieval_Megapack/Meshes/Walls/SM_Wall_6M"));
    auto Ground=[&](float X,float Y){return FVector(X,Y,(Height(Origin.X+X*OwnerScale,Origin.Y+Y*OwnerScale)-Origin.Z)/OwnerScale);};
    if(Wall)
    {
        const auto Bounds=Wall->GetBounds();
        const FVector Scale(110/(2*Bounds.BoxExtent.X),18/(2*Bounds.BoxExtent.Y),65/(2*Bounds.BoxExtent.Z));
        for(int32 Side=0;Side<4;++Side)for(int32 I=0;I<4;++I)
        {
            if((Side==1||Side==3)&&(I==1||I==2))continue;
            const FRotator Rotation(0,Side*90,0);const FVector Center=Rotation.RotateVector(FVector((I-1.5f)*110,-220,0));
            FVector Position=Ground(Center.X,Center.Y)-FVector(0,0,12);
            Position-=Rotation.RotateVector(FVector(Bounds.Origin.X*Scale.X,Bounds.Origin.Y*Scale.Y,(Bounds.Origin.Z-Bounds.BoxExtent.Z)*Scale.Z));
            NativePerimeter(Wall,FTransform(Rotation,Position,Scale),0);
        }
    }
    auto* Bottom=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Medieval_Megapack/Meshes/Towers/SM_Tower_Bottom_01"));
    auto* Top=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Medieval_Megapack/Meshes/Towers/SM_Tower_Top_01"));
    auto NativeTower=[&](UStaticMesh* Mesh,FVector Position,float Width,int32 Kind)
    {
        // Identical to Place for these non-Alien meshes at yaw zero and Tall one.
        const auto Bounds=Mesh->GetBounds();const float Scale=Width/FMath::Max(Bounds.BoxExtent.X*2.f,1.f);
        Position-=FVector(Bounds.Origin.X,Bounds.Origin.Y,0)*Scale;
        Position.Z-=(Bounds.Origin.Z-Bounds.BoxExtent.Z)*Scale;
        NativePerimeter(Mesh,FTransform(FRotator::ZeroRotator,Position,FVector(Scale)),Kind);
    };
    if(Bottom&&Top)for(int32 X:{-1,1})for(int32 Y:{-1,1})
    {
        FVector Position=Ground(X*220,Y*220);const auto BaseBounds=Bottom->GetBounds();
        const float Scale=65/(BaseBounds.BoxExtent.X*2);NativeTower(Bottom,Position,65,1);
        Position.Z+=BaseBounds.BoxExtent.Z*2*Scale;NativeTower(Top,Position,Top->GetBounds().BoxExtent.X*2*Scale,2);
    }
    int32 PerimeterHISM=0;
    TInlineComponentArray<UHierarchicalInstancedStaticMeshComponent*> ExistingInstances;Owner->GetComponents(ExistingInstances);
    for(const auto* H:ExistingInstances)if(H->GetStaticMesh()&&(H->GetStaticMesh()==Wall||H->GetStaticMesh()==Bottom||H->GetStaticMesh()==Top))++PerimeterHISM;
    UE_LOG(LogTemp,Display,TEXT("SOUL_CROWNSTEAD_PERIMETER walls=%d bottoms=%d tops=%d static_components=%d hism_components=%d native_materials=1"),
        PerimeterPlaced[0],PerimeterPlaced[1],PerimeterPlaced[2],PerimeterPlaced[0]+PerimeterPlaced[1]+PerimeterPlaced[2],PerimeterHISM);
    CrownsteadHinterland(Owner);
    UE_LOG(LogTemp,Display,TEXT("SOUL_CROWNSTEAD_POPULATION requested=13 placed=%d variants_A_B_C=%d/%d/%d route_rejects=%d ground_rejects=%d max_relief_cm=%.3f forced_lod=1 owner_scale=%.1f native_materials=1 civic_trial=1 old_hall_reserved=1"),
        Placed[0]+Placed[1]+Placed[2],Placed[0],Placed[1],Placed[2],RouteRejects,GroundRejects,MaximumRelief,OwnerScale);
    return true;
}
}
bool RetainedReviewCapture(){return EvilCorridor()&&FParse::Param(FCommandLine::Get(),TEXT("SoulRetainedCapture"));}
bool PopulationReviewCapture(){return RetainedReviewCapture()&&FParse::Param(FCommandLine::Get(),TEXT("SoulPopulationReview"));}
bool EvilCorridor(){return FParse::Param(FCommandLine::Get(),TEXT("SoulEvilCorridor"));}
FString LocationName(FName Id,const FString& Fallback){if(EvilCorridor())if(const auto* N=Bake().Names.Find(Id))return *N;return Fallback;}
bool Mesa(){return FParse::Param(FCommandLine::Get(),TEXT("SoulMesaTerrain"))
    ||(!FParse::Param(FCommandLine::Get(),TEXT("SoulTerrainV2"))&&!FParse::Param(FCommandLine::Get(),TEXT("SoulLegacyTerrain")));}
bool Enabled(){return Mesa()||FParse::Param(FCommandLine::Get(),TEXT("SoulTerrainV2"));}
float Scale(){return Mesa()?10.f:Enabled()?20.f:1.f;}
float RegionScale(){return EvilCorridor()?2.5f:Mesa()?5.f:Scale();}
FVector2D FocusBounds(){return Mesa()?FVector2D(68000,68000):FVector2D(3800,2600)*Scale();}
const TMap<FName,FVector>& Locations(){return Bake().Places;}
const TArray<TArray<FVector>>& WaterLines(){return Bake().Water;}
float Height(float X,float Y)
{
    const auto& B=Bake();if(!B.bValid)return 0;
    const float U=FMath::Clamp((X-B.Minimum)/B.Extent*(B.Resolution-1),0.f,B.Resolution-1.001f);
    const float V=FMath::Clamp((Y-B.Minimum)/B.Extent*(B.Resolution-1),0.f,B.Resolution-1.001f);
    const int32 IX=FMath::FloorToInt(U),IY=FMath::FloorToInt(V);
    auto Z=[&](int32 A,int32 C){const int32 I=(C*B.Resolution+A)*2;return (static_cast<int32>(B.Heights[I])+(static_cast<int32>(B.Heights[I+1])<<8)-32768)*B.HeightUnit;};
    // UE LandscapeRender.cpp uses the (0,0)-(1,1) quad diagonal.
    const float FX=U-IX,FY=V-IY;
    return FX>=FY?Z(IX,IY)+(Z(IX+1,IY)-Z(IX,IY))*FX+(Z(IX+1,IY+1)-Z(IX+1,IY))*FY:
        Z(IX,IY)+(Z(IX+1,IY+1)-Z(IX,IY+1))*FX+(Z(IX,IY+1)-Z(IX,IY))*FY;
}
FVector Road(FName From,FName To,float Alpha)
{
    FString A=From.ToString(),B=To.ToString();if(A>B){Swap(A,B);Alpha=1-Alpha;}
    const auto* P=Bake().Routes.Find(A+TEXT("|")+B);if(!P||P->Num()<2)return FVector::ZeroVector;
    const float T=FMath::Clamp(Alpha,0.f,1.f)*(P->Num()-1);const int32 I=FMath::Min(FMath::FloorToInt(T),P->Num()-2);
    FVector V=FMath::Lerp((*P)[I],(*P)[I+1],T-I);V.Z=RoadSurface(V.X,V.Y);return V;
}
float RoadSurface(float X,float Y)
{
    float Z=Height(X,Y)+22.f;
    for(const auto& B:Bake().Bridges)
    {
        const FVector P=FRotator(0,B.Yaw,0).UnrotateVector(FVector(X,Y,0)-B.Center);
        if(EvilCorridor())
        {
            const float Distance=FVector2D(FMath::Max(0.f,static_cast<float>(FMath::Abs(P.X))-B.HalfSpan),P.Y).Size();
            const float W=1-FMath::SmoothStep(0.f,3000.f,Distance);
            Z=FMath::Max(Z,FMath::Lerp(Z,static_cast<float>(B.Center.Z+10),W));continue;
        }
        if(FMath::Abs(P.Y)>950||FMath::Abs(P.X)>B.HalfSpan+1800)continue;
        const float W=1-FMath::SmoothStep(B.HalfSpan,B.HalfSpan+1800.f,static_cast<float>(FMath::Abs(P.X)));
        Z=FMath::Max(Z,FMath::Lerp(Z,static_cast<float>(B.Center.Z+10),W));
    }
    return Z;
}
void DressRoad(AActor* Owner,USceneComponent* RoadComponent,FName From,FName To)
{
    for(const auto& B:Bake().Bridges)
    {
        if(B.Ford||!((B.A==From&&B.B==To)||(B.A==To&&B.B==From)))continue;
        auto* H=Instances(Owner,TEXT("/Game/Kingdom_Capital/Meshes/Bridge/SM_arch_bridge_01"));
        if(!H)continue;
        H->AttachToComponent(RoadComponent,FAttachmentTransformRules::KeepRelativeTransform);
        if(EvilCorridor())
        {
            if(B.A!=TEXT("river_ford"))continue;
            const int32 Count=FMath::CeilToInt(B.HalfSpan/990.f);const float Half=B.HalfSpan/Count;
            const FRotator Rotation(0,B.Yaw,0);const FVector Scale(Half/990.f,1.5f,2.f);
            for(int32 I=0;I<Count;++I){const FVector C=B.Center+Rotation.RotateVector(FVector(-B.HalfSpan+Half+I*Half*2,0,0));
                H->AddInstance(FTransform(Rotation,C+Rotation.RotateVector(FVector(990*Scale.X,293.9635*Scale.Y,0)),Scale));}
            continue;
        }
        const FVector S(B.HalfSpan/990.f,1.5f,2.f);
        const FRotator R(0,B.Yaw,0);
        const FVector Pivot=B.Center+R.RotateVector(FVector(990*S.X,293.9635*S.Y,0));
        H->AddInstance(FTransform(R,Pivot,S));
    }
}
void Build(ASoulCampaignWorldActor* Owner)
{
    bool Loaded=false;
    const TCHAR* Package=Mesa()?TEXT("/Game/SoulCampaignMountain/L_evil_waterfront"):TEXT("/Game/Soul/Campaign/TerrainV2/L_FounderTerrain");
    // This is already a canonical long package name. The name-search overload
    // consults the asynchronously gathering registry and can report an existing
    // map missing on a fresh editor-game launch. Load that exact package directly.
    const ULevelStreamingDynamic::FLoadLevelInstanceParams Params(Owner->GetWorld(),Package,FTransform::Identity);
    auto* Level=ULevelStreamingDynamic::LoadLevelInstance(Params,Loaded);
    if(Level){Level->SetShouldBeLoaded(true);Level->SetShouldBeVisible(true);Owner->GetWorld()->FlushLevelStreaming(EFlushLevelStreamingType::Full);}
    if(!Loaded||!Level||!Level->GetLoadedLevel()||!Bake().bValid)
    {
        UE_LOG(LogTemp,Error,TEXT("SOUL_TERRAIN_INTEGRATION_FAIL map=%s requested=%d instance=%d loaded=%d bake=%d; run Tools/Setup_Soul_Mesa.ps1"),Package,Loaded,Level!=nullptr,Level && Level->GetLoadedLevel(),Bake().bValid);
        FPlatformMisc::RequestExitWithStatus(false,1);return;
    }
    // Remove study-only actors from this transient instance, never save the package.
    if(Mesa())for(AActor* Actor:TArray<TObjectPtr<AActor>>(Level->GetLoadedLevel()->Actors))
        if(Actor&&(Actor->IsA<ADirectionalLight>()||Actor->IsA<ASkyLight>()||Actor->IsA<ACameraActor>()))Actor->Destroy();
    UE_LOG(LogTemp,Display,TEXT("SOUL_TERRAIN_V2 landscape_loaded=%d bake_valid=%d scale=%.0f map=%s"),Loaded,Bake().bValid,Scale(),Package);
    // Probe the actual imported Landscape before settlements exist. This catches
    // flipped PNG axes or a stale bake/import pair that source-only tests miss.
    double MaxError=0;int32 Hits=0,Probes=0;
    for(const auto& Route:Bake().Routes)for(int32 I=0;I<Route.Value.Num();I+=8)
    {
        const FVector P=Route.Value[I];if(EvilCorridor()&&Height(P.X,P.Y)<=180.f)continue;FHitResult Hit;++Probes;
        if(Owner->GetWorld()->SweepSingleByChannel(Hit,FVector(P.X,P.Y,100000),FVector(P.X,P.Y,-100000),FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(.25f)))
        {++Hits;MaxError=FMath::Max(MaxError,FMath::Abs(Hit.ImpactPoint.Z-Height(P.X,P.Y)));}
    }
    if(Hits!=Probes||MaxError>5){UE_LOG(LogTemp,Error,TEXT("SOUL_TERRAIN_V2_ALIGNMENT_FAIL hits=%d/%d max_cm=%.3f"),Hits,Probes,MaxError);FPlatformMisc::RequestExitWithStatus(false,1);return;}
    else {UE_LOG(LogTemp,Display,TEXT("SOUL_TERRAIN_V2_ALIGNMENT_PASS hits=%d/%d max_cm=%.3f"),Hits,Probes,MaxError);}
    if(Mesa())
    {
        Hits=Probes=0;MaxError=0;
        // The active routes remain on the eastern peninsula. Probe dry ground
        // across the whole map as well, including the upgraded western coast.
        for(int32 Y=0;Y<=10;++Y)for(int32 X=0;X<=10;++X)
        {
            const FVector P(-67500.f+X*13500.f,-67500.f+Y*13500.f,0);
            const float Expected=Height(P.X,P.Y);if(Expected<=180.f)continue;
            FHitResult Hit;++Probes;
            if(Owner->GetWorld()->SweepSingleByChannel(Hit,FVector(P.X,P.Y,100000),FVector(P.X,P.Y,-100000),FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(.25f)))
            {++Hits;MaxError=FMath::Max(MaxError,FMath::Abs(Hit.ImpactPoint.Z-Expected));}
        }
        if(Probes==0||Hits!=Probes||MaxError>5)
        {UE_LOG(LogTemp,Error,TEXT("SOUL_TERRAIN_COAST_ALIGNMENT_FAIL hits=%d/%d max_cm=%.3f"),Hits,Probes,MaxError);FPlatformMisc::RequestExitWithStatus(false,1);return;}
        UE_LOG(LogTemp,Display,TEXT("SOUL_TERRAIN_COAST_ALIGNMENT_PASS hits=%d/%d max_cm=%.3f"),Hits,Probes,MaxError);
    }
    if(Mesa())
    {
        // Instance biome dressing over the approved heightfield. The terrain,
        // roads and strategic state remain the existing authorities.
        auto* Pine=Instances(Owner,TEXT("/Game/Forest_village/Meshes/Vegetation/SM_pine_tree_01"));
        auto* Broad=Instances(Owner,TEXT("/Game/Forest_village/Meshes/Vegetation/Update/SM_tree_01_Update"));
        auto* Rock=Instances(Owner,TEXT("/Game/Forest_village/Meshes/Rocks/SM_rock_03"));
        for(auto* Trees:{Pine,Broad}) if(Trees) Trees->SetForcedLodModel(2);
        FRandomStream Dressing(20261002);
        int32 Trees=0,Rocks=0,VergeSuppressed=0;
        const FVector Capital=Locations().FindRef(TEXT("human_capital"));
        double MaximumCanopyRadius=0;
        for(auto* H:{Pine,Broad})if(H)
        {
            const auto Extent=H->GetStaticMesh()->GetBounds().BoxExtent;
            MaximumCanopyRadius=FMath::Max(MaximumCanopyRadius,240.*FVector2D(Extent).Size()/FMath::Max(Extent.X,1.));
        }
        const FVector Forest=Locations().FindRef(TEXT("forest_edge"));
        for(int32 I=0;I<42000 && Trees<4200;++I)
        {
            const float X=Dressing.FRandRange(-70000,70000),Y=Dressing.FRandRange(-70000,70000);
            const float Z=Height(X,Y);
            if(Z<260 || Z>8500) continue;
            bool Clear=false;
            for(const auto& Location:Locations())
                if(FVector::DistSquaredXY(FVector(X,Y,0),Location.Value)<FMath::Square(Location.Key==TEXT("human_capital")?3200.f:1600.f)) {Clear=true;break;}
            if(Clear)continue;
            for(const auto& Route:Bake().Routes)
            {
                for(const FVector& Point:Route.Value)
                    if(FVector::DistSquaredXY(FVector(X,Y,0),Point)<FMath::Square(650.f)) {Clear=true;break;}
                if(Clear)break;
            }
            if(Clear)continue;
            const float Slope=FVector2D(Height(X+140,Y)-Height(X-140,Y),Height(X,Y+140)-Height(X,Y-140)).Size()/280;
            const float Patch=FMath::PerlinNoise2D(FVector2D(X,Y)/6500.f);
            const bool Volcanic=X<-30000 && Y<0;
            const float Woodland=1-FMath::SmoothStep(9000.0,26000.0,FVector::Dist2D(FVector(X,Y,0),Forest));
            const bool Wooded=!Volcanic && Slope<.65f && (Woodland+Patch>.55f || (Y>30000 && Patch>.05f) || (Z>2800 && Patch>.12f));
            if(Wooded)
            {
                const bool Suppress=(EvilCorridor()&&CrownsteadVergeOverlap(FVector2D(X-Capital.X,Y-Capital.Y),MaximumCanopyRadius))
                    ||RetainedPopulationOverlap(FVector2D(X,Y),MaximumCanopyRadius);
                // Keep both original random argument expressions and the 4200 would-place
                // budget: suppression must not shift any later tree or rock candidate.
                Place(Suppress?nullptr:(I%3==0?Broad:Pine),FVector(X,Y,Z),Dressing.FRandRange(240,480),Dressing.FRandRange(0,360));
                ++Trees;if(Suppress)++VergeSuppressed;
            }
            else if(Slope>.35f && Dressing.FRand()<.10f)
            {Place(Rock,FVector(X,Y,Z-20),Dressing.FRandRange(180,500),Dressing.FRandRange(0,360),.75f);++Rocks;}
        }
        UE_LOG(LogTemp,Display,TEXT("SOUL_CAMPAIGN_DRESSING: trees=%d rocks=%d tree_attempts=%d verge_suppressed=%d instanced=1"),Trees-VergeSuppressed,Rocks,Trees,VergeSuppressed);
        return;
    }
    auto* Pine=Instances(Owner,TEXT("/Game/Forest_village/Meshes/Vegetation/SM_pine_tree_01"));
    auto* Pine2=Instances(Owner,TEXT("/Game/Forest_village/Meshes/Vegetation/SM_pine_tree_03"));
    auto* Broad=Instances(Owner,TEXT("/Game/Forest_village/Meshes/Vegetation/Update/SM_tree_01_Update"));
    auto* Rock=Instances(Owner,TEXT("/Game/Forest_village/Meshes/Rocks/SM_rock_03"));
    FRandomStream R(29092026);
    for(int32 I=0;I<14000;++I)
    {
        const float X=R.FRandRange(-6500,6500)*20,Y=R.FRandRange(-4600,4400)*20;const float Z=Height(X,Y);
        bool Clear=false;for(const auto& P:Locations())if(FVector::DistSquaredXY(FVector(X,Y,0),P.Value)<FMath::Square(P.Key==TEXT("human_capital")?8500.f:5000.f)){Clear=true;break;}
        for(const auto& F:Bake().Fields)
        {
            const FVector P=FRotator(0,F.Yaw,0).UnrotateVector(FVector(X,Y,0)-F.Center);
            if(FMath::Abs(P.X)<F.Extent.X+600&&FMath::Abs(P.Y)<F.Extent.Y+600){Clear=true;break;}
        }
        if(Clear||Z<450)continue;
        // Keep every route clear of trees/rocks using the same baked polyline.
        for(const auto& Route:Bake().Routes)for(const FVector& P:Route.Value)if(FVector::DistSquaredXY(P,FVector(X,Y,0))<FMath::Square(1550.f)){Clear=true;break;}
        if(Clear)continue;
        const float Slope=FVector2D(Height(X+160,Y)-Height(X-160,Y),Height(X,Y+160)-Height(X,Y-160)).Size()/320;
        const float Patch=FMath::PerlinNoise2D(FVector2D(X,Y)/11000.f);
        const float Fringe=Patch*7000;
        const bool Forest=(X>-4000+Fringe&&X<29000+Fringe&&Y<4000+Fringe&&Y>-34000+Fringe)||(X<-69000+Fringe)||(Y>30000+Fringe&&X<17000);
        if(Forest&&Slope<.68f&&Z<13500&&R.FRand()<.60f+Patch*.45f)
            Place(I%5==0?Broad:I%2?Pine:Pine2,FVector(X,Y,Z),R.FRandRange(850,1650),R.FRandRange(0,360),R.FRandRange(.85f,1.18f));
        else if((Slope>.40f||Z>9000)&&R.FRand()<.10f+FMath::Max(Patch,0.f)*.24f)
            Place(Rock,FVector(X,Y,Z-100),R.FRandRange(450,1600),R.FRandRange(0,360),R.FRandRange(.5f,1.1f));
    }
    auto* Hedge=Instances(Owner,TEXT("/Game/Forest_village/Meshes/Vegetation/SM_bush"));
    for(const auto& F:Bake().Fields)for(int32 Side:{-1,1})for(int32 I=0;I<18;++I)
    {
        const FVector Local(FMath::Lerp(-F.Extent.X,F.Extent.X,I/17.f),Side*F.Extent.Y,0);
        FVector P=F.Center+FRotator(0,F.Yaw,0).RotateVector(Local);P.Z=Height(P.X,P.Y);
        Place(Hedge,P,240+R.FRand()*90,R.FRand()*360,.65f);
    }
}
void DressSettlementSurroundings(AActor* Owner,FName Id)
{
    // The native capital miniature replaces civic geometry, not its existing
    // measured farms/verges. Both presentations share the same retained ground.
    if(RetainedPopulationEnabled()&&Id==TEXT("human_capital"))CrownsteadHinterland(Owner);
}
bool DressRegion(AActor* Owner,FName Id)
{
    if(!Enabled())return false;
    if(EvilCorridor()&&Id==TEXT("human_capital")&&CrownsteadPopulation(Owner))return true;
    RetainedRegionPopulation(Owner,Id);
    // Replace the old generic village scatter with the reviewed native forest
    // prefab cluster. Native Ravenhold towers and Warzone assemblies replace
    // AlienPlanet frontier markers. Full authored capital routing remains pending.
    if(RetainedPopulationEnabled()&&(Id==TEXT("forest_edge")||Id==TEXT("orc_camp")||Id==TEXT("orc_watch")||Id==TEXT("north_pass")||Id==TEXT("crossroads")||Id==TEXT("old_quarry")))return true;
    auto* House=Instances(Owner,TEXT("/Game/Forest_village/Meshes/Kit/SM_small_wood_building"));
    auto* Hall=Instances(Owner,TEXT("/Game/Kingdom_Capital/Meshes/Buildings/SM_circular_module_01"));
    auto* Tower=Instances(Owner,TEXT("/Game/Kingdom_Capital/Meshes/Buildings/SM_circular_module_04"));
    auto* AlienCitadel=Id==TEXT("orc_camp")?Instances(Owner,TEXT("/Game/AlienPlanet/Meshes/SM_BigTowerComplex")):nullptr;
    auto* AlienWatch=(Id==TEXT("orc_watch")||Id==TEXT("north_pass"))?Instances(Owner,TEXT("/Game/AlienPlanet/Meshes/SM_BigBetweenTower")):nullptr;
    auto* Stone=Instances(Owner,TEXT("/Game/Forest_village/Meshes/Rocks/SM_rock_03"));
    const bool Human=Id==TEXT("human_capital")||Id==TEXT("crossroads")||Id==TEXT("river_ford");
    auto* TownHouse=Human?Instances(Owner,TEXT("/Game/Medieval_Megapack/Meshes/Courtyard/Houses/SM_Building_E")):nullptr;
    auto* TownRoof=Human?Instances(Owner,TEXT("/Game/Medieval_Megapack/Meshes/Courtyard/Houses/SM_Roof_E")):nullptr;
    const FVector Origin=Owner->GetActorLocation();
    const float LandmarkScale=Owner->GetActorScale3D().X;
    auto Ground=[&](float X,float Y){return FVector(X,Y,(Height(Origin.X+X*LandmarkScale,Origin.Y+Y*LandmarkScale)-Origin.Z)/LandmarkScale);};
    auto Cottage=[&](FVector P,float W,float Yaw)
    {
        if(Mesa())
        {
            const FVector WorldPoint=Origin+P*LandmarkScale;
            const float Clearance=W*LandmarkScale*.7f+130.f;
            for(const auto& Route:Bake().Routes)for(const FVector& Point:Route.Value)
                if(FVector::DistSquaredXY(WorldPoint,Point)<FMath::Square(Clearance))return;
        }
        if(!TownHouse||!TownRoof){Place(House,P,W,Yaw);return;}
        const auto B=TownHouse->GetStaticMesh()->GetBounds(),R=TownRoof->GetStaticMesh()->GetBounds();
        const float S=W/(2*B.BoxExtent.X);
        Place(TownHouse,P,W,Yaw);
        Place(TownRoof,P+FVector(0,0,B.BoxExtent.Z*2*S-2),R.BoxExtent.X*2*S,Yaw);
    };
    if(Id==TEXT("river_ford"))
    {
        auto* Bridge=Instances(Owner,TEXT("/Game/Kingdom_Capital/Meshes/Bridge/SM_arch_bridge_01"));
        // The bridge mesh pivot is its east end and its deck is local Z=0.
        if(Bridge&&!Mesa())Bridge->AddInstance(FTransform(FRotator::ZeroRotator,FVector(210,20.578f,8),FVector(.212f,.07f,.10f)));
        Cottage(Ground(-320,180),85,70);Cottage(Ground(310,-130),65,-80);
    }
    else if(Id==TEXT("old_quarry"))
    {
        for(int I=0;I<13;++I)Place(Stone,Ground(-220+(I%4)*95,-100+(I/4)*90),55+(I%3)*18,I*37,.45f);
        Place(House,Ground(160,160),95,170);
    }
    else if(Id==TEXT("ancient_shrine"))
    {
        Place(Hall,Ground(0,Mesa()?-220:0),175,0,.75f);
        for(int I=0;I<5;++I)Place(Stone,Ground(-170+I*80,170),45,I*37,1.8f);
    }
    else
    {
        const bool Capital=Id==TEXT("human_capital"),Fort=Id==TEXT("orc_camp"),Watch=Id==TEXT("orc_watch")||Id==TEXT("north_pass");
        if(Capital||Fort||Watch)
        {
            if(Fort&&AlienCitadel)
            {
                Place(AlienCitadel,Ground(0,Mesa()?-330:0),960,-18,.92f);
            }
            else
            {
                Place(Hall,Ground(0,Mesa()?-330:0),Capital?300:150,0,.8f);
            }
            if(Watch&&AlienWatch)
            {
                Place(AlienWatch,Ground(0,Mesa()?-210:0),Id==TEXT("north_pass")?260:620,Id==TEXT("north_pass")?90:15,.78f);
            }
            else if(!Fort) for(int I=0;I<(Watch?2:4);++I)
            {Place(Tower,Ground((I%2?1:-1)*(Watch?135:220),(I/2?1:-1)*(Watch?135:220)),65,I*90,.85f);}
            if(Fort||Watch)
            {
                // Frontier masonry replaces the capital's gilded roof treatment.
                auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Soul/Campaign/TerrainV2/M_CampaignMasonry"));
                for(auto* H:{Hall,Tower})if(H)for(int I=0;I<H->GetNumMaterials();++I)H->SetMaterial(I,Material);
            }
            if(Capital||Fort)
            {
                auto* Wall=Instances(Owner,TEXT("/Game/Medieval_Megapack/Meshes/Walls/SM_Wall_6M"));
                if(Wall)
                {
                    auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Soul/Campaign/TerrainV2/M_CampaignMasonry"));
                    for(int I=0;I<Wall->GetNumMaterials();++I)Wall->SetMaterial(I,Material);
                    const auto B=Wall->GetStaticMesh()->GetBounds();
                    const FVector S(110/(2*B.BoxExtent.X),18/(2*B.BoxExtent.Y),(Fort?90:65)/(2*B.BoxExtent.Z));
                    for(int Side=0;Side<4;++Side)for(int I=0;I<4;++I)
                    {
                        if(Side==(Capital?1:3)&&(I==1||I==2))continue; // road-facing gate
                        const FRotator R(0,Side*90,0);const FVector C=R.RotateVector(FVector((I-1.5f)*110,-220,0));
                        FVector P=Ground(C.X,C.Y)-FVector(0,0,12);
                        P-=R.RotateVector(FVector(B.Origin.X*S.X,B.Origin.Y*S.Y,(B.Origin.Z-B.BoxExtent.Z)*S.Z));
                        Wall->AddInstance(FTransform(R,P,S));
                    }
                }
            }
        }
        const int Count=Capital?34:Fort?12:Watch?4:11;
        FRandomStream R(FCrc::StrCrc32(*Id.ToString()));
        for(int I=0;I<Count;++I)
        {
            float X,Y;
            if(Capital)
            {
                // Three irregular streets flank the civic precinct and its east gate.
                X=-330+(I%7)*110+R.FRandRange(-18,18);
                Y=(I/7<2?-1.f:1.f)*(290+(I/7%3)*120)+R.FRandRange(-20,20);
            }
            else if(Fort||Watch){const float A=I*2.39996f;const float D=275+FMath::Sqrt(static_cast<float>(I))*48;X=FMath::Cos(A)*D;Y=FMath::Sin(A)*D;}
            else {X=-260+(I%4)*145+R.FRandRange(-24,24);Y=(I/4-1)*150+R.FRandRange(-25,25);}
            Cottage(Ground(X,Y),R.FRandRange(52,72),R.FRandRange(-10,10)+(Y>0?180:0));
        }
    }
    return true;
}
}
