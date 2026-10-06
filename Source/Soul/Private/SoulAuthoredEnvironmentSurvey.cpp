#include "SoulAuthoredSettlementQualification.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "LevelInstance/LevelInstanceInterface.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "Engine/Level.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Json.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

namespace
{
TArray<TSharedPtr<FJsonValue>> HumanVector(const FVector& V)
{
    return {MakeShared<FJsonValueNumber>(V.X), MakeShared<FJsonValueNumber>(V.Y), MakeShared<FJsonValueNumber>(V.Z)};
}
void HumanTransform(const FTransform& T, TSharedPtr<FJsonObject> Row)
{
    Row->SetArrayField(TEXT("location"), HumanVector(T.GetLocation()));
    Row->SetArrayField(TEXT("scale"), HumanVector(T.GetScale3D()));
    const FRotator R = T.Rotator();
    Row->SetArrayField(TEXT("rotation"), HumanVector(FVector(R.Pitch, R.Yaw, R.Roll)));
}
bool ExportHumanArchitecture(UWorld* World, const FString& Prefix)
{
    // Native authored placements and lowest render LODs only. This observer
    // produces source data, never a replacement city or a saved donor edit.
    TArray<TSharedPtr<FJsonValue>> Instances, Families, Ground;
    auto Meshes = MakeShared<FJsonObject>();
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (const auto* Instance = Cast<ILevelInstanceInterface>(*It))
            if (const ULevel* Loaded = Instance->GetLoadedLevel())
            {
                auto Row = MakeShared<FJsonObject>();
                Row->SetStringField(TEXT("actor"), It->GetPathName());
                Row->SetStringField(TEXT("source"), Instance->GetWorldAssetPackage());
                Row->SetStringField(TEXT("loaded_level"), Loaded->GetOutermost()->GetName());
                Row->SetStringField(TEXT("parent_level"), It->GetLevel()->GetOutermost()->GetName());
                HumanTransform(It->GetActorTransform(), Row);
                Families.Add(MakeShared<FJsonValueObject>(Row));
            }
        TInlineComponentArray<UStaticMeshComponent*> Components;
        It->GetComponents(Components);
        for (const auto* Component : Components)
        {
            const UStaticMesh* Mesh = Component->GetStaticMesh();
            if (!Mesh) continue;
            const FString Path = Mesh->GetPathName();
            if (!(Path.StartsWith(TEXT("/Game/CastleTown/Static_Mesh/Village/Building_Kit/"))
                || Path.StartsWith(TEXT("/Game/CastleTown/Static_Mesh/Castle_Kit/"))
                || Path.StartsWith(TEXT("/Game/CastleTown/Static_Mesh/DockWalls/"))
                || Path.StartsWith(TEXT("/Game/CastleTown/Static_Mesh/Brick_Kit/")))) continue;
            if (!Meshes->HasField(Path))
            {
                auto Detail = MakeShared<FJsonObject>();
                const int32 LOD = FMath::Max(0, Mesh->GetNumLODs() - 1);
                Detail->SetNumberField(TEXT("lod"), LOD);
                Detail->SetNumberField(TEXT("triangles"), Mesh->GetNumTriangles(LOD));
                Meshes->SetObjectField(Path, Detail);
            }
            auto Add = [&](const FTransform& Transform)
            {
                auto Row = MakeShared<FJsonObject>();
                Row->SetStringField(TEXT("actor"), It->GetPathName());
                Row->SetStringField(TEXT("level"), It->GetLevel()->GetOutermost()->GetName());
                Row->SetStringField(TEXT("mesh"), Path);
                Row->SetStringField(TEXT("component"), Component->GetName());
                TArray<TSharedPtr<FJsonValue>> Materials;
                for (int32 Index = 0; Index < Component->GetNumMaterials(); ++Index)
                    Materials.Add(MakeShared<FJsonValueString>(GetPathNameSafe(Component->GetMaterial(Index))));
                Row->SetArrayField(TEXT("materials"), Materials);
                HumanTransform(Transform, Row);
                Instances.Add(MakeShared<FJsonValueObject>(Row));
            };
            if (const auto* Instanced = Cast<UInstancedStaticMeshComponent>(Component))
                for (int32 Index = 0; Index < Instanced->GetInstanceCount(); ++Index)
                {
                    FTransform Transform;
                    if (Instanced->GetInstanceTransform(Index, Transform, true)) Add(Transform);
                }
            else Add(Component->GetComponentTransform());
        }
    }
    // Coarse read-only collision observations, not a playable-arena acceptance.
    // The exact formation and reinforcement checks still precede real battle.
    FCollisionObjectQueryParams Objects(ECC_WorldStatic);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(SoulHumanSourceGround), false);
    for (int32 X = -40000; X <= 40000; X += 1000)
        for (int32 Y = -40000; Y <= 40000; Y += 1000)
        {
            FHitResult Hit;
            if (!World->LineTraceSingleByObjectType(Hit, FVector(X,Y,15000), FVector(X,Y,-6000), Objects, Params)) continue;
            auto Row = MakeShared<FJsonObject>();
            Row->SetArrayField(TEXT("position"), HumanVector(Hit.ImpactPoint));
            Row->SetNumberField(TEXT("normal_z"), Hit.ImpactNormal.Z);
            Row->SetStringField(TEXT("actor"), GetPathNameSafe(Hit.GetActor()));
            Row->SetStringField(TEXT("component"), GetPathNameSafe(Hit.GetComponent()));
            Ground.Add(MakeShared<FJsonValueObject>(Row));
        }
    auto Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("scope"), TEXT("Actual streamed architecture and transforms; no donor changes; ground grid is not battle qualification"));
    Root->SetArrayField(TEXT("instances"), Instances);
    Root->SetArrayField(TEXT("level_instances"), Families);
    Root->SetObjectField(TEXT("meshes"), Meshes);
    Root->SetArrayField(TEXT("ground_grid"), Ground);
    FString Text;
    FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Text));
    UE_LOG(LogTemp, Display, TEXT("SOUL_HUMAN_ARCHITECTURE_EXPORT instances=%d families=%d meshes=%d ground=%d"),
        Instances.Num(), Families.Num(), Meshes->Values.Num(), Ground.Num());
    return FFileHelper::SaveStringToFile(Text, *(FPaths::ProjectSavedDir() / (Prefix + TEXT("_architecture.json"))));
}
}

