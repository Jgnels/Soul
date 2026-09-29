#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoulCampaignWorldActor.generated.h"

// Presentation only. Region IDs, legal movement and knowledge belong to SoulWorld.
UCLASS()
class SOUL_API ASoulCampaignWorldActor : public AActor
{
    GENERATED_BODY()
public:
    ASoulCampaignWorldActor();
    virtual void Tick(float DeltaSeconds) override;
    void Build(class USoulFounderPlaytestStateSubsystem* InState);
    void RefreshKnowledge();
    void PresentPlayerLocation(FName RegionId, bool bAnimate);
    static const TMap<FName, FVector>& Locations();
    static float HeightAt(float X, float Y);
    static float RiverX(float Y);
    static constexpr float HalfWidth = 8000.f;
    static constexpr float HalfDepth = 8000.f;
    static FVector RoadPoint(FName From, FName To, float Alpha);
    static class UMaterialInstanceDynamic* MakeMaterial(UObject* Outer, const FLinearColor& Color);
    static class UInstancedStaticMeshComponent* MakeInstances(AActor* Owner, const TCHAR* Shape, const FLinearColor& Color);
private:
    UPROPERTY() TObjectPtr<class USoulFounderPlaytestStateSubsystem> State;
    UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Terrain;
    UPROPERTY() TObjectPtr<class UProceduralMeshComponent> River;
    UPROPERTY() TObjectPtr<class USceneComponent> Party;
    UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> Soldiers;
    UPROPERTY() TArray<TObjectPtr<class UProceduralMeshComponent>> Roads;
    TArray<TPair<FName,FName>> RoadRegions;
    TArray<FVector> Vertices, Normals;
    TArray<int32> Triangles;
    TArray<FVector2D> UVs;
    TArray<FLinearColor> BaseColors;
    FString KnowledgeSignature;
    FName PresentedRegion, TravelFrom, TravelTo;
    float TravelAlpha = 1.f;
    void BuildTerrain();
    void BuildRoads();
    void BuildDressing();
    void BuildParty();
};
