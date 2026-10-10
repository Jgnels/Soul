#pragma once
#include "CoreMinimal.h"
class UTexture2D;
// Small, licensed loose UI resources. Fixed process-lifetime cache; no gameplay authority.
namespace SoulHUDArt { SOULCORE_API UTexture2D* Texture(const TCHAR* Name); SOULCORE_API bool Enabled(); }
