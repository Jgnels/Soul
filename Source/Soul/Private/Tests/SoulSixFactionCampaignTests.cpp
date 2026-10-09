#include "Misc/AutomationTest.h"
#include "Async/Async.h"
#include "Misc/CommandLine.h"
#include "Engine/GameInstance.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "Dom/JsonObject.h"
#include "SoulSettlementStateSubsystem.h"
#include "RBSaveSubsystem.h"
#include "RBSaveCore.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
    struct FFixture
    {
        FString Old; UGameInstance* GI; USoulFounderPlaytestStateSubsystem* S;
        explicit FFixture(bool Six=true)
        {
            Old=FCommandLine::Get();FCommandLine::Set(Six?TEXT("-SoulComposition -SoulSixFactionProof"):TEXT(""));
            GI=NewObject<UGameInstance>();GI->AddToRoot();GI->Init();S=GI->GetSubsystem<USoulFounderPlaytestStateSubsystem>();S->InitializeScenario();
            FCommandLine::Set(*Old);
        }
        ~FFixture(){GI->Shutdown();GI->RemoveFromRoot();}
    };
    TSharedPtr<FJsonObject> Read(const FRBSaveDomainState& D)
    {TSharedPtr<FJsonObject> O;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(D.Fields[0].StringValue),O);return O;}
    FRBSaveDomainState Edited(const FRBSaveDomainState& D,TFunctionRef<void(FJsonObject&)> Edit)
    {auto R=D;auto O=Read(D);Edit(*O);FJsonSerializer::Serialize(O.ToSharedRef(),TJsonWriterFactory<>::Create(&R.Fields[0].StringValue));return R;}
    FString Snapshot(USoulFounderPlaytestStateSubsystem* S)
    {FRBSaveDomainState D;FString E;return S->CaptureRBSaveDomain_Implementation(D,E)?D.Fields[0].StringValue:E;}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulSixFactionCanonicalTest,"Soul.Integration.SixFaction.CanonicalOwnershipAndControlledMovement",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoulSixFactionCanonicalTest::RunTest(const FString&)
{
    FFixture F;auto* S=F.S;TestTrue(TEXT("explicit profile initialized"),S->bInitialized&&S->IsSixFactionProfile());
    TestEqual(TEXT("isolated slot"),S->GetCampaignSaveSlotName(),FString(TEXT("Soul.Composition3500.SixFactionProof")));
    TestEqual(TEXT("canonical world"),S->World.Regions.Num(),36);int32 Edges=0,Neutral=0;
    TMap<FName,int32> Owners;for(const auto& P:S->World.Regions){Edges+=P.Value.Neighbors.Num();if(P.Value.OwnerFactionId.IsNone())++Neutral;else ++Owners.FindOrAdd(P.Value.OwnerFactionId);}
    TestEqual(TEXT("51 legal pairs"),Edges,102);TestEqual(TEXT("24 neutral"),Neutral,24);TestEqual(TEXT("six owners"),Owners.Num(),6);
    const TMap<FName,FName> Secondary={{TEXT("humans"),TEXT("crossroads")},{TEXT("dwarves"),TEXT("dwarf_forge_approach")},{TEXT("orcs"),TEXT("orc_war_camp")},{TEXT("vikings"),TEXT("viking_forest_track")},{TEXT("nature"),TEXT("nature_forest_clearing")},{TEXT("dark"),TEXT("dark_castle_approach")}};
    for(FName Id:FSoulCampaignRules::CanonicalFactions())
    {
        TestEqual(TEXT("two starting possessions"),Owners.FindRef(Id),2);FSoulFactionCampaignState Before,After;FString Error;
        TestTrue(TEXT("exact faction army inspectable"),S->InspectFactionArmy(Id,Before));TestEqual(TEXT("army owner"),Before.Army.FactionId,Id);
        TestEqual(TEXT("army stationed in its own region"),S->World.Regions[Before.Army.RegionId].OwnerFactionId,Id);
        const FString Original=Snapshot(S);TestFalse(TEXT("non-edge/self move rejected"),S->MoveFactionArmy(Id,Before.Army.RegionId,Error));TestEqual(TEXT("rejected movement atomic"),Snapshot(S),Original);
        TestTrue(TEXT("owned neighbor movement"),S->MoveFactionArmy(Id,Secondary[Id],Error));S->InspectFactionArmy(Id,After);
        TestEqual(TEXT("exact army identity retained"),After.Army.ArmyId,Before.Army.ArmyId);TestEqual(TEXT("one AP charged to right faction"),After.Economy.ActionPoints,Before.Economy.ActionPoints-1);
        TestEqual(TEXT("exact destination"),After.Army.RegionId,Secondary[Id]);TestEqual(TEXT("count retained"),After.Army.TroopCount,Before.Army.TroopCount);
    }
    const FString BeforeAI=Snapshot(S);S->AdvanceEnemyAI();TestEqual(TEXT("AI does not mutate state"),Snapshot(S),BeforeAI);
    S->AdvanceDay();for(FName Id:FSoulCampaignRules::CanonicalFactions()){FSoulFactionCampaignState A;S->InspectFactionArmy(Id,A);TestEqual(TEXT("shared day via existing rules"),A.Economy.Day,2);TestEqual(TEXT("actions reset"),A.Economy.ActionPoints,A.Economy.MaxActionPoints);}
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulSixFactionRestoreTest,"Soul.Integration.SixFaction.AtomicAdversarialRestore",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoulSixFactionRestoreTest::RunTest(const FString&)
{
    FFixture F;auto* S=F.S;FRBSaveDomainState Save;FString Error;TestTrue(TEXT("capture"),S->CaptureRBSaveDomain_Implementation(Save,Error));
    // Distinct owner balances and movement make accidental state collapse visible.
    Save=Edited(Save,[](FJsonObject& O){int32 I=0;for(auto V:O.GetArrayField(TEXT("other_faction_states"))){auto A=V->AsObject();A->GetObjectField(TEXT("resources"))->SetNumberField(TEXT("gold"),1000+137*I);A->SetNumberField(TEXT("ap"),I%4);++I;}});
    TestTrue(TEXT("valid distinct faction balances restore"),S->RestoreRBSaveDomain_Implementation(Save,Error));const FString Expected=Snapshot(S);
    TArray<TPair<FString,TFunction<void(FJsonObject&)>>> Cases;
    Cases.Add({TEXT("wrong profile"),[](FJsonObject& O){O.SetStringField(TEXT("profile"),TEXT("Soul.Composition3500.OrcProof"));}});
    Cases.Add({TEXT("missing profile"),[](FJsonObject& O){O.RemoveField(TEXT("profile"));}});
    Cases.Add({TEXT("old nine-region shape"),[](FJsonObject& O){auto A=O.GetArrayField(TEXT("canonical_regions"));A.SetNum(9);O.SetArrayField(TEXT("canonical_regions"),A);}});
    Cases.Add({TEXT("wrong region count"),[](FJsonObject& O){auto A=O.GetArrayField(TEXT("canonical_regions"));A.Pop();O.SetArrayField(TEXT("canonical_regions"),A);}});
    Cases.Add({TEXT("missing canonical region"),[](FJsonObject& O){O.GetArrayField(TEXT("canonical_regions"))[0]->AsObject()->SetStringField(TEXT("id"),TEXT("invented_region"));}});
    Cases.Add({TEXT("duplicate region"),[](FJsonObject& O){auto A=O.GetArrayField(TEXT("canonical_regions"));A[1]=A[0];O.SetArrayField(TEXT("canonical_regions"),A);}});
    Cases.Add({TEXT("unknown region owner"),[](FJsonObject& O){O.GetArrayField(TEXT("canonical_regions"))[0]->AsObject()->SetStringField(TEXT("owner"),TEXT("alien_faction"));}});
    Cases.Add({TEXT("owner mirror disagreement"),[](FJsonObject& O){O.GetObjectField(TEXT("owners"))->SetStringField(TEXT("human_capital"),TEXT("dark"));}});
    Cases.Add({TEXT("unknown faction"),[](FJsonObject& O){O.GetArrayField(TEXT("other_faction_states"))[0]->AsObject()->SetStringField(TEXT("faction"),TEXT("aliens"));}});
    Cases.Add({TEXT("duplicate faction"),[](FJsonObject& O){auto A=O.GetArrayField(TEXT("other_faction_states"));A[1]=A[0];O.SetArrayField(TEXT("other_faction_states"),A);}});
    Cases.Add({TEXT("wrong army owner"),[](FJsonObject& O){O.GetArrayField(TEXT("other_faction_states"))[0]->AsObject()->SetStringField(TEXT("army_owner"),TEXT("humans"));}});
    Cases.Add({TEXT("wrong army identity"),[](FJsonObject& O){O.GetArrayField(TEXT("other_faction_states"))[0]->AsObject()->SetStringField(TEXT("army_id"),TEXT("humans.primary"));}});
    Cases.Add({TEXT("unsupported roster substitution"),[](FJsonObject& O){O.GetArrayField(TEXT("other_faction_states"))[2]->AsObject()->SetStringField(TEXT("unit"),TEXT("human_knight"));}});
    Cases.Add({TEXT("foreign occupied region"),[](FJsonObject& O){O.GetArrayField(TEXT("other_faction_states"))[0]->AsObject()->SetStringField(TEXT("region"),TEXT("human_capital"));}});
    Cases.Add({TEXT("negative troops"),[](FJsonObject& O){O.GetArrayField(TEXT("other_faction_states"))[0]->AsObject()->SetNumberField(TEXT("troops"),-1);}});
    Cases.Add({TEXT("fractional troops"),[](FJsonObject& O){O.GetArrayField(TEXT("other_faction_states"))[0]->AsObject()->SetNumberField(TEXT("troops"),1.5);}});
    Cases.Add({TEXT("overflow gold"),[](FJsonObject& O){O.GetArrayField(TEXT("other_faction_states"))[0]->AsObject()->GetObjectField(TEXT("resources"))->SetNumberField(TEXT("gold"),1.e20);}});
    Cases.Add({TEXT("missing gold"),[](FJsonObject& O){O.GetArrayField(TEXT("other_faction_states"))[0]->AsObject()->GetObjectField(TEXT("resources"))->RemoveField(TEXT("gold"));}});
    Cases.Add({TEXT("invalid AP"),[](FJsonObject& O){O.GetArrayField(TEXT("other_faction_states"))[0]->AsObject()->SetNumberField(TEXT("ap"),4);}});
    Cases.Add({TEXT("foreign day"),[](FJsonObject& O){O.GetArrayField(TEXT("other_faction_states"))[0]->AsObject()->SetNumberField(TEXT("day"),5);}});
    Cases.Add({TEXT("null army row"),[](FJsonObject& O){auto A=O.GetArrayField(TEXT("other_faction_states"));A[0]=MakeShared<FJsonValueNull>();O.SetArrayField(TEXT("other_faction_states"),A);}});
    for(const auto& C:Cases)
    {
        const auto Bad=Edited(Save,C.Value);TestFalse(*C.Key,S->RestoreRBSaveDomain_Implementation(Bad,Error));
        TestFalse(TEXT("explicit reason"),Error.IsEmpty());TestEqual(*(C.Key+TEXT(" leaves live state exact")),Snapshot(S),Expected);
    }
    FFixture Legacy(false);FRBSaveDomainState Old;Legacy.S->CaptureRBSaveDomain_Implementation(Old,Error);
    TestFalse(TEXT("actual nine-region save rejected"),S->RestoreRBSaveDomain_Implementation(Old,Error));TestEqual(TEXT("no 9-to-36 mutation"),Snapshot(S),Expected);
    const FString LegacyBefore=Snapshot(Legacy.S);TestFalse(TEXT("six-faction into default rejected"),Legacy.S->RestoreRBSaveDomain_Implementation(Save,Error));TestEqual(TEXT("default untouched"),Snapshot(Legacy.S),LegacyBefore);
    FFixture Fresh;TestTrue(TEXT("new subsystem restores all faction state"),Fresh.S->RestoreRBSaveDomain_Implementation(Save,Error));TestEqual(TEXT("exact fresh snapshot"),Snapshot(Fresh.S),Expected);
    const auto OldUnbound=Edited(Save,[](FJsonObject& O){for(const auto& V:O.GetArrayField(TEXT("other_faction_states")))if(V->AsObject()->GetStringField(TEXT("faction"))==TEXT("vikings"))V->AsObject()->SetStringField(TEXT("unit"),TEXT("None"));});
    TestTrue(TEXT("earlier unbound Viking save remains accepted without migration"),Fresh.S->RestoreRBSaveDomain_Implementation(OldUnbound,Error));
    FSoulFactionCampaignState Viking;Fresh.S->InspectFactionArmy(TEXT("vikings"),Viking);
    TestTrue(TEXT("saved unbound roster remains unbound"),Viking.Army.UnitId.IsNone());
    Fresh.S->PlayerRegion=Fresh.S->World.Regions[Viking.Army.RegionId].Neighbors[0];
    FSoulCampaignBattleDescriptor Unbound;TestFalse(TEXT("old unbound army cannot silently gain new roster"),Fresh.S->BuildBattleDescriptor(Viking.Army.RegionId,Unbound,Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulSixFactionAdmissionTest,"Soul.Integration.SixFaction.ExactBattleRejection",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoulSixFactionAdmissionTest::RunTest(const FString&)
{
    FFixture F;auto* S=F.S;
    for(FName Id:FSoulCampaignRules::CanonicalFactions())
    {
        if(Id==TEXT("humans"))continue;FSoulFactionCampaignState A;S->InspectFactionArmy(Id,A);
        S->PlayerRegion=S->World.Regions[A.Army.RegionId].Neighbors[0]; // Test arrangement only, not runtime travel.
        const FString Before=Snapshot(S);FSoulCampaignBattleDescriptor D;FString Error;
        const bool Supported=!FSoulCampaignRules::AdmittedStrategicUnit(Id).IsNone();
        TestEqual(TEXT("only exact admitted pair accepted"),S->BuildBattleDescriptor(A.Army.RegionId,D,Error),Supported);
        TestEqual(TEXT("request leaves state unchanged"),Snapshot(S),Before);
        if(Supported){TestEqual(TEXT("exact faction"),D.EnemyFaction,Id);TestEqual(TEXT("exact roster"),D.EnemyUnitId,A.Army.UnitId);}
        else {TestTrue(TEXT("explicit unsupported binding reason"),Error.Contains(TEXT("Unsupported exact battle binding")));TestFalse(TEXT("unsupported encounter cannot commit"),S->BeginBattle(A.Army.RegionId));TestEqual(TEXT("no AP/owner/pending mutation"),Snapshot(S),Before);}
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulSixFactionDomainRecoveryTest,"Soul.Integration.SixFaction.RBSaveCrossDomainRollback",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoulSixFactionDomainRecoveryTest::RunTest(const FString&)
{
    FFixture F;auto* S=F.S;auto* Save=F.GI->GetSubsystem<URBSaveSubsystem>();auto* Towns=F.GI->GetSubsystem<USoulSettlementStateSubsystem>();
    const TArray<FName> Both={TEXT("Soul.Campaign"),TEXT("Soul.Settlements")};FString Error;rb::save::Snapshot Saved;
    TestTrue(TEXT("capture existing RBSave domains"),Save->CaptureRegisteredDomains(S->GetCampaignSaveSlotName(),Saved,Error,&Both));
    S->AdvanceDay();S->MoveFactionArmy(TEXT("orcs"),TEXT("orc_war_camp"),Error);S->MovePlayerTo(TEXT("crossroads"));
    const FString Live=Snapshot(S);FRBSaveDomainState TownBefore;Towns->CaptureRBSaveDomain_Implementation(TownBefore,Error);
    for(const std::string DomainId:{std::string("Soul.Campaign"),std::string("Soul.Settlements")})
    {
        auto Broken=Saved;for(auto& D:Broken.domains)if(D.id==DomainId)D.fields[DomainId=="Soul.Campaign"?"CampaignJson":"StateJson"]=std::string("{}");
        TestFalse(TEXT("corrupt domain rejects both"),Save->RestoreRegisteredDomains(Broken,Error,&Both));
        TestFalse(TEXT("rollback succeeds"),Error.Contains(TEXT("RECOVERY FAILED")));
        TestEqual(TEXT("all six faction state remains exact"),Snapshot(S),Live);
        FRBSaveDomainState TownAfter;Towns->CaptureRBSaveDomain_Implementation(TownAfter,Error);
        TestEqual(TEXT("settlement state also preserved"),TownAfter.Fields[0].StringValue,TownBefore.Fields[0].StringValue);
    }
    TestTrue(TEXT("valid domains remain restorable"),Save->RestoreRegisteredDomains(Saved,Error,&Both));
    TestEqual(TEXT("day restored"),S->Economy.Day,1);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulSixFactionConsequenceTest,"Soul.Integration.SixFaction.BattleConsequencesPreserveOtherFactions",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoulSixFactionConsequenceTest::RunTest(const FString&)
{
    for(FName Id:TArray<FName>{TEXT("dwarves"),TEXT("orcs")})for(bool Won:{true,false})
    {
        FFixture F;auto* S=F.S;FSoulFactionCampaignState Target;S->InspectFactionArmy(Id,Target);
        const FName Origin=S->World.Regions[Target.Army.RegionId].Neighbors[0];S->PlayerRegion=Origin;
        // Arrange a legal allied origin in this isolated state test, then exercise
        // the real descriptor/result/save path. This is not a runtime travel claim.
        S->World.Regions[Origin].OwnerFactionId=S->PlayerFaction;
        TestTrue(TEXT("supported exact encounter"),S->BeginBattle(Target.Army.RegionId));const auto D=S->PendingBattle;
        FSoulCampaignBattleResult Result;Result.EncounterId=D.EncounterId;Result.TargetRegion=D.TargetRegion;Result.bPlayerWon=Won;
        Result.PlayerSurvivors=Won?17:0;Result.EnemySurvivors=Won?0:9;Result.PlayerManaRemaining=D.PlayerMana;
        TestTrue(TEXT("real result validator applies"),S->ApplyBattleResult(Result));
        for(FName Other:FSoulCampaignRules::CanonicalFactions())
        {
            FSoulFactionCampaignState A;S->InspectFactionArmy(Other,A);
            TestEqual(TEXT("identity never swaps"),A.Army.FactionId,Other);
            if(Other!=Id&&Other!=S->PlayerFaction)TestEqual(TEXT("unrelated armies untouched"),A.Army.TroopCount,30);
        }
        FSoulFactionCampaignState After;S->InspectFactionArmy(Id,After);TestEqual(TEXT("right faction loses bodies"),After.Army.TroopCount,Result.EnemySurvivors);
        FRBSaveDomainState Saved;FString Error;TestTrue(TEXT("capture result"),S->CaptureRBSaveDomain_Implementation(Saved,Error));
        FFixture Fresh;TestTrue(TEXT("six-faction result restore"),Fresh.S->RestoreRBSaveDomain_Implementation(Saved,Error));TestEqual(TEXT("exact result snapshot"),Snapshot(Fresh.S),Snapshot(S));
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulControlledActionTest,"Soul.Integration.SixFaction.ControlledActionAdmission",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoulControlledActionTest::RunTest(const FString&)
{
    FFixture F; auto* S=F.S; FString Error; FSoulControlledCampaignAction A;
    const FString Initial=Snapshot(S);
    auto Rejected=[&](FName Faction,FName Army,FName Source,FName Target)
    {
        const FString Before=Snapshot(S);
        TestFalse(TEXT("invalid proposal rejected"),S->PrepareControlledAction(Faction,Army,Source,Target,A,Error));
        TestFalse(TEXT("explicit rejection"),Error.IsEmpty());
        TestEqual(TEXT("rejection leaves campaign exact"),Snapshot(S),Before);
        TestTrue(TEXT("failed preparation clears proposal"),A.ExpectedCampaignState.IsEmpty());
    };
    Rejected(TEXT("aliens"),TEXT("aliens.primary"),TEXT("human_capital"),TEXT("crossroads"));
    Rejected(TEXT("humans"),TEXT("missing"),TEXT("human_capital"),TEXT("crossroads"));
    Rejected(TEXT("humans"),TEXT("orcs.primary"),TEXT("human_capital"),TEXT("crossroads"));
    Rejected(TEXT("humans"),TEXT("humans.primary"),TEXT("crossroads"),TEXT("human_capital"));
    Rejected(TEXT("humans"),TEXT("humans.primary"),TEXT("human_capital"),TEXT("invalid"));
    Rejected(TEXT("humans"),TEXT("humans.primary"),TEXT("human_capital"),TEXT("dark_fortress"));
    S->World.Regions[TEXT("crossroads")].OwnerFactionId=TEXT("unknown_owner");
    Rejected(TEXT("humans"),TEXT("humans.primary"),TEXT("human_capital"),TEXT("crossroads"));
    TestEqual(TEXT("malformed destination owner reason"),Error,FString(TEXT("REJECT_DESTINATION_OWNER")));
    S->World.Regions[TEXT("crossroads")].OwnerFactionId=TEXT("humans");
    S->World.Regions[TEXT("human_capital")].OwnerFactionId=TEXT("orcs");
    Rejected(TEXT("humans"),TEXT("humans.primary"),TEXT("human_capital"),TEXT("crossroads"));
    TestEqual(TEXT("foreign source reason"),Error,FString(TEXT("REJECT_SOURCE_OWNER")));
    S->World.Regions[TEXT("human_capital")].OwnerFactionId=TEXT("humans");
    S->bPersistenceBusy=true;
    Rejected(TEXT("humans"),TEXT("humans.primary"),TEXT("human_capital"),TEXT("crossroads"));
    S->bPersistenceBusy=false;
    S->Economy.ActionPoints=0;
    Rejected(TEXT("humans"),TEXT("humans.primary"),TEXT("human_capital"),TEXT("crossroads"));
    S->Economy.ActionPoints=3;
    TestTrue(TEXT("legal preparation"),S->PrepareControlledAction(TEXT("humans"),TEXT("humans.primary"),TEXT("human_capital"),TEXT("crossroads"),A,Error));
    TestEqual(TEXT("preparation read-only"),Snapshot(S),Initial);
    const auto OffThread=Async(EAsyncExecution::ThreadPool,[S,A]()
    {
        FString PrepareError,ExecuteError;FSoulControlledCampaignAction Proposal;
        const bool P=S->PrepareControlledAction(TEXT("humans"),TEXT("humans.primary"),TEXT("human_capital"),TEXT("crossroads"),Proposal,PrepareError);
        const bool X=S->ExecuteControlledAction(A,ExecuteError);
        return TArray<FString>{P?FString(TEXT("accepted")):PrepareError,X?FString(TEXT("accepted")):ExecuteError,Proposal.ExpectedCampaignState.IsEmpty()?TEXT("cleared"):TEXT("dirty")};
    }).Get();
    TestEqual(TEXT("off-thread preparation rejected"),OffThread[0],FString(TEXT("REJECT_NOT_GAME_THREAD")));
    TestEqual(TEXT("off-thread execution rejected"),OffThread[1],FString(TEXT("REJECT_NOT_GAME_THREAD")));
    TestEqual(TEXT("off-thread proposal cleared"),OffThread[2],FString(TEXT("cleared")));
    TestEqual(TEXT("off-thread calls leave live state exact"),Snapshot(S),Initial);
    auto Bad=A; Bad.ExpectedProfile=TEXT("Soul.VerticalCampaign");
    TestFalse(TEXT("wrong save profile rejected"),S->ExecuteControlledAction(Bad,Error));
    TestEqual(TEXT("profile rejection exact"),Snapshot(S),Initial);
    ++S->CampaignLoadRevision;
    TestFalse(TEXT("restored state invalidates proposal"),S->ExecuteControlledAction(A,Error));
    --S->CampaignLoadRevision;
    S->World.Regions[TEXT("crossroads")].OwnerFactionId=NAME_None;
    FString Changed=Snapshot(S);
    TestFalse(TEXT("changed destination rejects stale action"),S->ExecuteControlledAction(A,Error));
    TestEqual(TEXT("no stale capture"),Snapshot(S),Changed);
    S->World.Regions[TEXT("crossroads")].OwnerFactionId=TEXT("humans");
    S->Economy.Resources[TEXT("gold")]+=1; Changed=Snapshot(S);
    TestFalse(TEXT("other campaign mutation rejects proposal"),S->ExecuteControlledAction(A,Error));
    TestEqual(TEXT("no stale spend"),Snapshot(S),Changed);
    S->Economy.Resources[TEXT("gold")]-=1;
    const TMap<FName,FName> Targets={{TEXT("humans"),TEXT("crossroads")},{TEXT("dwarves"),TEXT("dwarf_forge_approach")},{TEXT("orcs"),TEXT("orc_war_camp")},{TEXT("vikings"),TEXT("viking_forest_track")},{TEXT("nature"),TEXT("nature_forest_clearing")},{TEXT("dark"),TEXT("dark_castle_approach")}};
    for(FName Id:FSoulCampaignRules::CanonicalFactions())
    {
        FSoulFactionCampaignState Before,After; S->InspectFactionArmy(Id,Before);
        TestTrue(TEXT("controlled six-faction proposal"),S->PrepareControlledAction(Id,Before.Army.ArmyId,Before.Army.RegionId,Targets[Id],A,Error));
        TestTrue(TEXT("normal authority executes"),S->ExecuteControlledAction(A,Error));
        S->InspectFactionArmy(Id,After);
        TestEqual(TEXT("correct faction location"),After.Army.RegionId,Targets[Id]);
        TestEqual(TEXT("correct faction pays AP"),After.Economy.ActionPoints,Before.Economy.ActionPoints-1);
        const FString Executed=Snapshot(S);
        TestFalse(TEXT("proposal cannot be replayed"),S->ExecuteControlledAction(A,Error));
        TestEqual(TEXT("replay rejection exact"),Snapshot(S),Executed);
    }
    const FString BeforeAI=Snapshot(S); S->AdvanceEnemyAI();
    TestEqual(TEXT("autonomous AI stays off"),Snapshot(S),BeforeAI);
    FFixture Captures;
    const TMap<FName,TArray<FName>> Paths={
        {TEXT("humans"),{TEXT("crossroads"),TEXT("old_quarry")}},
        {TEXT("dwarves"),{TEXT("dwarf_forge_approach"),TEXT("dwarf_high_quarry")}},
        {TEXT("orcs"),{TEXT("orc_badlands")}}, {TEXT("vikings"),{TEXT("viking_fjord_ridge")}},
        {TEXT("nature"),{TEXT("nature_river_woodland")}}, {TEXT("dark"),{TEXT("dark_ash_plain")}}};
    for(FName Id:FSoulCampaignRules::CanonicalFactions())
    {
        TestTrue(TEXT("capture target initially neutral"),Captures.S->World.Regions[Paths[Id].Last()].OwnerFactionId.IsNone());
        for(FName Target:Paths[Id])
        {
            FSoulFactionCampaignState Army;Captures.S->InspectFactionArmy(Id,Army);
            TestTrue(TEXT("neutral proposal admitted without a combat roster"),Captures.S->PrepareControlledAction(Id,Army.Army.ArmyId,Army.Army.RegionId,Target,A,Error));
            TestTrue(TEXT("normal capture executed"),Captures.S->ExecuteControlledAction(A,Error));
            TestEqual(TEXT("right capturing owner"),Captures.S->World.Regions[Target].OwnerFactionId,Id);
            TestFalse(TEXT("neutral capture never starts combat"),Captures.S->HasPendingBattle());
        }
    }
    FRBSaveDomainState Captured;Captures.S->CaptureRBSaveDomain_Implementation(Captured,Error);
    FFixture Restored;TestTrue(TEXT("all-six captures restore"),Restored.S->RestoreRBSaveDomain_Implementation(Captured,Error));
    TestEqual(TEXT("capture snapshot exact"),Snapshot(Restored.S),Snapshot(Captures.S));
    FFixture Legacy(false);
    TestFalse(TEXT("default profile cannot execute six-faction action"),Legacy.S->ExecuteControlledAction(A,Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulControlledReverseTest,"Soul.Integration.SixFaction.ControlledReverseBattleConsequences",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoulControlledReverseTest::RunTest(const FString&)
{
    for(FName Id:TArray<FName>{TEXT("dwarves"),TEXT("orcs"),TEXT("vikings")}) for(bool Won:{false,true})
    {
        FFixture F;auto* S=F.S;FString Error;
        const TArray<FName> Human={TEXT("crossroads"),TEXT("forest_edge"),TEXT("north_pass")};
        auto Move=[&](FName Who,FName Target)
        {
            FSoulFactionCampaignState A;S->InspectFactionArmy(Who,A);
            if(A.Economy.ActionPoints==0){S->AdvanceDay();S->InspectFactionArmy(Who,A);}
            FSoulControlledCampaignAction Proposal;
            return S->PrepareControlledAction(Who,A.Army.ArmyId,A.Army.RegionId,Target,Proposal,Error)&&S->ExecuteControlledAction(Proposal,Error);
        };
        for(FName R:Human)TestTrue(TEXT("Human legally occupies pass"),Move(TEXT("humans"),R));
        TArray<FName> Steps;
        if(Id==TEXT("dwarves"))Steps={TEXT("dwarf_forge_approach")};
        if(Id==TEXT("vikings"))Steps={TEXT("viking_fjord_ridge"),TEXT("viking_snow_pass"),TEXT("mountain_shrine"),TEXT("dwarf_mountain_pass")};
        for(FName R:Steps)TestTrue(TEXT("attacker stages legally"),Move(Id,R));
        FSoulFactionCampaignState Before;S->InspectFactionArmy(Id,Before);
        if(Before.Economy.ActionPoints==0){S->AdvanceDay();S->InspectFactionArmy(Id,Before);}
        const auto HumanEconomy=S->Economy;const auto HumanMana=S->Hero.Mana;
        FSoulControlledCampaignAction A;
        TestTrue(TEXT("reverse proposal admitted"),S->PrepareControlledAction(Id,Before.Army.ArmyId,Before.Army.RegionId,TEXT("north_pass"),A,Error));
        const FString Unchanged=Snapshot(S);
        const auto Recipes=S->BattlefieldTemplates;S->BattlefieldTemplates.Reset();
        TestFalse(TEXT("lost approach rejects before mutation"),S->ExecuteControlledAction(A,Error));
        TestEqual(TEXT("failed battlefield admission exact"),Snapshot(S),Unchanged);S->BattlefieldTemplates=Recipes;
        // Admission can become unavailable after preparation. A pending bridge
        // must reject without AP/ownership/save mutation or replacing that battle.
        auto* Bridge=F.GI->GetSubsystem<USoulCampaignBattleBridge>();FSoulCampaignBattleDescriptor Occupied;
        TestTrue(TEXT("build occupied bridge fixture"),S->BuildFactionBattleDescriptor(Id,TEXT("north_pass"),Occupied,Error));
        TestTrue(TEXT("existing bridge occupied"),Bridge->BeginEncounter(Occupied));
        TestFalse(TEXT("late bridge occupancy rejects"),S->ExecuteControlledAction(A,Error));
        TestEqual(TEXT("bridge rejection exact campaign"),Snapshot(S),Unchanged);
        TestEqual(TEXT("occupied encounter preserved"),Bridge->GetPendingEncounter()->EncounterId,Occupied.EncounterId);
        FSoulCampaignBattleResult Release;Release.EncounterId=Occupied.EncounterId;Release.TargetRegion=Occupied.TargetRegion;Release.bPlayerWon=false;Release.PlayerSurvivors=0;Release.EnemySurvivors=17;
        AddExpectedError(TEXT("Rejected campaign result"),EAutomationExpectedErrorFlags::Contains,1);
        TestTrue(TEXT("test fixture clears bridge through normal result validation"),Bridge->ResolveEncounter(Release));
        TestEqual(TEXT("no campaign pending means no spurious result mutation"),Snapshot(S),Unchanged);
        TestTrue(TEXT("reverse battle commits"),S->ExecuteControlledAction(A,Error));
        const auto D=S->PendingBattle;
        TestEqual(TEXT("attacker faction"),D.PlayerFaction,Id);TestEqual(TEXT("defender Human"),D.EnemyFaction,FName(TEXT("humans")));
        TestEqual(TEXT("exact attacker unit"),D.PlayerUnitId,Before.Army.UnitId);TestEqual(TEXT("exact Human unit"),D.EnemyUnitId,FName(TEXT("human_knight")));
        TestEqual(TEXT("Human AP is not spent"),S->Economy.ActionPoints,HumanEconomy.ActionPoints);
        FSoulFactionCampaignState Paid;S->InspectFactionArmy(Id,Paid);TestEqual(TEXT("acting army pays"),Paid.Economy.ActionPoints,Before.Economy.ActionPoints-1);
        FSoulCampaignBattleResult R;R.EncounterId=D.EncounterId;R.TargetRegion=D.TargetRegion;R.bPlayerWon=Won;
        R.PlayerSurvivors=Won?13:0;R.EnemySurvivors=Won?0:17;R.PlayerManaRemaining=0;R.PlayerReinforcements=2;R.EnemyReinforcements=1;
        // Arranged authority test only. Natural combat is a separate rendered runtime gate.
        TestTrue(TEXT("valid directed result accepted"),S->ApplyBattleResult(R));
        S->InspectFactionArmy(Id,Paid);
        TestEqual(TEXT("attacker survivor ledger"),Paid.Army.TroopCount,R.PlayerSurvivors);
        TestEqual(TEXT("Human defender survivor ledger"),S->PlayerArmy.FindRef(S->PlayerUnitId),R.EnemySurvivors);
        TestEqual(TEXT("attacker return location"),Paid.Army.RegionId,Won?D.TargetRegion:D.SourceRegion);
        TestEqual(TEXT("Human identity/location not swapped"),S->PlayerRegion,D.TargetRegion);
        TestEqual(TEXT("correct capture"),S->World.Regions[D.TargetRegion].OwnerFactionId,Won?Id:FName(TEXT("humans")));
        TestEqual(TEXT("Human hero untouched"),S->Hero.Mana,HumanMana);
        TestEqual(TEXT("occupied-region army excludes defeated colocated army"),S->ArmyCountAtRegion(D.TargetRegion),Won?R.PlayerSurvivors:R.EnemySurvivors);
        FRBSaveDomainState Save;TestTrue(TEXT("capture directed result"),S->CaptureRBSaveDomain_Implementation(Save,Error));
        FFixture Fresh;TestTrue(TEXT("fresh authority restores all six armies"),Fresh.S->RestoreRBSaveDomain_Implementation(Save,Error));
        TestEqual(TEXT("fresh snapshot exact"),Snapshot(Fresh.S),Snapshot(S));
        TestFalse(TEXT("result cannot apply twice"),S->ApplyBattleResult(R));
    }
    for(FName Id:TArray<FName>{TEXT("nature"),TEXT("dark")})
    {
        FFixture F;auto* S=F.S;
        if(Id==TEXT("nature"))
        {
            FRBSaveDomainState Saved;FString Error;S->CaptureRBSaveDomain_Implementation(Saved,Error);
            const auto Unbound=Edited(Saved,[](FJsonObject& O){for(const auto& V:O.GetArrayField(TEXT("other_faction_states")))if(V->AsObject()->GetStringField(TEXT("faction"))==TEXT("nature"))V->AsObject()->SetStringField(TEXT("unit"),TEXT("None"));});
            TestTrue(TEXT("earlier unbound Nature save preserved"),S->RestoreRBSaveDomain_Implementation(Unbound,Error));
        }
        FSoulFactionCampaignState Army;S->InspectFactionArmy(Id,Army);
        TestTrue(TEXT("unsupported fixture has no roster"),Army.Army.UnitId.IsNone());
        // Minimal hostile test arrangement; production observer never overrides owners.
        const FName Target=S->World.Regions[Army.Army.RegionId].Neighbors[0];
        S->PlayerRegion=Target;S->World.Regions[Target].OwnerFactionId=TEXT("humans");
        const FString Before=Snapshot(S);FString Error;FSoulControlledCampaignAction A;
        TestFalse(TEXT("unbound faction hostile proposal rejects"),S->PrepareControlledAction(Id,Army.Army.ArmyId,Army.Army.RegionId,Target,A,Error));
        TestEqual(TEXT("unsupported roster remains exact"),Snapshot(S),Before);
        TestEqual(TEXT("explicit unsupported pair reason"),Error,FString(TEXT("REJECT_UNSUPPORTED_ORDERED_PAIR")));
    }
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulControlledOtherPairsTest,"Soul.Integration.SixFaction.ControlledNonHumanPairs",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoulControlledOtherPairsTest::RunTest(const FString&)
{
    struct FPlan {FName Attacker,Defender;TArray<FName> AttackSteps,DefendSteps;FName Target;};
    const FPlan Plans[]={
        {TEXT("dwarves"),TEXT("orcs"),{TEXT("dwarf_forge_approach")},{TEXT("north_pass")},TEXT("north_pass")},
        {TEXT("orcs"),TEXT("dwarves"),{},{TEXT("dwarf_forge_approach"),TEXT("north_pass")},TEXT("north_pass")},
        {TEXT("dwarves"),TEXT("vikings"),{TEXT("dwarf_forge_approach"),TEXT("north_pass")},{TEXT("viking_fjord_ridge"),TEXT("viking_snow_pass"),TEXT("mountain_shrine"),TEXT("dwarf_mountain_pass")},TEXT("dwarf_mountain_pass")},
        {TEXT("vikings"),TEXT("dwarves"),{TEXT("viking_fjord_ridge"),TEXT("viking_snow_pass"),TEXT("mountain_shrine")},{TEXT("dwarf_forge_approach"),TEXT("north_pass"),TEXT("dwarf_mountain_pass")},TEXT("dwarf_mountain_pass")},
        {TEXT("orcs"),TEXT("vikings"),{},{TEXT("viking_fjord_ridge"),TEXT("viking_snow_pass"),TEXT("mountain_shrine"),TEXT("dwarf_mountain_pass"),TEXT("north_pass")},TEXT("north_pass")},
        {TEXT("vikings"),TEXT("orcs"),{TEXT("viking_fjord_ridge"),TEXT("viking_snow_pass"),TEXT("mountain_shrine"),TEXT("dwarf_mountain_pass")},{TEXT("north_pass")},TEXT("north_pass")}
    };
    for(const auto& P:Plans)for(bool Won:{false,true})
    {
        FFixture F;auto* S=F.S;FString Error;
        auto Travel=[&](FName Id,const TArray<FName>& Path)
        {
            for(FName Target:Path)
            {
                FSoulFactionCampaignState A;S->InspectFactionArmy(Id,A);
                if(A.Economy.ActionPoints<1){S->AdvanceDay();S->InspectFactionArmy(Id,A);}
                FSoulControlledCampaignAction Action;
                if(!S->PrepareControlledAction(Id,A.Army.ArmyId,A.Army.RegionId,Target,Action,Error)||!S->ExecuteControlledAction(Action,Error))return false;
            }
            return true;
        };
        if(!TestTrue(TEXT("defender legal staging"),Travel(P.Defender,P.DefendSteps))||!TestTrue(TEXT("attacker legal staging"),Travel(P.Attacker,P.AttackSteps)))continue;
        FSoulFactionCampaignState Before,Defender;S->InspectFactionArmy(P.Attacker,Before);S->InspectFactionArmy(P.Defender,Defender);
        if(Before.Economy.ActionPoints<1){S->AdvanceDay();S->InspectFactionArmy(P.Attacker,Before);}
        const auto HumanArmy=S->PlayerArmy;const auto HumanRegion=S->PlayerRegion;const auto HumanMana=S->Hero.Mana;
        const auto HumanGold=S->Economy.Resources.FindRef(TEXT("gold"));const auto HumanAP=S->Economy.ActionPoints;
        FSoulControlledCampaignAction Action;
        if(!TestTrue(TEXT("exact ordered proposal"),S->PrepareControlledAction(P.Attacker,Before.Army.ArmyId,Before.Army.RegionId,P.Target,Action,Error)))continue;
        TestTrue(TEXT("normal hostile execution"),S->ExecuteControlledAction(Action,Error));const auto D=S->PendingBattle;
        TestEqual(TEXT("attacker identity"),D.PlayerFaction,P.Attacker);TestEqual(TEXT("defender identity"),D.EnemyFaction,P.Defender);
        TestEqual(TEXT("attacker roster"),D.PlayerUnitId,Before.Army.UnitId);TestEqual(TEXT("defender roster"),D.EnemyUnitId,Defender.Army.UnitId);
        TestEqual(TEXT("existing canonical pass recipe"),D.BattlefieldId,FName(TEXT("dragon_pass")));
        FSoulFactionCampaignState Paid;S->InspectFactionArmy(P.Attacker,Paid);TestEqual(TEXT("normal hostile AP debit"),Paid.Economy.ActionPoints,Before.Economy.ActionPoints-1);
        FSoulCampaignBattleResult R;R.EncounterId=D.EncounterId;R.TargetRegion=P.Target;R.bPlayerWon=Won;R.PlayerSurvivors=Won?11:0;R.EnemySurvivors=Won?0:14;
        R.PlayerReinforcements=2;R.EnemyReinforcements=3;
        // Arranged validator coverage, not evidence of natural battle outcomes.
        TestTrue(TEXT("side-correct result applies"),S->ApplyBattleResult(R));
        FSoulFactionCampaignState A,B;S->InspectFactionArmy(P.Attacker,A);S->InspectFactionArmy(P.Defender,B);
        TestEqual(TEXT("attacker survivor authority"),A.Army.TroopCount,R.PlayerSurvivors);TestEqual(TEXT("defender survivor authority"),B.Army.TroopCount,R.EnemySurvivors);
        TestEqual(TEXT("attacker return"),A.Army.RegionId,Won?P.Target:D.SourceRegion);TestEqual(TEXT("defender remains at encounter"),B.Army.RegionId,P.Target);
        TestEqual(TEXT("capture actual victorious faction"),S->World.Regions[P.Target].OwnerFactionId,Won?P.Attacker:P.Defender);
        TestTrue(TEXT("uninvolved Human army untouched"),S->PlayerArmy.OrderIndependentCompareEqual(HumanArmy));TestEqual(TEXT("Human region untouched"),S->PlayerRegion,HumanRegion);
        TestEqual(TEXT("Human mana untouched"),S->Hero.Mana,HumanMana);TestEqual(TEXT("Human gold untouched"),S->Economy.Resources.FindRef(TEXT("gold")),HumanGold);TestEqual(TEXT("Human AP untouched"),S->Economy.ActionPoints,HumanAP);
        FRBSaveDomainState Save;TestTrue(TEXT("all faction result capture"),S->CaptureRBSaveDomain_Implementation(Save,Error));FFixture Fresh;
        TestTrue(TEXT("fresh result restore"),Fresh.S->RestoreRBSaveDomain_Implementation(Save,Error));TestEqual(TEXT("exact non-Human result state"),Snapshot(Fresh.S),Snapshot(S));
    }
    int32 Admitted=0;
    for(FName A:FSoulCampaignRules::CanonicalFactions())for(FName B:FSoulCampaignRules::CanonicalFactions())
    {
        const FName AU=FSoulCampaignRules::AdmittedStrategicUnit(A),BU=FSoulCampaignRules::AdmittedStrategicUnit(B);
        const bool Expected=A!=B&&A!=TEXT("dark")&&B!=TEXT("dark")&&(!(A==TEXT("nature")||B==TEXT("nature"))||A==TEXT("humans")||B==TEXT("humans"));
        const bool Actual=FSoulCampaignBattleDescriptor::SupportsExactPair(A,AU,B,BU);
        TestEqual(TEXT("explicit pair surface"),Actual,Expected);if(Actual)++Admitted;
        TestFalse(TEXT("wrong roster never aliases"),FSoulCampaignBattleDescriptor::SupportsExactPair(A,TEXT("wrong_unit"),B,BU));
    }
    TestEqual(TEXT("fourteen ordered exact pairs"),Admitted,14);
    return true;
}

#endif
