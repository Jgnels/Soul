#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Templates/Function.h"
#include "RBSaveTypes.h"

namespace rb::save { struct Snapshot; }

using FRBSaveCppCompletion = TFunction<void(FRBSaveOperationResult)>;
#include "RBSaveSubsystem.generated.h"

UCLASS()
class RBSAVE_API URBSaveSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // Integration-grade coordinated API. Prefer these in games.
    UFUNCTION(BlueprintCallable, Category="RB Save")
    void SaveGameAsync(const FString& Slot, const FRBSaveOperationDelegate& Completion);

    UFUNCTION(BlueprintCallable, Category="RB Save")
    void LoadGameAsync(const FString& Slot, const FRBSaveOperationDelegate& Completion);

    UFUNCTION(BlueprintCallable, Category="RB Save")
    void RollbackToGenerationAsync(const FString& Slot, int64 Generation,
                                   const FRBSaveOperationDelegate& Completion);

    UFUNCTION(BlueprintCallable, Category="RB Save|Inspection")
    bool GetActiveGeneration(const FString& Slot, FRBSaveGenerationInfo& OutInfo, FString& Error) const;

    UFUNCTION(BlueprintCallable, Category="RB Save|Inspection")
    bool ValidateActiveGeneration(const FString& Slot, FString& Error) const;
    UFUNCTION(BlueprintPure, Category="RB Save|Backend")
    TArray<FName> GetAvailableWorldBackends() const;

    UFUNCTION(BlueprintPure, Category="RB Save|Backend")
    FName GetEffectiveWorldBackend() const;

    // C++ callback path used by coordinated backends.
    void SaveWorldCpp(const FString& Slot, FRBSaveCppCompletion Completion);
    void LoadWorldCpp(const FString& Slot, FRBSaveCppCompletion Completion);

    // Direct native world API retained for compatibility/fallback.
    UFUNCTION(BlueprintCallable, Category="RB Save|World")
    void SaveWorldAsync(const FString& Slot, const FRBSaveOperationDelegate& Completion);

    UFUNCTION(BlueprintCallable, Category="RB Save|World")
    void LoadWorldAsync(const FString& Slot, const FRBSaveOperationDelegate& Completion);

    UFUNCTION(BlueprintCallable, Category="RB Save|Domain")
    bool RegisterDomainProvider(UObject* Provider, FString& Error);

    UFUNCTION(BlueprintCallable, Category="RB Save|Domain")
    void UnregisterDomainProvider(UObject* Provider);

    UFUNCTION(BlueprintCallable, Category="RB Save|Domain")
    void SaveDomainsAsync(const FString& Slot, const FRBSaveOperationDelegate& Completion);

    UFUNCTION(BlueprintCallable, Category="RB Save|Domain")
    void LoadDomainsAsync(const FString& Slot, const FRBSaveOperationDelegate& Completion);

    // Explicit checkpoint scope. Unselected domains are left untouched; every selected
    // provider must exist and succeed. Uses the same atomic domain backend.
    UFUNCTION(BlueprintCallable, Category="RB Save|Domain")
    void SaveSelectedDomainsAsync(const FString& Slot, const TArray<FName>& DomainIds,
                                  const FRBSaveOperationDelegate& Completion);
    UFUNCTION(BlueprintCallable, Category="RB Save|Domain")
    void LoadSelectedDomainsAsync(const FString& Slot, const TArray<FName>& DomainIds,
                                  const FRBSaveOperationDelegate& Completion);


    UFUNCTION(BlueprintPure, Category="RB Save|Domain")
    TArray<FName> GetRegisteredDomainIds() const;

    UFUNCTION(BlueprintCallable, Category="RB Save|Inspection")
    FString InspectSlot(const FString& Slot, bool& bValid, FString& Error) const;
    UFUNCTION(BlueprintCallable, Category="RB Save|Inspection")
    bool RestoreQuickBackup(const FString& Slot, FString& Error) const;

    UFUNCTION(BlueprintPure, Category="RB Save|Paths")
    FString GetSlotPath(const FString& Slot) const;

    UFUNCTION(BlueprintPure, Category="RB Save|Paths")
    FString GetDomainSlotPath(const FString& Slot) const;

    UFUNCTION(BlueprintPure, Category="RB Save|Paths")
    FString GetGenerationRoot(const FString& Slot) const;

    bool CaptureRegisteredDomains(const FString& Slot, rb::save::Snapshot& OutSnapshot,
                                  FString& OutError, const TArray<FName>* SelectedDomains = nullptr) const;
    bool RestoreRegisteredDomains(const rb::save::Snapshot& Snapshot, FString& OutError,
                                  const TArray<FName>* SelectedDomains = nullptr);

private:
    UPROPERTY(Transient)
    TArray<TWeakObjectPtr<UObject>> DomainProviders;
};
