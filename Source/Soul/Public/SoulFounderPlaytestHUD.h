#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SoulFounderPlaytestHUD.generated.h"

UCLASS()
class SOUL_API ASoulFounderPlaytestHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
};
