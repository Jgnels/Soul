#include "SoulTitanPilotBuildCommandlet.h"
#include "SoulTitanPilot.h"
#include "AssetCompilingManager.h"
#include "ActorFactories/ActorFactory.h"
#include "Builders/CubeBuilder.h"
#include "Camera/CameraActor.h"
#include "Components/BrushComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "LevelInstance/LevelInstanceSubsystem.h"
#include "Logging/MessageLog.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavMesh/RecastNavMesh.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "UObject/UnrealType.h"

namespace
{
class FPilotMapCheckLog final : public FOutputDevice
{
public:
    int32 Errors = 0, Warnings = 0;
    virtual void Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category) override
    {
        if (Category != TEXT("MapCheck")) return;
        if (Verbosity == ELogVerbosity::Error || FString(Message).StartsWith(TEXT("Error:"))) ++Errors;
        if (Verbosity == ELogVerbosity::Warning || FString(Message).StartsWith(TEXT("Warning:"))) ++Warnings;
    }
};
}

USoulTitanPilotBuildCommandlet::USoulTitanPilotBuildCommandlet()
{
    IsEditor = true; IsClient = false; IsServer = false; LogToConsole = true;
}
int32 USoulTitanPilotBuildCommandlet::Main(const FString& Params)
{
    const FString File = FPaths::ProjectContentDir()/TEXT("Soul/Maps/Soul_TitanPilot.umap");
    if (IFileManager::Get().FileExists(*File))
    {
        UE_LOG(LogTemp, Error, TEXT("Pilot already exists; refuse overwrite. Preserve map before rebuilding."));
        return 1;
    }
    UWorld* World = GEditor->NewMap(false);
    if (!World) return 2;
    World->GetWorldSettings()->DefaultGameMode = ASoulTitanPilotGameMode::StaticClass();
    auto* Instance = World->SpawnActor<ALevelInstance>();
    Instance->SetActorLabel(TEXT("Soul_Clifftop_Mine_Donor"));
    Instance->SetDesiredRuntimeBehavior(ELevelInstanceRuntimeBehavior::LevelStreaming);
    if (!Instance->SetWorldAsset(TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Environment/Clifftop/Level_Instance/IL_Clifftop_Mine_TownGrid.IL_Clifftop_Mine_TownGrid"))))) return 3;
    auto* Subsystem = World->GetSubsystem<ULevelInstanceSubsystem>();
    Subsystem->BlockLoadLevelInstance(Instance);
    FAssetCompilingManager::Get().FinishAllCompilation();
    FBox Bounds(ForceInit);
    int32 Actors = 0, Meshes = 0;
    Subsystem->ForEachActorInLevelInstance(Instance, [&](AActor* Actor)
    {
        ++Actors;
        TArray<UStaticMeshComponent*> Components; Actor->GetComponents(Components);
        for (auto* Component : Components) if (Component->GetStaticMesh())
        {
            Component->UpdateBounds(); Bounds += Component->Bounds.GetBox(); ++Meshes;
        }
        return true;
    });
    UE_LOG(LogTemp, Display, TEXT("SOUL_TITAN_DONOR_LOADED actors=%d meshes=%d bounds=%s"), Actors, Meshes, *Bounds.ToString());
    if (!Bounds.IsValid || Meshes < 200) return 4;
    // Keep donor packages byte-identical. Normalize only the Soul-owned instance transform.
    const FVector Offset(-Bounds.GetCenter().X, -Bounds.GetCenter().Y, -Bounds.Min.Z);
    Instance->SetActorLocation(Offset);
    Instance->PostEditMove(true);
    Bounds = Bounds.ShiftBy(Offset);
    TArray<TSharedPtr<FJsonValue>> Geometry;
    Subsystem->ForEachActorInLevelInstance(Instance, [&](AActor* Actor)
    {
        TArray<UStaticMeshComponent*> Components; Actor->GetComponents(Components);
        for (auto* Component : Components) if (Component->GetStaticMesh())
        {
            Component->UpdateBounds();
            auto Row=MakeShared<FJsonObject>();
            Row->SetStringField(TEXT("mesh"), Component->GetStaticMesh()->GetPathName());
            Row->SetStringField(TEXT("actor"),Actor->GetActorLabel());
            Row->SetStringField(TEXT("bounds"),Component->Bounds.GetBox().ToString());
            Row->SetStringField(TEXT("profile"),Component->GetCollisionProfileName().ToString());
            Row->SetNumberField(TEXT("pawn_response"),int32(Component->GetCollisionResponseToChannel(ECC_Pawn)));
            Row->SetNumberField(TEXT("collision_enabled"),int32(Component->GetCollisionEnabled()));
            Geometry.Add(MakeShared<FJsonValueObject>(Row));
        }
        return true;
    });
    FString GeometryJson;
    FJsonSerializer::Serialize(Geometry,TJsonWriterFactory<>::Create(&GeometryJson));
    FFileHelper::SaveStringToFile(GeometryJson,*(FPaths::ProjectDir()/TEXT("Evidence/TitanPilot-20261005/donor_component_geometry.json")));
    const FVector Extent = Bounds.GetExtent() + FVector(1800,1800,500);
    auto* Ground = World->SpawnActor<AStaticMeshActor>(FVector(0,0,-50), FRotator::ZeroRotator);
    Ground->SetActorLabel(TEXT("Soul_Pilot_TraversalGround"));
    Ground->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    Ground->SetActorScale3D(FVector(Extent.X*2/100, Extent.Y*2/100, 1));
    Ground->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
    Ground->GetStaticMeshComponent()->SetMobility(EComponentMobility::Static);
    auto* Start = World->SpawnActor<APlayerStart>(FVector(Bounds.Min.X-1000, Bounds.Min.Y-1000, 100), FRotator::ZeroRotator);
    Start->SetActorLabel(TEXT("Soul_PlayerStart"));
    auto* Light = World->SpawnActor<ADirectionalLight>(FVector(0,0,5000), FRotator(-55,-35,0));
    Light->SetActorLabel(TEXT("Soul_Pilot_Sun"));
    Light->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Light->GetLightComponent()->SetIntensity(3.f);
    CastChecked<UDirectionalLightComponent>(Light->GetLightComponent())->SetAtmosphereSunLight(true);
    auto* Atmosphere = World->SpawnActor<AActor>();
    Atmosphere->SetActorLabel(TEXT("Soul_Pilot_Sky"));
    auto* AtmosphereComponent = NewObject<USkyAtmosphereComponent>(Atmosphere);
    Atmosphere->SetRootComponent(AtmosphereComponent);
    Atmosphere->AddInstanceComponent(AtmosphereComponent);
    AtmosphereComponent->RegisterComponent();
    auto* Sky = World->SpawnActor<ASkyLight>();
    Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Sky->GetLightComponent()->SetIntensity(0.6f);
    auto* Camera = World->SpawnActor<ACameraActor>(FVector(-Extent.X*1.3,-Extent.Y*1.3,Extent.GetMax()*1.0), FRotator(-33,45,0));
    Camera->SetActorLabel(TEXT("Soul_Pilot_Overview"));
    auto* Volume = World->SpawnActor<ANavMeshBoundsVolume>(FVector(0,0,Bounds.GetCenter().Z), FRotator::ZeroRotator);
    Volume->SetActorLabel(TEXT("Soul_Pilot_NavigationBounds"));
    auto* Builder = NewObject<UCubeBuilder>(Volume);
    Builder->X = Extent.X*2; Builder->Y = Extent.Y*2; Builder->Z = Extent.Z*2+1000;
    UActorFactory::CreateBrushForVolumeActor(Volume, Builder);
    Volume->PostEditChange();
    auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
    if (!Nav)
    {
        FNavigationSystem::AddNavigationSystemToWorld(*World, FNavigationSystemRunMode::EditorMode);
        Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
    }
    if (!Nav) return 5;
    Nav->OnNavigationBoundsUpdated(Volume);
    auto* Recast = Cast<ARecastNavMesh>(Nav->GetDefaultNavDataInstance(FNavigationSystem::Create));
    if (!Recast) return 6;
    // Runtime rebuild also includes streamed geometry; the navigation authority
    // and bounds are owned by this persistent Soul world, never donor nav data.
    auto* RuntimeMode = FindFProperty<FEnumProperty>(ANavigationData::StaticClass(), TEXT("RuntimeGeneration"));
    if (!RuntimeMode) return 7;
    RuntimeMode->GetUnderlyingProperty()->SetIntPropertyValue(RuntimeMode->ContainerPtrToValuePtr<void>(Recast), uint64(ERuntimeGenerationType::Dynamic));
    Recast->SetActorLabel(TEXT("Soul_Pilot_RecastNavMesh"));
    World->FlushLevelStreaming(EFlushLevelStreamingType::Full);
    Nav->RemoveNavigationBuildLock(ENavigationBuildLock::AsyncLoadLock);
    Nav->Build();
    for (int32 Tick=0; Tick<3000 && Nav->IsNavigationBuildInProgress(); ++Tick)
    {
        Nav->Tick(0.033f); FPlatformProcess::Sleep(0.01f);
    }
    UE_LOG(LogTemp, Display, TEXT("SOUL_TITAN_NAV_BUILD remaining=%d"), Nav->GetNumRemainingBuildTasks());
    if (Nav->IsNavigationBuildInProgress()) return 8;
    // The donor is raised architecture with no surrounding Titan terrain.
    // Spawn on its walkable terrace, not on the staging floor underneath it.
    FNavLocation Terrace;
    int32 BestConnected = 0;
    int32 ProjectedCandidates = 0, EligibleCandidates = 0;
    double BestSpawnScore = TNumericLimits<double>::Max();
    for (int32 X=-3000; X<=3000; X+=1000) for (int32 Y=-3000; Y<=3000; Y+=1000)
    for (int32 Z=600; Z<=3000; Z+=200)
    {
        FNavLocation Candidate;
        if (!Nav->ProjectPointToNavigation(FVector(X,Y,Z), Candidate, FVector(100,100,120)) || Candidate.Location.Z<300) continue;
        ++ProjectedCandidates;
        auto Reachable = [&](const FVector& Point)
        {
            FNavLocation End;
            if (!Nav->ProjectPointToNavigation(Point, End, FVector(150,150,250))) return false;
            auto* Path = UNavigationSystemV1::FindPathToLocationSynchronously(World, Candidate.Location, End.Location);
            return Path && Path->IsValid() && !Path->IsPartial();
        };
        int32 LocalRoutes = 0;
        for (const FVector ProbeOffset : {FVector(600,0,0), FVector(0,600,0), FVector(1000,1000,0)})
            if (Reachable(Candidate.Location+ProbeOffset)) ++LocalRoutes;
        if (LocalRoutes < 2) continue;
        ++EligibleCandidates;
        int32 Connected = 0;
        // Five-metre samples capture this narrow, connected terrace network;
        // ten-metre spacing misses much of the actual walkable surface.
        for (int32 TX=-4000; TX<=4000; TX+=500) for (int32 TY=-4000; TY<=4000; TY+=500)
            if (Reachable(FVector(TX,TY,Candidate.Location.Z))) ++Connected;
        const double Score = FVector::DistSquared2D(Candidate.Location,FVector(-1500,-1500,0));
        if (Connected>BestConnected || (Connected==BestConnected && Score<BestSpawnScore))
        { Terrace=Candidate; BestConnected=Connected; BestSpawnScore=Score; }
    }
    if (BestConnected < 10)
    {
        UE_LOG(LogTemp, Error, TEXT("SOUL_TITAN_NAV_BUILD no upper donor terrace projected=%d eligible=%d best_connected=%d best=%s"),ProjectedCandidates,EligibleCandidates,BestConnected,*Terrace.Location.ToString()); return 8;
    }
    Start->SetActorLocation(Terrace.Location + FVector(0,0,100));
    UE_LOG(LogTemp, Display, TEXT("SOUL_TITAN_TERRACE_START nav=%s connected_samples=%d"), *Terrace.Location.ToString(), BestConnected);
    FNavLocation SpawnNav;
    if (!Nav->ProjectPointToNavigation(Start->GetActorLocation(), SpawnNav, FVector(200,200,300)))
    {
        UE_LOG(LogTemp, Error, TEXT("SOUL_TITAN_NAV_BUILD spawn projection failed")); return 8;
    }
    FMessageLog(TEXT("MapCheck")).NewPage(FText::FromString(TEXT("Soul Titan Pilot")));
    FPilotMapCheckLog CheckLog;
    GLog->AddOutputDevice(&CheckLog);
    const bool Checked = GEditor->Exec(World, TEXT("MAP CHECK DONTDISPLAYDIALOG"), *GLog);
    GLog->RemoveOutputDevice(&CheckLog);
    if (!Checked) return 9;
    const int32 Errors = CheckLog.Errors;
    const int32 Warnings = CheckLog.Warnings;
    UE_LOG(LogTemp, Display, TEXT("SOUL_TITAN_MAP_CHECK errors=%d warnings_or_worse=%d"), Errors, Warnings);
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(File), true);
    const bool Saved = FEditorFileUtils::SaveMap(World, File);
    UE_LOG(LogTemp, Display, TEXT("SOUL_TITAN_MAP_SAVED success=%d path=%s offset=%s bounds=%s"), Saved, *File, *Offset.ToString(), *Bounds.ToString());
    return Saved && Errors == 0 ? 0 : 9;
}
