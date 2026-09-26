#include "SoulFounderPlaytestHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "SoulFounderPlaytestCampaignActor.h"

void ASoulFounderPlaytestHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !GetWorld()) return;

    ASoulFounderPlaytestCampaignActor* Campaign = nullptr;
    for (TActorIterator<ASoulFounderPlaytestCampaignActor> It(GetWorld()); It; ++It)
    {
        Campaign = *It;
        break;
    }

    const float X = 28.0f;
    float Y = 28.0f;
    const float Width = FMath::Min(760.0f, Canvas->ClipX - 56.0f);
    const TArray<FString> Lines = Campaign
            ? Campaign->BuildHudLines()
            : TArray<FString>{TEXT("Starting Soul founder playtest...")};

    const float Height = 76.0f + Lines.Num() * 24.0f;
    DrawRect(FLinearColor(0.015f, 0.02f, 0.03f, 0.82f), X - 14.0f, Y - 14.0f, Width, Height);
    DrawText(
        TEXT("SOUL — FOUNDER PLAYTEST"),
        FLinearColor(0.95f, 0.84f, 0.32f),
        X,
        Y,
        GEngine ? GEngine->GetMediumFont() : nullptr,
        1.0f,
        false);
    Y += 40.0f;

    for (int32 Index = 0; Index < Lines.Num(); ++Index)
    {
        const bool bEmphasis =
            Lines[Index].StartsWith(TEXT("SKILL POINT"))
            || Lines[Index].StartsWith(TEXT("HUMAN CAPITAL"));

        DrawText(
            Lines[Index],
            bEmphasis ? FLinearColor(0.95f, 0.84f, 0.32f) : FLinearColor::White,
            X,
            Y,
            GEngine ? GEngine->GetSmallFont() : nullptr,
            1.0f,
            false);
        Y += 24.0f;
    }

    if (Campaign && Campaign->IsSkillChoiceOpen())
    {
        const float BoxY = Canvas->ClipY - 120.0f;
        const float BoxW = FMath::Min(220.0f, (Canvas->ClipX - 100.0f) / 3.0f);
        const TCHAR* Names[3] = {TEXT("[1] COMMAND"), TEXT("[2] ADVENTURE"), TEXT("[3] MAGIC")};
        for (int32 I = 0; I < 3; ++I)
        {
            const float BoxX = 30.0f + I * (BoxW + 16.0f);
            DrawRect(
                FLinearColor(0.05f, 0.08f, 0.12f, 0.92f),
                BoxX,
                BoxY,
                BoxW,
                70.0f);
            DrawText(
                Names[I],
                FLinearColor(0.95f, 0.84f, 0.32f),
                BoxX + 12.0f,
                BoxY + 22.0f,
                GEngine ? GEngine->GetSmallFont() : nullptr,
                1.0f,
                false);
        }
    }
}