// Read-only, opt-in source-content diagnostic. It observes the existing native
// world and its authored cameras; it never initializes a campaign, saves a
// package or binds an unqualified city to the playable settlement scenario.
void USoulAuthoredSettlementQualification::TickHumanEnvironmentSurvey(double Now)
{
    auto* World = GetWorld();
    auto* PC = World->GetFirstPlayerController();
    if (Step == 0)
    {
        FString UserDir;
        // The launcher chooses the map; this observer never loads a package
        // or adds the unqualified Human environment to production cook roots.
        if (!Check(World->GetOutermost()->GetName().StartsWith(TEXT("/Game/Soul/Maps/Settlements/"))
            && World->GetFName() == TEXT("L_HumanCapital_Authored") && World->GetAuthGameMode()
            && World->GetAuthGameMode()->GetClass() == AGameModeBase::StaticClass(),
            TEXT("Human survey uses the intact owned map and neutral native game mode"))
            || !Check(FParse::Value(FCommandLine::Get(), TEXT("UserDir="), UserDir)
                && !FPaths::IsRelative(UserDir)
                && FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),
                    FPaths::ConvertRelativePathToFull(UserDir)), TEXT("isolated Human survey user directory"))
            || !Check(FParse::Value(FCommandLine::Get(), TEXT("SoulCampaignCapturePrefix="), Prefix)
                && !Prefix.IsEmpty() && Prefix == FPaths::MakeValidFileName(Prefix), TEXT("explicit Human survey prefix"))) return;
        Next(1);
        return;
    }
    int32 PendingLevels = 0;
    for (const auto* Level : World->GetStreamingLevels())
        if (Level && (Level->IsStreamingStatePending()
            || (Level->ShouldBeLoaded() && !Level->IsLevelLoaded())
            || (Level->ShouldBeVisible() && !Level->IsLevelVisible()))) ++PendingLevels;
    if (PendingLevels > 0 || IsAsyncLoading())
    {
        bHumanStreamingReady = false;
        AssetsQuietSince = 0;
        if (Now >= NextAssetWaitLog)
        {
            UE_LOG(LogTemp, Display, TEXT("SOUL_HUMAN_STREAMING_WAIT levels=%d pending=%d async=%d"),
                World->GetStreamingLevels().Num(), PendingLevels, IsAsyncLoading());
            NextAssetWaitLog = Now + 10;
        }
        if (Now - StepStarted > 1200) Check(false, TEXT("Human streaming did not finish within twenty minutes"));
        NextTime = Now + .5;
        return;
    }
    if (!bHumanStreamingReady) { bHumanStreamingReady = true; StepStarted = Now; }
    if (!AwaitVisualAssets(Now) || !PC) return;
    if (Step == 1)
    {
        if (!Check(ExportHumanArchitecture(World, Prefix), TEXT("native architectural placements and collision grid recorded"))) return;
        int32 Actors = 0, Cameras = 0, Loaded = 0;
        FString Report = TEXT("Read-only native runtime survey; not a settlement-development or battle acceptance.\n");
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            ++Actors;
            if (It->IsA<ACameraActor>())
            {
                ++Cameras;
                Report += FString::Printf(TEXT("camera=%s position=%s\n"), *It->GetName(), *It->GetActorLocation().ToString());
            }
        }
        TArray<TSharedPtr<FJsonValue>> ActorRows;
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            auto Row = MakeShared<FJsonObject>();
            Row->SetStringField(TEXT("path"), It->GetPathName());
            Row->SetStringField(TEXT("class"), It->GetClass()->GetPathName());
#if WITH_EDITOR
            Row->SetStringField(TEXT("label"), It->GetActorLabel());
