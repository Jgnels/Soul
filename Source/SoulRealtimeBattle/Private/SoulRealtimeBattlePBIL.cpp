#include "SoulRealtimeBattlePBIL.h"

#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "RBPBILInfluenceComponent.h"
#include "RBPBILInfluenceVolume.h"
#include "RBPBILLibrary.h"

bool USoulRealtimeBattlePBIL::Configure(UWorld* World, const FVector& Origin, const FVector& HalfExtents)
{
    Shutdown();
    QueryCount = 0;
    SuccessfulQueryCount = 0;
    if (!World || Origin.ContainsNaN())
    {
        return false;
    }
    BattleWorld = World;
    const FTransform Transform(FRotator::ZeroRotator, Origin);
    Field = World->SpawnActorDeferred<ARBPBILInfluenceVolume>(
        ARBPBILInfluenceVolume::StaticClass(), Transform, nullptr, nullptr,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Field || !Field->SetRefreshPolicy(ERBPBILRefreshPolicy::CPU) ||
        !Field->ConfigureRuntimeBounds(HalfExtents, 150.0f))
    {
        Shutdown();
        return false;
    }
    // The existing TCAT CPU path avoids its previously observed UE 5.8 GPU-readback assertion.
    Field->FinishSpawning(Transform);
    UE_LOG(LogTemp, Log, TEXT("SOUL_PBIL_READY refresh=CPU grid=%dx%d origin=%s"),
        Field->GetColumns(), Field->GetRows(), *Origin.ToCompactString());
    return Field->GetColumns() > 1 && Field->GetRows() > 1;
}

bool USoulRealtimeBattlePBIL::RegisterUnit(AActor* Unit, bool bEnemy)
{
    if (!IsValid(Unit) || !IsValid(Field) || Unit->GetWorld() != BattleWorld.Get())
    {
        return false;
    }
    for (const FUnitSource& Source : Sources)
    {
        if (Source.Unit.Get() == Unit)
        {
            return Source.bEnemy == bEnemy && Source.Influence.IsValid();
        }
    }
    URBPBILInfluenceComponent* Influence = NewObject<URBPBILInfluenceComponent>(Unit);
    Unit->AddInstanceComponent(Influence);
    Influence->SetupAttachment(Unit->GetRootComponent());
    // Congestion represents occupancy from both sides, not enemy threat or faction identity.
    Influence->SetChannelInfluence(ERBPBILChannel::Congestion, 400.0f, 1.0f);
    Influence->RegisterComponent();
    FUnitSource& Source = Sources.AddDefaulted_GetRef();
    Source.Unit = Unit;
    Source.Influence = Influence;
    Source.bEnemy = bEnemy;
    return true;
}

void USoulRealtimeBattlePBIL::UnregisterUnit(AActor* Unit)
{
    Sources.RemoveAll([Unit](FUnitSource& Source)
    {
        if (!Source.Unit.IsValid() || Source.Unit.Get() == Unit)
        {
            if (URBPBILInfluenceComponent* Influence = Source.Influence.Get())
            {
                Influence->DestroyComponent();
            }
            return true;
        }
        return false;
    });
}

bool USoulRealtimeBattlePBIL::QueryApproach(AActor* Unit, bool bEnemy,
    const FVector& Target, FVector& OutLocation)
{
    OutLocation = Target;
    UWorld* World = BattleWorld.Get();
    if (!World || !IsValid(Field) || !IsValid(Unit) || Target.ContainsNaN())
    {
        return false;
    }
    const FUnitSource* Source = Sources.FindByPredicate([Unit, bEnemy](const FUnitSource& Entry)
    {
        return Entry.Unit.Get() == Unit && Entry.bEnemy == bEnemy && Entry.Influence.IsValid();
    });
    if (!Source)
    {
        return false;
    }
    const FVector From = Unit->GetActorLocation();
    const FVector Direction = (Target - From).GetSafeNormal2D();
    const float Distance = FVector::Dist2D(From, Target);
    // Leave close combat to RB Combat; approach orders must not pull engaged units out of melee.
    if (Distance < 650.0f)
    {
        return false;
    }
    FRBPBILQueryRequest Request;
    Request.Channel = ERBPBILChannel::Congestion;
    Request.Origin = From + Direction * FMath::Min(600.0f, Distance * 0.5f);
    Request.SearchRadius = 300.0f;
    Request.bFindHighest = false;
    Request.bReachableOnly = true;
    Request.bIgnoreHeight = true;
    FRBPBILQueryResult Result;
    ++QueryCount;
    if (!URBPBILLibrary::QueryBestLocationImmediate(World, Request, Result) ||
        !Result.bSuccess || Result.WorldLocation.ContainsNaN() ||
        FVector::DistSquared2D(Result.WorldLocation, Target) >= FMath::Square(Distance) ||
        FVector::DotProduct(Result.WorldLocation - From, Direction) < 100.0f)
    {
        return false;
    }
    // TCAT's reachability query permits worlds without NavData. A physical obstruction must
    // still reject a direct approach so the adapter does not order units through graveyard bones.
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_WorldStatic);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(SoulPBILApproach), false, Unit);
    Params.AddIgnoredActor(Field);
    FHitResult Hit;
    const FVector Lift(0.0f, 0.0f, 100.0f);
    if (World->SweepSingleByObjectType(Hit, From + Lift, Result.WorldLocation + Lift,
        FQuat::Identity, Objects, FCollisionShape::MakeSphere(35.0f), Params))
    {
        return false;
    }
    OutLocation = Result.WorldLocation;
    ++SuccessfulQueryCount;
    return true;
}

int32 USoulRealtimeBattlePBIL::GetRegisteredUnitCount() const
{
    int32 Count = 0;
    for (const FUnitSource& Source : Sources)
    {
        Count += Source.Unit.IsValid() && Source.Influence.IsValid() ? 1 : 0;
    }
    return Count;
}

void USoulRealtimeBattlePBIL::Shutdown()
{
    for (FUnitSource& Source : Sources)
    {
        if (URBPBILInfluenceComponent* Influence = Source.Influence.Get())
        {
            Influence->DestroyComponent();
        }
    }
    Sources.Reset();
    if (IsValid(Field))
    {
        Field->Destroy();
    }
    Field = nullptr;
    BattleWorld.Reset();
}
