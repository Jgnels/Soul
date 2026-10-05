#pragma once
#include "Commandlets/Commandlet.h"
#include "SoulTitanPilotBuildCommandlet.generated.h"

UCLASS()
class USoulTitanPilotBuildCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    USoulTitanPilotBuildCommandlet();
    virtual int32 Main(const FString& Params) override;
};
