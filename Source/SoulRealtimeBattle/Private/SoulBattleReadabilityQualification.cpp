#include "SoulRealtimeBattleArena.h"
#include "Camera/CameraActor.h"
#include "GameFramework/Character.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

// Explicit opt-in rendered qualification. Calls the normal action handlers;
// native input delivery is checked separately by the control receipt workflow.
void ASoulRealtimeArenaGameMode::TickReadabilityProof()
{
    const double Now = FPlatformTime::Seconds();
    if (ReadabilityStarted == 0.0) ReadabilityStarted = Now;
    const double Elapsed = Now - ReadabilityStarted;
    if (!ReadabilityCaptureName.IsEmpty() && Now >= ReadabilityCaptureAt)
    {
        const FString Directory = FPaths::ProjectSavedDir() / TEXT("Screenshots/Readability");
        IFileManager::Get().MakeDirectory(*Directory, true);
        FScreenshotRequest::RequestScreenshot(Directory / (ReadabilityCaptureName + TEXT(".png")), false, false);
        UE_LOG(LogTemp, Display, TEXT("SOUL_READABILITY_CAPTURE: %s battle=%.2f casts=%d"), *ReadabilityCaptureName, BattleElapsed, MagicCasts);
        ReadabilityCaptureName.Reset();
    }
    auto Capture = [this, Now](const FString& Label)
    {
        ReadabilityCaptureName = Label;
        ReadabilityCaptureAt = FPlatformTime::Seconds() + 0.65; // Allow the camera and particles to render after the action.
    };
    if (Elapsed > 210.0)
    {
        UE_LOG(LogTemp, Error, TEXT("SOUL_READABILITY_FAIL: battle/control qualification timed out"));
        FPlatformMisc::RequestExitWithStatus(false, 1);
        return;
    }
    if (bFinished)
    {
        if (ReadabilityStage != 99)
        {
            Capture(TEXT("result"));
            ReadabilityNextCapture = Now + 3.0;
            ReadabilityStage = 99;
        }
        else if (Now >= ReadabilityNextCapture)
        {
            const bool Passed = MagicCasts >= 5 && AcceptedContactCount() > 0 && TotalAlliedTargets() == 0;
            UE_LOG(LogTemp, Display, TEXT("SOUL_READABILITY_%s: casts=%d contacts=%d alliedTargets=%d seconds=%.2f"),
                Passed ? TEXT("PASS") : TEXT("FAIL"), MagicCasts, AcceptedContactCount(), TotalAlliedTargets(), BattleElapsed);
            FPlatformMisc::RequestExitWithStatus(false, Passed ? 0 : 1);
        }
        return;
    }
    auto* PC = GetWorld()->GetFirstPlayerController();
    if (!PC || !PlayerHero) return;
    static const double Times[] = {4, 8, 12, 16, 20, 21, 23, 25, 27, 29, 33, 43, 50, 55};
    if (ReadabilityStage >= UE_ARRAY_COUNT(Times))
    {
        if (Now >= ReadabilityNextCapture)
        {
            Capture(FString::Printf(TEXT("combat-%03d"), FMath::FloorToInt(BattleElapsed)));
            ReadabilityNextCapture = Now + 5.0;
        }
        return;
    }
    if(ReadabilityStage==3 && Elapsed>=14.0 && ReadabilityNextCapture==0.0)
    {
        SelectAlliedFormationSlot(0);
        HandleBattleAction(TEXT("Focus"));
        TacticalDistance=1100;TacticalRotation=FRotator(-20,120,0);
        Capture(TEXT("frontline-equipment"));
        ReadabilityNextCapture=1.0;
    }
    if (Elapsed < Times[ReadabilityStage]) return;
    auto Cast = [this](const TCHAR* Name)
    {
        const FString Spell = FString::Printf(TEXT("/Game/Soul/Magic/Spells/DA_Soul_%s.DA_Soul_%s"), Name, Name);
        const FString Presentation = FString::Printf(TEXT("/Game/Soul/Magic/Presentation/DA_SoulPresentation_%s.DA_SoulPresentation_%s"), Name, Name);
        const bool Accepted = CastPlayerSpell(*Spell, *Presentation);
        UE_LOG(LogTemp, Display, TEXT("SOUL_READABILITY_SPELL: name=%s accepted=%d"), Name, Accepted);
    };
    // The armies now have a longer approach. Wait for a legal target rather
    // than treating an out-of-range initial click as a spell implementation failure.
    if(ReadabilityStage==5 && FindPlayerSpellTarget(2400.f)==INDEX_NONE) return;
    switch (ReadabilityStage++)
    {
    case 0: if (bTacticalCameraActive) ToggleBattleCamera(); Capture(TEXT("deployment-hero")); break;
    case 1:
        PC->SetControlRotation(FRotator(-4, 0, 0));
        ToggleFirstPersonCamera();
        Capture(TEXT("deployment-first-person"));
        break;
    case 2:
        ToggleBattleCamera();
        Capture(TEXT("deployment-commander"));
        break;
    case 3:
        SetupBattleCamera();
        SelectAlliedFormationSlot(1);
        CommandSelectedAllies(ERBHostGroupOrder::Hold);
        Capture(TEXT("missile-selected-hold"));
        break;
    case 4:
        if (FParse::Param(FCommandLine::Get(), TEXT("SoulBattleProfile")))
            PC->ConsoleCommand(TEXT("csvprofile FRAMES=1200"), true);
        if (bBattlePaused) ToggleBattlePause();
        break;
    case 5: Cast(TEXT("Firebolt")); Capture(TEXT("firebolt-flight")); break;
    case 6:
        Capture(TEXT("firebolt-after"));
        break;
    case 7:
        Cast(TEXT("Blizzard"));
        Capture(TEXT("blizzard"));
        break;
    case 8:
        Cast(TEXT("TidalWard"));
        Capture(TEXT("tidal-ward"));
        break;
    case 9:
        Cast(TEXT("Tailwind"));
        Capture(TEXT("tailwind"));
        break;
    case 10: Capture(TEXT("skirmish")); break;
    case 11: Cast(TEXT("ChainLightning")); Capture(TEXT("chain-lightning-engagement")); break;
    case 12:
        if (!bBattlePaused) ToggleBattlePause();
        Capture(TEXT("engagement-paused"));
        break;
    case 13:
        ReturnSelectedAlliesToAI();
        if (bBattlePaused) ToggleBattlePause();
        ReadabilityNextCapture = Now + 5.0;
        break;
    default: break;
    }
}
