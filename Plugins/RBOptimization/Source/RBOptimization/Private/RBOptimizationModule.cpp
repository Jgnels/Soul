#include "Modules/ModuleManager.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "RBOptimizationAudit.h"
class FRBOptimizationModule final : public IModuleInterface
{
    TUniquePtr<FAutoConsoleCommandWithWorldAndArgs> Audit;
public:
    virtual void StartupModule() override {
        Audit=MakeUnique<FAutoConsoleCommandWithWorldAndArgs>(TEXT("rbopt.Audit"),
            TEXT("Read-only loaded-world audit: rbopt.Audit unique-name.json"),
            FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args,UWorld* World) {
                FString Output;
                const bool Ok=Args.Num()==1 && URBOptimizationAudit::WriteLoadedWorldAudit(World,Args[0],Output);
                UE_LOG(LogTemp,Display,TEXT("RBOptimization audit %s: %s"),Ok?TEXT("written"):TEXT("refused"),*Output);
            }));
    }
    virtual void ShutdownModule() override { Audit.Reset(); }
};
IMPLEMENT_MODULE(FRBOptimizationModule,RBOptimization)
