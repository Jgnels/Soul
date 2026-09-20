#include "SoulBattlefieldRecipe.h"

int32 FSoulBattlefieldRecipeRules::Score(const FSoulBattleContext& Context, const FSoulBattlefieldTemplate& Template)
{
    if (Context.bSiege && !Template.bSupportsSiege)
    {
        return MIN_int32;
    }

    int32 Result = Template.BaseScore;
    if (!Context.Biome.IsNone() && Template.Biomes.Contains(Context.Biome)) Result += 400;
    if (!Context.Landform.IsNone() && Template.Landforms.Contains(Context.Landform)) Result += 500;
    if (!Context.Feature.IsNone() && Template.Features.Contains(Context.Feature)) Result += 700;
    if (Context.bSettlementNearby && Template.Features.Contains(TEXT("SettlementEdge"))) Result += 300;
    if (Context.bRoadPresent && Template.Features.Contains(TEXT("Road"))) Result += 150;
    return Result;
}

FName FSoulBattlefieldRecipeRules::SelectBest(const FSoulBattleContext& Context, const TArray<FSoulBattlefieldTemplate>& Templates)
{
    FName BestId = NAME_None;
    int32 BestScore = MIN_int32;
    for (const FSoulBattlefieldTemplate& Template : Templates)
    {
        const int32 Candidate = Score(Context, Template);
        if (Candidate == MIN_int32) continue;
        if (BestId.IsNone() || Candidate > BestScore || (Candidate == BestScore && Template.Id.LexicalLess(BestId)))
        {
            BestId = Template.Id;
            BestScore = Candidate;
        }
    }
    return BestId;
}
