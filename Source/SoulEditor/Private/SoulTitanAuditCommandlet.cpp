#include "SoulTitanAuditCommandlet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Serialization/JsonSerializer.h"

namespace
{
const TCHAR* Seeds[] = {
    TEXT("/Game/Environment/Clifftop/Level_Instance/IL_Clifftop_Mine_TownGrid"),
    TEXT("/Game/Environment/Sulfur/Level_Instances/LI_Sulfur_BanditRestOutpost")
};

TArray<TSharedPtr<FJsonValue>> Strings(TArray<FString> Values)
{
    Values.Sort();
    TArray<TSharedPtr<FJsonValue>> Result;
    for (const FString& Value : Values) Result.Add(MakeShared<FJsonValueString>(Value));
    return Result;
}

bool Forbidden(const FString& Package)
{
    // Exact passive material-support exception. The migration planner still
    // verifies these packages' native classes before it can produce a plan.
    if (Package == TEXT("/Game/Blueprint/FoliageInteraction/MPC_Player")
        || Package == TEXT("/Game/Blueprint/FoliageInteraction/RT_Player"))
        return false;
    for (const TCHAR* Prefix : { TEXT("/Game/Characters/"), TEXT("/Game/Blueprint/"),
        TEXT("/Game/BlueprintDEV/"), TEXT("/Game/Maps/"), TEXT("/Script/Titan"),
        TEXT("/Game/__ExternalActors__/Maps/"), TEXT("/Game/__ExternalObjects__/Maps/") })
        if (Package.StartsWith(Prefix)) return true;
    return false;
}
}

