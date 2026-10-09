#include "Misc/AutomationTest.h"
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
#endif
