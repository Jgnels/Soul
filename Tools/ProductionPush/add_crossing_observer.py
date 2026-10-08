from pathlib import Path
R=Path.cwd()
p=R/'Source/Soul/Private/SoulCampaignWorldActor.cpp';s=p.read_text();s=s.replace('Party->SetVisibility(true,true);SetPartyWalking(false);','Party->SetHiddenInGame(false,true);SetPartyWalking(false);',1).replace('Party->SetVisibility(!Ferry,true);SetPartyWalking(!Ferry&&TravelAlpha<1);','Party->SetHiddenInGame(Ferry,true);SetPartyWalking(!Ferry&&TravelAlpha<1);',1);p.write_text(s)
p=R/'Source/Soul/Public/SoulCampaignTerrain.h';s=p.read_text().replace('    bool FerryTravel(','    bool RouteUsesFerry(FName From,FName To);\n    bool FerryTravel(',1);p.write_text(s)
p=R/'Source/Soul/Private/SoulCampaignTerrain.cpp';s=p.read_text();key='bool FerryTravel(';i=s.index(key);s=s[:i]+'''bool RouteUsesFerry(FName From,FName To)
{
    if(!Composition())return false;FString A=From.ToString(),B=To.ToString();if(A>B)Swap(A,B);
    const auto* Ranges=Bake().FerryRanges.Find(A+TEXT("|")+B);return Ranges&&!Ranges->IsEmpty();
}
'''+s[i:];p.write_text(s)
p=R/'Source/Soul/Private/SoulFounderPlaytestCampaignActor.cpp';s=p.read_text();s=s.replace('    const int32 BeforeLevel = State->Hero.Level;','    const FName TravelOrigin = State->PlayerRegion;\n    const int32 BeforeLevel = State->Hero.Level;',1).replace('    if (State->Hero.Level > BeforeLevel)','    if (SoulCampaignTerrain::RouteUsesFerry(TravelOrigin,RegionId))\n        LastMessage += TEXT(" Ferry passage included; your company disembarks at the far landing.");\n    if (State->Hero.Level > BeforeLevel)',1);p.write_text(s)
p=R/'Source/Soul/Public/SoulFounderPlaytestGameMode.h';s=p.read_text().replace('    void TickVisualQualification(float Seconds);','    void TickCompositionTraversal(float Seconds);\n    void TickVisualQualification(float Seconds);',1);p.write_text(s)
p=R/'Source/Soul/Private/SoulFounderPlaytestGameMode.cpp';s=p.read_text().replace('    Super::Tick(Seconds);','    Super::Tick(Seconds);\n    if(FParse::Param(FCommandLine::Get(),TEXT("SoulCompositionTraversal")))\n    {if(!bDone)TickCompositionTraversal(Seconds);return;}',1);p.write_text(s)