USoulTitanAuditCommandlet::USoulTitanAuditCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 USoulTitanAuditCommandlet::Main(const FString& Params)
{
    FString Donor, Output;
    if (!FParse::Value(*Params, TEXT("Donor="), Donor) || !FParse::Value(*Params, TEXT("Output="), Output))
    {
        UE_LOG(LogTemp, Error, TEXT("Require -Donor=<read-only project root> -Output=<Soul evidence JSON>"));
        return 1;
    }
    Donor = FPaths::ConvertRelativePathToFull(Donor / TEXT("Content")) + TEXT("/");
    Output = FPaths::ConvertRelativePathToFull(Output);
    const FString Workspace = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
    if (!FPaths::IsUnderDirectory(Output, Workspace) || FPaths::IsUnderDirectory(Output, Donor)
        || !IFileManager::Get().DirectoryExists(*Donor)) return 2;

    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    // The commandlet's process-local mount preserves /Game package names. No
    // Titan project startup, packages, Blueprint logic or configuration is loaded.
    FPackageName::RegisterMountPoint(TEXT("/Game/"), Donor);
    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("method"), TEXT("UE AssetRegistry per-file hard+soft closure with recursive map companions"));
    Report->SetStringField(TEXT("donor_content"), Donor);
    Report->SetBoolField(TEXT("migration_permitted"), false);
    TArray<TSharedPtr<FJsonValue>> Donors;
    for (const TCHAR* Seed : Seeds)
    {
        TArray<FString> Queue { FString(Seed) };
        TSet<FString> Seen;
        TArray<TSharedPtr<FJsonValue>> Packages;
        TArray<FString> Missing, Blocked, External;
        int64 TotalBytes = 0;
        for (int32 Index = 0; Index < Queue.Num(); ++Index)
        {
            const FString Package = Queue[Index];
            if (Seen.Contains(Package)) continue;
            Seen.Add(Package);
            if (Seen.Num() > 10000) return 3;
            if (Forbidden(Package)) { Blocked.Add(Package); continue; }
            if (!Package.StartsWith(TEXT("/Game/"))) { External.Add(Package); continue; }
            FString Filename = Donor / Package.RightChop(6);
            if (IFileManager::Get().FileExists(*(Filename + TEXT(".umap")))) Filename += TEXT(".umap");
            else Filename += TEXT(".uasset");
            if (!IFileManager::Get().FileExists(*Filename)) { Missing.Add(Package); continue; }
            Registry.ScanSynchronous({}, { Filename }, UE::AssetRegistry::EScanFlags::ForceRescan
                | UE::AssetRegistry::EScanFlags::IgnoreDenyListScanFilters);
            TArray<FName> Hard, All;
            FAssetRegistryDependencyOptions HardOptions;
            HardOptions.bIncludeSoftPackageReferences = false;
            HardOptions.bIncludeHardPackageReferences = true;
            HardOptions.bIncludeSearchableNames = false;
            HardOptions.bIncludeSoftManagementReferences = false;
            HardOptions.bIncludeHardManagementReferences = false;
            FAssetRegistryDependencyOptions AllOptions = HardOptions;
            AllOptions.bIncludeSoftPackageReferences = true;
            const bool bHardQuery = Registry.K2_GetDependencies(FName(*Package), HardOptions, Hard);
            const bool bAllQuery = Registry.K2_GetDependencies(FName(*Package), AllOptions, All);
            TArray<FAssetData> Assets;
            Registry.GetAssetsByPackageName(FName(*Package), Assets, true);
            TArray<FString> Classes, ClassPackages, HardStrings, SoftStrings, Companions;
            for (const FAssetData& Asset : Assets)
            {
                const FString ClassPath = Asset.AssetClassPath.ToString();
                Classes.AddUnique(ClassPath);
                // External-actor package dependency queries can legitimately return false
                // while AssetData still exposes the actor Blueprint class. Treat that class
                // package as a real dependency and recurse into it rather than silently
                // accepting an incomplete World Partition closure.
                if (ClassPath.StartsWith(TEXT("/Game/")))
                {
                    int32 DotIndex = INDEX_NONE;
                    if (ClassPath.FindChar(TEXT('.'), DotIndex) && DotIndex > 0)
                    {
                        ClassPackages.AddUnique(ClassPath.Left(DotIndex));
                        Queue.Add(ClassPath.Left(DotIndex));
                    }
                }
            }
            for (FName Dep : Hard) HardStrings.Add(Dep.ToString());
            for (FName Dep : All)
            {
                if (!Hard.Contains(Dep)) SoftStrings.Add(Dep.ToString());
                Queue.Add(Dep.ToString());
            }
            if (Filename.EndsWith(TEXT(".umap")))
            {
                for (const TCHAR* Tree : { TEXT("__ExternalActors__"), TEXT("__ExternalObjects__") })
                {
                    TArray<FString> Files;
                    IFileManager::Get().FindFilesRecursive(Files, *(Donor / Tree / Package.RightChop(6)), TEXT("*.uasset"), true, false);
                    for (FString File : Files)
                    {
                        FPaths::MakePathRelativeTo(File, *Donor);
                        Companions.Add(TEXT("/Game/") + FPaths::ChangeExtension(File, TEXT("")));
                    }
                }
                Queue.Append(Companions);
            }
            auto Record = MakeShared<FJsonObject>();
            Record->SetStringField(TEXT("package"), Package);
            Record->SetStringField(TEXT("file"), Filename);
            Record->SetNumberField(TEXT("bytes"), IFileManager::Get().FileSize(*Filename));
            Record->SetBoolField(TEXT("hard_query_found"), bHardQuery);
            Record->SetBoolField(TEXT("all_query_found"), bAllQuery);
            Record->SetArrayField(TEXT("classes"), Strings(Classes));
            Record->SetArrayField(TEXT("class_dependencies"), Strings(ClassPackages));
            Record->SetArrayField(TEXT("hard"), Strings(HardStrings));
            Record->SetArrayField(TEXT("soft"), Strings(SoftStrings));
            Record->SetArrayField(TEXT("companions"), Strings(Companions));
            Packages.Add(MakeShared<FJsonValueObject>(Record));
            TotalBytes += IFileManager::Get().FileSize(*Filename);
        }
        auto Result = MakeShared<FJsonObject>();
        Result->SetStringField(TEXT("seed"), Seed);
        Result->SetNumberField(TEXT("bytes"), TotalBytes);
        Result->SetArrayField(TEXT("packages"), Packages);
        Result->SetArrayField(TEXT("blocked"), Strings(Blocked));
        Result->SetArrayField(TEXT("missing"), Strings(Missing));
        Result->SetArrayField(TEXT("external"), Strings(External));
        Donors.Add(MakeShared<FJsonValueObject>(Result));
        UE_LOG(LogTemp, Display, TEXT("SOUL_TITAN_AUDIT seed=%s packages=%d blocked=%d missing=%d bytes=%lld"), Seed, Packages.Num(), Blocked.Num(), Missing.Num(), TotalBytes);
    }
    Report->SetArrayField(TEXT("donors"), Donors);
    FString Json;
    FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Json));
    FPackageName::UnRegisterMountPoint(TEXT("/Game/"), Donor);
    return FFileHelper::SaveStringToFile(Json, *Output) ? 0 : 4;
}
