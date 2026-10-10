#pragma once
#include "CoreMinimal.h"
// Frozen, measured Human bridge geometry in arena-local centimetres. Presentation
// traversal constraints only; canonical encounter, damage and outcome stay unchanged.
namespace SoulHeartlandBridge
{
 SOULREALTIMEBATTLE_API const TArray<FVector>& River();
 SOULREALTIMEBATTLE_API FVector ToDeck(const FVector& P);
 SOULREALTIMEBATTLE_API FVector FromDeck(const FVector& P);
 SOULREALTIMEBATTLE_API bool IsWater(const FVector& P);
 SOULREALTIMEBATTLE_API bool CrossesWater(const FVector& A,const FVector& B);
 SOULREALTIMEBATTLE_API bool Approach(const FVector& From,const FVector& Target,FVector& Out);
}