#endif
            Row->SetStringField(TEXT("level"), It->GetLevel()->GetOutermost()->GetName());
            Row->SetStringField(TEXT("transform"), It->GetActorTransform().ToString());
            Row->SetStringField(TEXT("parent"), GetPathNameSafe(It->GetAttachParentActor()));
            FVector Origin, Extent;
            It->GetActorBounds(false, Origin, Extent);
            Row->SetStringField(TEXT("bounds_origin"), Origin.ToString());
            Row->SetStringField(TEXT("bounds_extent"), Extent.ToString());
            TInlineComponentArray<UStaticMeshComponent*> Components;
            It->GetComponents(Components);
            TArray<TSharedPtr<FJsonValue>> Meshes;
            for (const auto* Component : Components)
                if (Component->GetStaticMesh())
                    Meshes.Add(MakeShared<FJsonValueString>(Component->GetStaticMesh()->GetPathName()));
            Row->SetArrayField(TEXT("meshes"), Meshes);
            ActorRows.Add(MakeShared<FJsonValueObject>(Row));
        }
        auto Inventory = MakeShared<FJsonObject>();
        Inventory->SetStringField(TEXT("scope"), TEXT("read-only loaded runtime actors, no soft source-world resolution or donor edits"));
        Inventory->SetArrayField(TEXT("actors"), ActorRows);
        FString InventoryText;
        FJsonSerializer::Serialize(Inventory, TJsonWriterFactory<>::Create(&InventoryText));
        if (!Check(FFileHelper::SaveStringToFile(InventoryText, *(FPaths::ProjectSavedDir() / (Prefix + TEXT("_actors.json")))),
            TEXT("fully streamed runtime actor inventory recorded"))) return;
        for (const auto* Level : World->GetStreamingLevels())
            if (Level)
            {
                const bool Ready = Level->GetLoadedLevel() && Level->IsLevelVisible();
                Loaded += Ready;
                Report += FString::Printf(TEXT("level=%s loaded_visible=%d\n"), *Level->GetWorldAssetPackageName(), Ready);
            }
        Report += FString::Printf(TEXT("actors=%d cameras=%d loaded_visible_levels=%d streaming_levels=%d\n"),
            Actors, Cameras, Loaded, World->GetStreamingLevels().Num());
        if (!Check(FFileHelper::SaveStringToFile(Report, *(FPaths::ProjectSavedDir() / (Prefix + TEXT("_native_world.txt")))), TEXT("native Human inventory recorded"))
            || !Check(Actors >= 3000 && Cameras >= 17 && Loaded >= 10,
                TEXT("substantial authored Human world and original sublevels loaded"))) return;
        UE_LOG(LogTemp, Display, TEXT("SOUL_HUMAN_ENVIRONMENT_READY actors=%d cameras=%d levels=%d"), Actors, Cameras, Loaded);
        Next(1);
        return;
    }
    const TCHAR* CameraNames[] = {TEXT("CineCameraActor_6"), TEXT("CineCameraActor_7"), TEXT("CineCameraActor_9")};
    const TCHAR* Labels[] = {TEXT("human_castle"), TEXT("human_market"), TEXT("human_houses")};
    if (Step >= 2 && Step <= 7)
    {
        const int32 Index = (Step - 2) / 2;
        if (Step % 2 == 0)
        {
            ACameraActor* Camera = nullptr;
            for (TActorIterator<ACameraActor> It(World); It; ++It)
                if (It->GetFName() == FName(CameraNames[Index])) { Camera = *It; break; }
            if (!Check(Camera != nullptr, TEXT("original authored camera exists"))) return;
            PC->SetViewTarget(Camera);
            Next(12);
        }
        else
        {
            if (!Check(PC->GetViewTarget() && PC->GetViewTarget()->GetFName() == FName(CameraNames[Index]),
                TEXT("authored camera still active at capture"))) return;
            Capture(Labels[Index]); Next(2);
        }
        return;
    }
    if (Step == 8)
    {
        // An unsaved inspection camera shows the whole authored city. It is
        // not a campaign footprint, settlement placement or player camera.
        const FVector Position(-35000, 42000, 30000), Target(-2500, 3500, 0);
        FActorSpawnParameters Params;
        Params.Name = TEXT("SoulHumanInspectionOverview");
        Params.ObjectFlags |= RF_Transient;
        auto* Camera = World->SpawnActor<ACameraActor>(Position, (Target - Position).Rotation(), Params);
        if (!Check(Camera != nullptr, TEXT("transient whole-city inspection camera"))) return;
        Camera->GetCameraComponent()->SetFieldOfView(65);
        PC->SetViewTarget(Camera);
        Next(12);
        return;
    }
    if (Step == 9)
    {
        if (!Check(PC->GetViewTarget() && PC->GetViewTarget()->GetFName() == TEXT("SoulHumanInspectionOverview"),
            TEXT("whole-city inspection camera active"))) return;
        Capture(TEXT("human_overview")); Next(2); return;
    }
    UE_LOG(LogTemp, Display, TEXT("SOUL_HUMAN_ENVIRONMENT_SURVEY_COMPLETE rendered_capture_requests=%d gameplay_proof=0"), Captures.Num());
    bDone = true;
    FPlatformMisc::RequestExitWithStatus(false, 0);
}
