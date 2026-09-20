#include "RBOptimizationAudit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/LightComponent.h"
#include "Materials/MaterialInterface.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/EngineVersion.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"

bool URBOptimizationAudit::WriteLoadedWorldAudit(const UObject* Context,const FString& FileName,FString& OutputPath)
{
    OutputPath.Reset();
    if(!GEngine || !IsInGameThread() || FileName.IsEmpty() || FileName.Len()>100
        || FPaths::GetCleanFilename(FileName)!=FileName || !FileName.EndsWith(TEXT(".json"))) return false;
    for(const TCHAR C:FileName) if(!(FChar::IsAlnum(C) || C==TEXT('_') || C==TEXT('-') || C==TEXT('.'))) return false;
    UWorld* World=GEngine->GetWorldFromContextObject(Context,EGetWorldErrorMode::ReturnNull);
    if(!World) return false;
    int32 Actors=0,ActorTicks=0,ComponentTicks=0,Physics=0,ISMs=0,Instances=0,Static=0,Skeletal=0,MovableLights=0,Niagara=0;
    TMap<FString,int32> Buckets;
    TArray<TSharedPtr<FJsonValue>> TickExamples;
    for(TActorIterator<AActor> It(World);It;++It) {
        const AActor* A=*It; ++Actors;
        if(A->IsActorTickEnabled()) { ++ActorTicks; if(TickExamples.Num()<50) TickExamples.Add(MakeShared<FJsonValueString>(A->GetPathName())); }
        TInlineComponentArray<UActorComponent*> Components; A->GetComponents(Components);
        for(const UActorComponent* C:Components) {
            if(C->IsComponentTickEnabled()) ++ComponentTicks;
            if(C->GetClass()->GetName()==TEXT("NiagaraComponent")) ++Niagara;
            if(const auto* P=Cast<UPrimitiveComponent>(C)) if(P->IsSimulatingPhysics()) ++Physics;
            if(const auto* Instanced=Cast<UInstancedStaticMeshComponent>(C)) { ++ISMs; Instances+=Instanced->GetInstanceCount(); }
            else if(const auto* M=Cast<UStaticMeshComponent>(C)) {
                ++Static;
                if(!M->GetStaticMesh()) continue;
                FString Key=GetPathNameSafe(A->GetLevel())+TEXT("|")+GetPathNameSafe(M->GetStaticMesh())
                    +TEXT("|")+M->GetCollisionProfileName().ToString()+FString::Printf(TEXT("|mobility=%d|shadow=%d"),int(M->Mobility),int(M->CastShadow));
                for(int32 I=0;I<M->GetNumMaterials();++I) Key+=TEXT("|")+GetPathNameSafe(M->GetMaterial(I));
                ++Buckets.FindOrAdd(Key);
            }
            if(Cast<USkeletalMeshComponent>(C)) ++Skeletal;
            if(const auto* L=Cast<ULightComponent>(C)) if(L->Mobility==EComponentMobility::Movable) ++MovableLights;
        }
    }
    auto Root=MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("schema"),TEXT("rbopt.loaded_world_audit.v1"));
    Root->SetStringField(TEXT("scope"),TEXT("loaded world only; NOT a profiler; no assets/config changed"));
    Root->SetStringField(TEXT("engine"),FEngineVersion::Current().ToString());
    Root->SetStringField(TEXT("world"),World->GetPathName());
    Root->SetStringField(TEXT("utc"),FDateTime::UtcNow().ToIso8601());
    Root->SetNumberField(TEXT("actors"),Actors); Root->SetNumberField(TEXT("enabled_actor_ticks"),ActorTicks);
    Root->SetNumberField(TEXT("enabled_component_ticks"),ComponentTicks); Root->SetNumberField(TEXT("simulating_physics_components"),Physics);
    Root->SetNumberField(TEXT("ism_components"),ISMs); Root->SetNumberField(TEXT("ism_instances"),Instances);
    Root->SetNumberField(TEXT("non_instanced_static_mesh_components"),Static); Root->SetNumberField(TEXT("skeletal_components"),Skeletal);
    Root->SetNumberField(TEXT("movable_light_components"),MovableLights); Root->SetNumberField(TEXT("niagara_components_exact_class"),Niagara);
    Root->SetArrayField(TEXT("enabled_actor_tick_examples_not_recommendations"),TickExamples);
    TArray<TPair<FString,int32>> Sorted; for(const auto& Pair:Buckets) if(Pair.Value>=4) Sorted.Emplace(Pair.Key,Pair.Value);
    Sorted.Sort([](const auto& A,const auto& B){return A.Value!=B.Value?A.Value>B.Value:A.Key<B.Key;});
    TArray<TSharedPtr<FJsonValue>> Groups;
    for(const auto& Pair:Sorted) { auto O=MakeShared<FJsonObject>(); O->SetStringField(TEXT("signature"),Pair.Key); O->SetNumberField(TEXT("components"),Pair.Value); Groups.Add(MakeShared<FJsonValueObject>(O)); }
    Root->SetArrayField(TEXT("repeated_mesh_candidates_semantics_not_validated"),Groups);
    auto Settings=MakeShared<FJsonObject>();
    for(const TCHAR* Name:{TEXT("r.ScreenPercentage"),TEXT("r.DynamicRes.OperationMode"),TEXT("r.VSync"),TEXT("t.MaxFPS"),TEXT("r.AntiAliasingMethod"),TEXT("r.DynamicGlobalIlluminationMethod"),TEXT("r.ReflectionMethod"),TEXT("r.Shadow.Virtual.Enable"),TEXT("r.Nanite"),TEXT("r.PSOPrecaching"),TEXT("r.Streaming.PoolSize")}) {
        const IConsoleVariable* V=IConsoleManager::Get().FindConsoleVariable(Name);
        Settings->SetStringField(Name,V?V->GetString():TEXT("UNAVAILABLE"));
    }
    Root->SetObjectField(TEXT("read_only_console_snapshot"),Settings);
    FString Json; const auto Writer=TJsonWriterFactory<>::Create(&Json);
    if(!FJsonSerializer::Serialize(Root,Writer)) return false;
    const FString Dir=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("RBOptimization"));
    if(!IFileManager::Get().MakeDirectory(*Dir,true)) return false;
    const FString Path=FPaths::Combine(Dir,FileName);
    if(IFileManager::Get().FileExists(*Path)) return false; // Never overwrite earlier evidence.
    if(!FFileHelper::SaveStringToFile(Json,*Path,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) return false;
    OutputPath=FPaths::ConvertRelativePathToFull(Path); return true;
}
