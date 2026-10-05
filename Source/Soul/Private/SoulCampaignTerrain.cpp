#include "SoulCampaignTerrain.h"
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
}
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
        int32 Trees=0,Rocks=0;
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
            {Place(I%3==0?Broad:Pine,FVector(X,Y,Z),Dressing.FRandRange(240,480),Dressing.FRandRange(0,360));++Trees;}
            else if(Slope>.35f && Dressing.FRand()<.10f)
            {Place(Rock,FVector(X,Y,Z-20),Dressing.FRandRange(180,500),Dressing.FRandRange(0,360),.75f);++Rocks;}
        }
        UE_LOG(LogTemp,Display,TEXT("SOUL_CAMPAIGN_DRESSING: trees=%d rocks=%d instanced=1"),Trees,Rocks);
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
bool DressRegion(AActor* Owner,FName Id)
{
    if(!Enabled())return false;
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
