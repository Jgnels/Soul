#pragma once
#include "Commandlets/Commandlet.h"
#include "SoulTitanAuditCommandlet.generated.h"

/** Read-only donor registry audit. Never loads or saves donor UObjects. */
UCLASS()
class USoulTitanAuditCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    USoulTitanAuditCommandlet();
    virtual int32 Main(const FString& Params) override;
};
