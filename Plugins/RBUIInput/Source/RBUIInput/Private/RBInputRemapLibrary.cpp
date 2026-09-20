#include "RBInputRemapLibrary.h"

#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "UserSettings/EnhancedInputUserSettings.h"

namespace
{
UEnhancedInputUserSettings* GetSettings(ULocalPlayer* LocalPlayer)
{
    if (!LocalPlayer)
    {
        return nullptr;
    }
    if (UEnhancedInputLocalPlayerSubsystem* Enhanced = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
    {
        return Enhanced->GetUserSettings();
    }
    return nullptr;
}

EPlayerMappableKeySlot ToEngineSlot(const ERBInputBindingSlot Slot)
{
    switch (Slot)
    {
        case ERBInputBindingSlot::Secondary: return EPlayerMappableKeySlot::Second;
        case ERBInputBindingSlot::Tertiary: return EPlayerMappableKeySlot::Third;
        case ERBInputBindingSlot::Quaternary: return EPlayerMappableKeySlot::Fourth;
        default: return EPlayerMappableKeySlot::First;
    }
}

void FillPreview(UEnhancedInputUserSettings* Settings, const FName MappingName, const FKey NewKey, FRBRemapResult& Result)
{
    Result.MappingName = MappingName;
    Result.NewKey = NewKey;

    if (!Settings || !Settings->GetActiveKeyProfile())
    {
        Result.Status = ERBRemapStatus::UserSettingsUnavailable;
        Result.Error = TEXT("Enhanced Input user settings are unavailable. Enable Enhanced Input User Settings for this project/local player.");
        return;
    }
    if (MappingName.IsNone() || !NewKey.IsValid())
    {
        Result.Status = ERBRemapStatus::InvalidRequest;
        Result.Error = TEXT("A valid mapping name and key are required.");
        return;
    }

    TArray<FName> MappingNames;
    Settings->GetActiveKeyProfile()->GetMappingNamesForKey(NewKey, MappingNames);
    for (const FName Name : MappingNames)
    {
        if (Name != MappingName)
        {
            Result.ConflictingMappings.AddUnique(Name);
        }
    }

    Result.Status = Result.ConflictingMappings.IsEmpty() ? ERBRemapStatus::Success : ERBRemapStatus::Conflict;
}
}

bool URBInputRemapLibrary::IsRemappingAvailable(ULocalPlayer* LocalPlayer)
{
    UEnhancedInputUserSettings* Settings = GetSettings(LocalPlayer);
    return Settings && Settings->GetActiveKeyProfile();
}

FRBRemapResult URBInputRemapLibrary::PreviewRemap(ULocalPlayer* LocalPlayer, const FName MappingName, const FKey NewKey)
{
    FRBRemapResult Result;
    FillPreview(GetSettings(LocalPlayer), MappingName, NewKey, Result);
    return Result;
}

bool URBInputRemapLibrary::ApplyRemap(ULocalPlayer* LocalPlayer, const FName MappingName, const FKey NewKey, const ERBInputBindingSlot Slot, const bool bRejectConflicts, const bool bPersist, FRBRemapResult& OutResult)
{
    UEnhancedInputUserSettings* Settings = GetSettings(LocalPlayer);
    FillPreview(Settings, MappingName, NewKey, OutResult);
    if (!Settings || OutResult.Status == ERBRemapStatus::UserSettingsUnavailable || OutResult.Status == ERBRemapStatus::InvalidRequest)
    {
        return false;
    }
    if (bRejectConflicts && OutResult.Status == ERBRemapStatus::Conflict)
    {
        OutResult.Error = TEXT("The requested key is already used by another player-mappable action.");
        return false;
    }

    FMapPlayerKeyArgs Args;
    Args.MappingName = MappingName;
    Args.Slot = ToEngineSlot(Slot);
    Args.NewKey = NewKey;
    Args.bCreateMatchingSlotIfNeeded = true;

    FGameplayTagContainer FailureReason;
    Settings->MapPlayerKey(Args, FailureReason);
    if (!FailureReason.IsEmpty())
    {
        OutResult.Status = ERBRemapStatus::EngineRejected;
        OutResult.Error = FailureReason.ToString();
        return false;
    }

    Settings->ApplySettings();
    if (bPersist)
    {
        Settings->SaveSettings();
    }
    OutResult.Status = ERBRemapStatus::Success;
    OutResult.Error.Reset();
    return true;
}

bool URBInputRemapLibrary::ResetMappingToDefault(ULocalPlayer* LocalPlayer, const FName MappingName, const bool bPersist, FString& OutError)
{
    UEnhancedInputUserSettings* Settings = GetSettings(LocalPlayer);
    if (!Settings || MappingName.IsNone())
    {
        OutError = TEXT("Enhanced Input user settings and a valid mapping name are required.");
        return false;
    }

    FMapPlayerKeyArgs Args;
    Args.MappingName = MappingName;
    FGameplayTagContainer FailureReason;
    Settings->ResetAllPlayerKeysInRow(Args, FailureReason);
    if (!FailureReason.IsEmpty())
    {
        OutError = FailureReason.ToString();
        return false;
    }

    Settings->ApplySettings();
    if (bPersist)
    {
        Settings->SaveSettings();
    }
    OutError.Reset();
    return true;
}

bool URBInputRemapLibrary::ResetAllMappingsToDefault(ULocalPlayer* LocalPlayer, const bool bPersist, FString& OutError)
{
    UEnhancedInputUserSettings* Settings = GetSettings(LocalPlayer);
    if (!Settings || !Settings->GetActiveKeyProfile())
    {
        OutError = TEXT("Enhanced Input user settings are unavailable.");
        return false;
    }

    Settings->GetActiveKeyProfile()->ResetToDefault();
    Settings->ApplySettings();
    if (bPersist)
    {
        Settings->SaveSettings();
    }
    OutError.Reset();
    return true;
}

TArray<FKey> URBInputRemapLibrary::GetCurrentKeysForMapping(ULocalPlayer* LocalPlayer, const FName MappingName)
{
    TArray<FKey> Result;
    UEnhancedInputUserSettings* Settings = GetSettings(LocalPlayer);
    if (!Settings || !Settings->GetActiveKeyProfile() || MappingName.IsNone())
    {
        return Result;
    }

    struct FSlotKey
    {
        uint8 Slot = 0;
        FKey Key;
    };
    TArray<FSlotKey> Ordered;
    for (const FPlayerKeyMapping& Mapping : Settings->FindMappingsInRow(MappingName))
    {
        if (Mapping.GetCurrentKey().IsValid())
        {
            Ordered.Add({static_cast<uint8>(Mapping.GetSlot()), Mapping.GetCurrentKey()});
        }
    }
    Ordered.Sort([](const FSlotKey& A, const FSlotKey& B) { return A.Slot < B.Slot; });
    for (const FSlotKey& Entry : Ordered)
    {
        Result.Add(Entry.Key);
    }
    return Result;
}

