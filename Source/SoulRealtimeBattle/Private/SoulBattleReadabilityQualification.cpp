#include "SoulRealtimeBattleArena.h"
#include "Camera/CameraActor.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimationAsset.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "GameFramework/HUDHitBox.h"
#include "Engine/Texture2D.h"
#include "Algo/AllOf.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#endif

// Explicit opt-in rendered qualification. Calls the normal action handlers;
// native input delivery is checked separately by the control receipt workflow.
void ASoulRealtimeArenaGameMode::TickReadabilityProof()
{
#if WITH_EDITOR
    // Editor-game can render default materials while asynchronous compilation
    // finishes. Qualification must capture the loaded scene, not that placeholder.
    if (ReadabilityStarted == 0.0)
    {
        auto& Compiler = FAssetCompilingManager::Get();
        UE_LOG(LogTemp, Display, TEXT("SOUL_CAPTURE_ASSETS: pending=%d"), Compiler.GetNumRemainingAssets());
        Compiler.FinishAllCompilation();
        UE_LOG(LogTemp, Display, TEXT("SOUL_CAPTURE_ASSETS: ready pending=%d"), Compiler.GetNumRemainingAssets());
    }
#endif
    if(FParse::Param(FCommandLine::Get(),TEXT("SoulAnimationPoseProof"))) { TickAnimationPoseProof(); return; }
    if(FParse::Param(FCommandLine::Get(),TEXT("SoulSpellbookProof"))) { TickSpellbookProof(); return; }
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
    float TimeoutSeconds = 210.0f;
    FParse::Value(FCommandLine::Get(), TEXT("SoulReadabilityTimeout="), TimeoutSeconds);
    TimeoutSeconds = FMath::Clamp(TimeoutSeconds, 210.0f, 600.0f);
    if (Elapsed > TimeoutSeconds)
    {
        UE_LOG(LogTemp, Error, TEXT("SOUL_READABILITY_FAIL: timeout wall=%.2f battle=%.2f alive=%d/%d contacts=%d budget=%.0f"),
            Elapsed, BattleElapsed, AliveForSide(0), AliveForSide(1), AcceptedContactCount(), TimeoutSeconds);
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
    // Preserve each scheduled view when loading or a delayed target advances
    // several wall-clock stages at once. Never overwrite an uncaptured spell.
    if (!ReadabilityCaptureName.IsEmpty()) return;
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
    {
        SetupBattleCamera();
        int32 AlliedSlot = 0;
        for (const auto& Formation : TacticalFormations)
        {
            if (Formation.Side != 0) continue;
            if (Formation.Kind == ESoulBattleFormationKind::MissileSupport)
            {
                SelectAlliedFormationSlot(AlliedSlot);
                UE_LOG(LogTemp, Display, TEXT("SOUL_READABILITY_MISSILE: slot=%d group=%d"), AlliedSlot, Formation.GroupIndex);
                break;
            }
            ++AlliedSlot;
        }
        CommandSelectedAllies(ERBHostGroupOrder::Hold);
        Capture(TEXT("missile-selected-hold"));
        break;
    }
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

// Opt-in rendered HUD qualification. Uses the real HUD hitboxes/action route;
// it is not a claim that physical mouse/controller hardware was exercised.
void ASoulRealtimeArenaGameMode::TickSpellbookProof()
{
    const double Now=FPlatformTime::Seconds();
    if(ReadabilityStarted==0) ReadabilityStarted=Now;
    auto* PC=GetWorld()->GetFirstPlayerController();
    auto* HUD=PC?Cast<ASoulRealtimeArenaHUD>(PC->GetHUD()):nullptr;
    if(!PC || !HUD) return;
    int32 PixelsX=0,PixelsY=0;PC->GetViewportSize(PixelsX,PixelsY);
    const float Scale=FMath::Max(.5f,FMath::Min(PixelsX/1280.f,PixelsY/720.f));
    const float W=PixelsX/Scale,H=PixelsY/Scale;
    auto Require=[&](bool OK,const TCHAR* Label)
    {
        UE_LOG(LogTemp,Display,TEXT("SOUL_SPELLBOOK_CHECK: %s %s"),Label,OK?TEXT("PASS"):TEXT("FAIL"));
        if(!OK) FPlatformMisc::RequestExitWithStatus(false,1);
        return OK;
    };
    auto Click=[&](float X,float Y,FName Expected)
    {
        const auto* Box=HUD->GetHitBoxAtCoordinates(FVector2D(X,Y)*Scale,true);
        if(!Require(Box && Box->GetName()==Expected,*Expected.ToString())) return false;
        HUD->NotifyHitBoxClick(Box->GetName());
        return true;
    };
    auto Capture=[&](const TCHAR* Label)
    {
        ReadabilityCaptureName=FString::Printf(TEXT("%dx%d-%s"),PixelsX,PixelsY,Label);
        ReadabilityCaptureAt=Now+.75;
    };
    if(!ReadabilityCaptureName.IsEmpty())
    {
        if(Now<ReadabilityCaptureAt || FScreenshotRequest::IsScreenshotRequested()) return;
        const FString Dir=FPaths::ProjectSavedDir()/TEXT("Screenshots/Spellbook");
        IFileManager::Get().MakeDirectory(*Dir,true);
        FScreenshotRequest::RequestScreenshot(Dir/(ReadabilityCaptureName+TEXT(".png")),false,false);
        UE_LOG(LogTemp,Display,TEXT("SOUL_SPELLBOOK_CAPTURE: %s"),*ReadabilityCaptureName);
        ReadabilityCaptureName.Reset();
        ReadabilityNextCapture=Now+1.5;
        return;
    }
    if(Now<ReadabilityNextCapture || Now-ReadabilityStarted<2.0) return;
    switch(ReadabilityStage++)
    {
    case 0:
        if(!bTacticalCameraActive) ToggleBattleCamera();
        PC->SetMouseLocation(PixelsX/2,PixelsY/2);
        if(!Require(SpellIcons.Num()==6 && Algo::AllOf(SpellIcons,[](const auto& Icon){return IsValid(Icon.Get());}),TEXT("six donor icons loaded"))) return;
        Capture(TEXT("compact-deployment"));
        break;
    case 1:
        if(!Click(W-150,H-38,TEXT("Grimoire"))) return;
        if(!Require(bSpellbookOpen && bBattlePaused,TEXT("book preserves manual pause"))) return;
        PC->SetMouseLocation(FMath::RoundToInt((W-340)*Scale),FMath::RoundToInt((H-470+55+52+20)*Scale));
        Capture(TEXT("open-book"));
        break;
    case 2:
        {
            const auto* Box=HUD->GetHitBoxAtCoordinates(FVector2D(W-100,H-210)*Scale,true);
            if(!Require(Box && Box->GetName()==TEXT("BookBackground"),TEXT("page blocks world clicks"))) return;
        }
        if(!Click(W-340,H-470+55+3*52+20,TEXT("BookSpell3"))) return;
        if(!Require(!bSpellbookOpen && SelectedSpellSlot==3 && bBattlePaused && MagicCasts==0,TEXT("paused ward arms without cast"))) return;
        Capture(TEXT("paused-selection"));
        break;
    case 3:
        HandleGamepadAction(TEXT("Cancel"));bGamepadActive=false;
        if(!Click(65,70,TEXT("PauseRule"))) return;
        if(!Click(W-340,30,TEXT("Pause"))) return;
        if(!Click(W-150,H-38,TEXT("Grimoire"))) return;
        if(!Require(bSpellbookOpen && !bBattlePaused && !bAllowTacticalPause,TEXT("live book no slowdown or pause"))) return;
        Capture(TEXT("live-book"));
        break;
    case 4:
        if(!Click(W-340,H-470+55+3*52+20,TEXT("BookSpell3"))) return;
        if(!Require(SelectedSpellSlot==3 && MagicCasts==0,TEXT("live selection still requires confirmation"))) return;
        HandleGamepadAction(TEXT("Cast"));bGamepadActive=false;
        if(!Require(MagicCasts==1 && SelectedSpellSlot==INDEX_NONE,TEXT("confirmed ward casts once"))) return;
        Capture(TEXT("ward-cast"));
        break;
    case 5:
        if(!Click(W-446+2*49+20,H-38,TEXT("Spell2"))) return;
        PC->SetMouseLocation(PixelsX/2,PixelsY/2);
        Capture(TEXT("ground-target-preview"));
        break;
    case 6:
        HandleGamepadAction(TEXT("Cancel"));bGamepadActive=false;
        PC->SetMouseLocation(FMath::RoundToInt((W-426)*Scale),FMath::RoundToInt((H-38)*Scale));
        Capture(TEXT("quick-slot-details"));
        break;
    case 7:
        if(!Click(W-150,H-38,TEXT("Grimoire"))) return;
        PC->SetMouseLocation(PixelsX/2,PixelsY/2);
        Capture(TEXT("live-book-return"));
        break;
    case 8:
        if(!Click(W-55,H-448,TEXT("CloseGrimoire"))) return;
        if(!Require(!bSpellbookOpen && !bBattlePaused,TEXT("close restores live battlefield"))) return;
        Capture(TEXT("compact-live"));
        break;
    case 9:
        HandleBattleAction(TEXT("Camera"));
        HandleBattleAction(TEXT("Grimoire"));
        PC->SetMouseLocation(PixelsX/2,PixelsY/2);
        Capture(TEXT("hero-book"));
        break;
    case 10:
        if(!Require(!bTacticalCameraActive && bSpellbookOpen && PC->bShowMouseCursor,TEXT("hero book releases pointer"))) return;
        if(!Click(W-55,H-448,TEXT("CloseGrimoire"))) return;
        Capture(TEXT("hero-controls-restored"));
        break;
    case 11:
        if(!Require(!bSpellbookOpen && !PC->bShowMouseCursor,TEXT("closing book restores hero mouse look"))) return;
        ToggleBattlePause();
        if(!Require(!bBattlePaused,TEXT("no-pause rejects pause after start"))) return;
        UE_LOG(LogTemp,Display,TEXT("SOUL_SPELLBOOK_PASS: resolution=%dx%d casts=%d elapsed=%.2f mana=%.1f"),
            PixelsX,PixelsY,MagicCasts,BattleElapsed,PlayerMana);
        FPlatformMisc::RequestExitWithStatus(false,0);
        break;
    }
}

// Staged pose inspection, separate from the live battle qualification. Never
// mutates donor assets, outcomes or ordinary play. Exit after all frames exist.
void ASoulRealtimeArenaGameMode::TickAnimationPoseProof()
{
    const double Now=FPlatformTime::Seconds();
    if(ReadabilityStarted==0) ReadabilityStarted=Now;
    if(!ReadabilityCaptureName.IsEmpty())
    {
        if(Now<ReadabilityCaptureAt || FScreenshotRequest::IsScreenshotRequested()) return;
        const FString Dir=FPaths::ProjectSavedDir()/TEXT("Screenshots/AnimationPoses");
        IFileManager::Get().MakeDirectory(*Dir,true);
        FScreenshotRequest::RequestScreenshot(Dir/(ReadabilityCaptureName+TEXT(".png")),false,false);
        UE_LOG(LogTemp,Display,TEXT("SOUL_POSE_CAPTURE: %s"),*ReadabilityCaptureName);
        ReadabilityCaptureName.Reset();ReadabilityNextCapture=Now+1.0;
        return;
    }
    if(Now-ReadabilityStarted<3 || Now<ReadabilityNextCapture) return;
    if(ReadabilityStage>=24)
    {
        UE_LOG(LogTemp,Display,TEXT("SOUL_POSE_PASS: captures=24 rosters=2"));
        FPlatformMisc::RequestExitWithStatus(false,0);return;
    }
    const int32 CaptureIndex=ReadabilityStage++;
    const int32 Side=CaptureIndex/12;
    const int32 Stage=CaptureIndex%12;
    const auto Wanted=Stage<8 ? ESoulRealtimeFormationRole::Guard : Stage<10 ? ESoulRealtimeFormationRole::Hero : ESoulRealtimeFormationRole::Apex;
    int32 Subject=INDEX_NONE;
    for(int32 I=0;I<Combatants.Num();++I)
        if(Combatants[I].Side==Side && Combatants[I].Role==Wanted) { Subject=I;break; }
    if(!Actors.IsValidIndex(Subject) || !Actors[Subject])
    {
        UE_LOG(LogTemp,Error,TEXT("SOUL_POSE_FAIL: missing subject side=%d role=%d capture=%d (use at least 30 active/pool per side)"),Side,int32(Wanted),CaptureIndex);
        FPlatformMisc::RequestExitWithStatus(false,1);return;
    }
    for(int32 I=0;I<Actors.Num();++I) if(Actors[I]) Actors[I]->SetActorHiddenInGame(I!=Subject);
    auto* Actor=Actors[Subject].Get();
    Actor->SetActorHiddenInGame(false);
    // Inspect each roster at the same clear patch. Enemy deployment positions
    // can put the inspection camera inside boundary rocks, hiding a valid corpse.
    FVector StageLocation=ResolveSpawnLocation(ArenaOrigin+FVector(0,0,100));
    StageLocation.Z+=Actor->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-96.f;
    Actor->SetActorLocationAndRotation(StageLocation,FRotator::ZeroRotator);
    UAnimationAsset* Clip=nullptr;
    if(Stage==0) Clip=ResolveStanceAnimation(Side,Wanted,0);
    else if(Stage==1) Clip=ResolveStanceAnimation(Side,Wanted,5);
    else if(Stage<6) Clip=ResolveVisualAttack(Side,Wanted,Stage-2);
    else if(Stage==6) Clip=ResolveVisualReaction(Side,Wanted);
    else if(Stage==7) Clip=ResolveStanceAnimation(Side,Wanted,2);
    else if(Stage<10) Clip=ResolveVisualAttack(Side,Wanted,Stage-7);
    else if(Stage==10) Clip=ResolveAerialFall(Side);
    else Clip=ResolveVisualDeath(Side,Wanted);
    if(!Clip)
    {
        UE_LOG(LogTemp,Error,TEXT("SOUL_POSE_FAIL: missing clip side=%d role=%d capture=%d"),Side,int32(Wanted),CaptureIndex);
        FPlatformMisc::RequestExitWithStatus(false,1);return;
    }
    Actor->GetMesh()->bPauseAnims=false;
    Actor->GetMesh()->PlayAnimation(Clip,false);
    Actor->GetMesh()->SetPosition(Clip->GetPlayLength()*(Stage==11?.95f:.42f),false);
    Actor->GetMesh()->TickAnimation(0,false);
    Actor->GetMesh()->RefreshBoneTransforms();
    Actor->GetMesh()->bPauseAnims=true;
    Combatants[Subject].bVisualAttackPlaying=true;
    if(Stage==11)
    {
        FVector P=Actor->GetMesh()->GetRelativeLocation();P.Z=-Actor->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
        Actor->GetMesh()->SetRelativeLocation(P);
    }
    // World pause prevents the usual component tick from publishing the sampled
    // pose. Flush both attachments and GPU skinning before freezing this frame.
    auto* Mesh=Actor->GetMesh();
    Mesh->UpdateComponentToWorld();
    Mesh->FinalizeBoneTransform();
    Mesh->UpdateBounds();
    Mesh->MarkRenderTransformDirty();
    Mesh->MarkRenderDynamicDataDirty();
    // A paused world can retain the previous GPU skinning buffer even after
    // socket transforms update. Recreate only this staged subject's render
    // state so the capture cannot pair an old body with newly posed equipment.
    Mesh->MarkRenderStateDirty();
    // Falling clips can move the body far from the character capsule. Frame the
    // sampled skeleton, including wings/tail, instead of cropping at actor origin.
    FBox PoseBounds(ForceInit);
    for(int32 Bone=0;Bone<Mesh->GetNumBones();++Bone)
        PoseBounds+=Mesh->GetBoneLocation(Mesh->GetBoneName(Bone));
    if(!bTacticalCameraActive) ToggleBattleCamera();
    TacticalFocus=PoseBounds.IsValid ? PoseBounds.GetCenter() : Actor->GetActorLocation();
    TacticalDistance=PoseBounds.IsValid ? FMath::Max(900.f,float(PoseBounds.GetExtent().Size())*4.5f) : 1250.f;
    TacticalRotation=FRotator(-20,140,0);
    UE_LOG(LogTemp,Display,TEXT("SOUL_POSE_FRAME: capture=%d bones=%d focus=%s distance=%.1f"),CaptureIndex,Mesh->GetNumBones(),*TacticalFocus.ToCompactString(),TacticalDistance);
    Status=FString::Printf(TEXT("STAGED ANIMATION INSPECTION / %s"),*Clip->GetName());
    ReadabilityCaptureName=FString::Printf(TEXT("%02d-%s"),CaptureIndex,*Clip->GetName());
    ReadabilityCaptureAt=Now+.8;
}
