from pathlib import Path
R=Path.cwd()
def edit(n,pairs):
 p=R/n;s=p.read_text()
 for old,new in pairs:assert old in s,(n,old[:80]);s=s.replace(old,new,1)
 p.write_text(s)
edit('Source/Soul/Private/SoulCampaignWorldActor.cpp',[
 ('void ASoulCampaignWorldActor::BuildRoads()\n{','void ASoulCampaignWorldActor::BuildRoads()\n{\n    if(SoulCampaignTerrain::Composition())return; // Baked art; legal travel still uses the canonical graph.'),
 ('if(SoulCampaignTerrain::Enabled()&&!SoulCampaignTerrain::World())','if(SoulCampaignTerrain::Enabled()&&!SoulCampaignTerrain::World()&&!SoulCampaignTerrain::Composition())'),
 ('if(SoulCampaignTerrain::Mesa()||SoulCampaignTerrain::World()){P.Z=','if(SoulCampaignTerrain::Composition()||SoulCampaignTerrain::Mesa()||SoulCampaignTerrain::World()){P.Z='),
 ('        if(SoulCampaignTerrain::EvilCorridor())\n        {\n            float Distance','        if(SoulCampaignTerrain::EvilCorridor()||SoulCampaignTerrain::Composition())\n        {\n            float Distance'),
 ('else {TravelAlpha=1;SetPartyWalking(false);Party->SetRelativeLocation','else {TravelAlpha=1;Party->SetVisibility(true,true);SetPartyWalking(false);Party->SetRelativeLocation'),
 ('    if(!SoulCampaignTerrain::Mesa()&&!SoulCampaignTerrain::World())\n    {\n        Route=','    if(!SoulCampaignTerrain::Composition()&&!SoulCampaignTerrain::Mesa()&&!SoulCampaignTerrain::World())\n    {\n        Route='),
 ('    const FVector Direction=Route-Party->GetRelativeLocation();','    if(SoulCampaignTerrain::Composition())\n    {\n        const bool Ferry=SoulCampaignTerrain::FerryTravel(TravelFrom,TravelTo,Eased);\n        Party->SetVisibility(!Ferry,true);SetPartyWalking(!Ferry&&TravelAlpha<1);\n    }\n    const FVector Direction=Route-Party->GetRelativeLocation();')])
edit('Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp',[
 ('    const bool bExpansion = SoulCampaignExpansion::Enabled();','    const bool bComposition = SoulCampaignTerrain::Composition();\n    const bool bExpansion = SoulCampaignExpansion::Enabled();\n    if(bComposition&&(bExpansion||FParse::Param(FCommandLine::Get(),TEXT("SoulWorldTerrain"))))\n    {UE_LOG(LogSoulCampaign,Error,TEXT("SOUL_COMPOSITION_SCENARIO_FAIL conflicting experimental flag"));return;}'),
 ('bAuthoredEnvironmentProof && !SoulCampaignTerrain::EvilCorridor()','bAuthoredEnvironmentProof && !SoulCampaignTerrain::EvilCorridor() && !bComposition'),
 ('    const bool bConfigLoaded = bExpansion ?','    const bool bConfigLoaded = bComposition ? ReadData(bHumanEnvironmentProof?TEXT("CampaignComposition/HumanRuntimeProof.json"):TEXT("CampaignComposition/RuntimeProof.json"),Config) : bExpansion ?'),
 ('const auto Scenario=(bAuthoredEnvironmentProof||bExpansion) ?','const auto Scenario=(bAuthoredEnvironmentProof||bExpansion||bComposition) ?'),
 ('if (bAuthoredEnvironmentProof || FParse::Param','if (bComposition || bAuthoredEnvironmentProof || FParse::Param')])
# Preserve candidate-native lighting; retained and battle environments keep existing behavior.
edit('Source/Soul/Private/SoulFounderPlaytestGameMode.cpp',[
 ('    auto* Sun=GetWorld()->SpawnActor<ADirectionalLight>','    if(!SoulCampaignTerrain::Composition())\n    {\n    auto* Sun=GetWorld()->SpawnActor<ADirectionalLight>'),
 ('    auto* Camera=GetWorld()->SpawnActor<ASoulCampaignCamera>();','    }\n    auto* Camera=GetWorld()->SpawnActor<ASoulCampaignCamera>();'),
 ('State && (SoulCampaignTerrain::World()||SoulCampaignTerrain::Mesa()','State && (SoulCampaignTerrain::Composition()||SoulCampaignTerrain::World()||SoulCampaignTerrain::Mesa()')])
# Profile-specific camera extent, using existing navigation code.
p=R/'Source/Soul/Private/SoulCampaignCamera.cpp';s=p.read_text();s=s.replace('SoulCampaignTerrain::World()','(SoulCampaignTerrain::World()||SoulCampaignTerrain::Composition())');s=s.replace('Distance=TargetDistance=18000.f','Distance=TargetDistance=SoulCampaignTerrain::Composition()?9000.f:18000.f');s=s.replace('float ASoulCampaignCamera::GetMaximumDistance() const {return','float ASoulCampaignCamera::GetMaximumDistance() const {return SoulCampaignTerrain::Composition()?80000.f:');s=s.replace('return FBox2D(FVector2D(-4998000,-4998000),FVector2D(4998000,4998000));','return SoulCampaignTerrain::Composition()?FBox2D(FVector2D(-600000,-600000),FVector2D(600000,600000)):FBox2D(FVector2D(-4998000,-4998000),FVector2D(4998000,4998000));');p.write_text(s)
edit('Source/Soul/Private/SoulAuthoredSettlementQualification.cpp',[
 ('&& SoulCampaignTerrain::EvilCorridor()\n            && !FParse::Param','&& (SoulCampaignTerrain::EvilCorridor()||SoulCampaignTerrain::Composition())\n            && !FParse::Param'),
 ('TEXT("isolated authored proof on retained terrain")','TEXT("isolated authored proof on retained or opt-in composition terrain")')])
edit('Source/Soul/Soul.Build.cs', [('                "Data/CampaignMesa/presentation.json",','                "Data/CampaignComposition/presentation.json",\n                "Data/CampaignComposition/RuntimeProof.json",\n                "Data/CampaignComposition/HumanRuntimeProof.json",\n                "Data/CampaignCompositionLocal/Composition_3500_r2.r16",\n                "Data/CampaignMesa/presentation.json",')])
print('EXISTING_PRESENTATION_HOOKS_PATCHED')
