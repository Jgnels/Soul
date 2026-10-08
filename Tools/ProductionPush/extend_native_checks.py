from pathlib import Path
p=Path('Source/Soul/Private/Tests/SoulCampaignWorldTests.cpp');s=p.read_text();old='    auto* State=NewObject<USoulFounderPlaytestStateSubsystem>(GI);State->InitializeScenario();';new='''    // Composition presents the existing stateful Human miniature. Its real
    // GameInstance owns both campaign and settlement subsystems.
    if(SoulCampaignTerrain::Composition())GI->Init();
    auto* State=SoulCampaignTerrain::Composition()?GI->GetSubsystem<USoulFounderPlaytestStateSubsystem>():NewObject<USoulFounderPlaytestStateSubsystem>(GI);
    if(!TestNotNull(TEXT("campaign authority"),State))return false;
    State->InitializeScenario();''';assert old in s;s=s.replace(old,new,1)
old='    if(SoulCampaignTerrain::World())\n    {';new='''    if(SoulCampaignTerrain::Composition())
    {
        TestFalse(TEXT("rejected world experiment stays disabled"),SoulCampaignTerrain::World());
        TestEqual(TEXT("all canonical composition regions"),Locations.Num(),36);
        TestEqual(TEXT("all legal composition routes"),SoulCampaignTerrain::ContextRoutes().Num(),51);
        int32 FerryEdges=0;
        for(const auto& E:SoulCampaignTerrain::ContextRoutes())
        {
            const bool Ferry=SoulCampaignTerrain::RouteUsesFerry(E.Key,E.Value);FerryEdges+=Ferry;
            TestEqual(TEXT("ferry role reversible"),Ferry,SoulCampaignTerrain::RouteUsesFerry(E.Value,E.Key));
            bool FerryObserved=false;
            for(int32 I=0;I<=200;++I)
            {
                const float T=I/200.f;
                TestEqual(TEXT("ferry stations reversible"),SoulCampaignTerrain::FerryTravel(E.Key,E.Value,T),SoulCampaignTerrain::FerryTravel(E.Value,E.Key,1-T));
                FerryObserved|=SoulCampaignTerrain::FerryTravel(E.Key,E.Value,T);
            }
            TestEqual(TEXT("each ferry role has actual water passage"),Ferry,FerryObserved);
        }
        TestEqual(TEXT("two physical ferry passages serve seven legal pairs"),FerryEdges,7);
        TestTrue(TEXT("existing Human state authority initialized"),State->IsSettlementDevelopmentReady());
        GI->Shutdown();
    }
    if(SoulCampaignTerrain::World())
    {''';assert old in s;s=s.replace(old,new,1);p.write_text(s)
