#if WITH_DEV_AUTOMATION_TESTS
#include "RBItemEconomyFixture.h"
#include "Misc/AutomationTest.h"
using namespace RB::Items;
using namespace RB::Items::Test;
constexpr auto Flags=EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBItemsAtomic,"RBItemEconomy.Core.AtomicRejection",Flags)
bool FRBItemsAtomic::RunTest(const FString& Parameters){
    (void)Parameters;Engine E(catalog(),initial());auto Before=E.snapshot();
    auto R=run(E,alice(),Grant{1,"wheat",1,{}});
    TestTrue(TEXT("Client cannot mint"),R.error==RB::Items::Error::Forbidden);
    TestTrue(TEXT("Failure is byte-stable"),Before==E.snapshot());return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBItemsCraft,"RBItemEconomy.Core.CraftingReservation",Flags)
bool FRBItemsCraft::RunTest(const FString& Parameters){
    (void)Parameters;Engine E(catalog(),initial());auto Wheat=give(E,1,"wheat",2);give(E,1,"water");
    auto R=run(E,alice(),StartCraft{"bread",4,1,{1},1});
    if(!TestTrue(TEXT("Job starts"),static_cast<bool>(R)) || R.created.empty())return false;
    TestTrue(TEXT("Inputs reserved not spent"),E.reserved(Wheat)==2 && total(E,"wheat")==2);
    TestTrue(TEXT("Reserved consume rejected"),run(E,alice(),Consume{Wheat,1}).error==RB::Items::Error::Reserved);
    TestTrue(TEXT("Clock advance"),static_cast<bool>(run(E,host(),AdvanceTime{10})));
    TestTrue(TEXT("Job completes"),static_cast<bool>(run(E,alice(),FinishCraft{R.created.front()})));
    TestTrue(TEXT("Output exists"),total(E,"bread")==1);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBItemsTrade,"RBItemEconomy.Core.VendorReplay",Flags)
bool FRBItemsTrade::RunTest(const FString& Parameters){
    (void)Parameters;Engine E(catalog(),initial());auto Bread=give(E,3,"bread");
    auto Cmd=next(E,alice(),Trade{5,Bread,1,1,5,true});
    TestTrue(TEXT("Trade accepted"),static_cast<bool>(E.execute(alice(),Cmd)));auto Before=E.snapshot();
    TestTrue(TEXT("Retry returns receipt"),E.execute(alice(),Cmd).replayed);
    TestTrue(TEXT("Retry is byte-stable"),Before==E.snapshot());return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBItemsSave,"RBItemEconomy.Core.SnapshotIntegrity",Flags)
bool FRBItemsSave::RunTest(const FString& Parameters){
    (void)Parameters;Engine E(catalog(),initial());give(E,1,"wheat",2);auto Bytes=E.snapshot();Engine Copy(catalog());
    TestTrue(TEXT("Restore accepted"),static_cast<bool>(Copy.restore(Bytes)));
    TestTrue(TEXT("Exact round trip"),Bytes==Copy.snapshot());Bytes[0]^=1;auto Before=Copy.snapshot();
    TestFalse(TEXT("Corruption rejected"),static_cast<bool>(Copy.restore(Bytes)));
    TestTrue(TEXT("Corruption does not mutate"),Before==Copy.snapshot());return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBItemsRace,"RBItemEconomy.Core.LootSingleWinner",Flags)
bool FRBItemsRace::RunTest(const FString& Parameters){
    (void)Parameters;Engine E(catalog(),initial());
    auto A=E.prepare(alice(),next(E,alice(),RollLoot{"chest",0,1}));auto B=E.prepare(bob(),next(E,bob(),RollLoot{"chest",0,2}));
    TestTrue(TEXT("Proposals valid"),static_cast<bool>(A.result) && static_cast<bool>(B.result));
    TestTrue(TEXT("First winner"),static_cast<bool>(E.commit(std::move(A.candidate))));
    TestTrue(TEXT("Second candidate stale"),E.commit(std::move(B.candidate)).error==RB::Items::Error::Conflict);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBItemsPremiumCraft,"RBItemEconomy.Core.Premium.CraftFuelTool",Flags)
bool FRBItemsPremiumCraft::RunTest(const FString& Parameters){
    (void)Parameters;Engine E(catalog(),initial());give(E,1,"wheat",2);give(E,1,"water");auto Coal=give(E,1,"coal");auto Hammer=give(E,1,"hammer");
    auto R=run(E,alice(),StartCraft{"powered_bread",4,1,{1},1});
    if(!TestTrue(TEXT("Powered job starts"),static_cast<bool>(R)) || R.created.empty())return false;
    TestEqual(TEXT("Fuel reserved"),static_cast<int64>(E.reserved(Coal)),static_cast<int64>(1));
    TestEqual(TEXT("Tool reserved"),static_cast<int64>(E.reserved(Hammer)),static_cast<int64>(1));
    TestTrue(TEXT("Clock advance"),static_cast<bool>(run(E,host(),AdvanceTime{10})));
    TestTrue(TEXT("Powered craft completes"),static_cast<bool>(run(E,alice(),FinishCraft{R.created.front()})));
    TestEqual(TEXT("Fuel consumed"),static_cast<int64>(total(E,"coal")),static_cast<int64>(0));
    TestEqual(TEXT("Tool wear committed"),E.state().items.at(Hammer).meta.durability,9750);
    TestEqual(TEXT("Bread produced"),static_cast<int64>(total(E,"bread")),static_cast<int64>(1));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBItemsPremiumStorage,"RBItemEconomy.Core.Premium.StorageDecayRepair",Flags)
bool FRBItemsPremiumStorage::RunTest(const FString& Parameters){
    (void)Parameters;auto S=initial();S.containers.at(1).decayRateBasisPoints=5000;Engine E(catalog(),S);auto Bread=give(E,1,"bread");
    TestEqual(TEXT("Fresh shelf life"),E.state().items.at(Bread).meta.remainingShelfLife,static_cast<int64>(100));
    TestTrue(TEXT("Storage time advances"),static_cast<bool>(run(E,host(),AdvanceTime{40})));
    TestEqual(TEXT("Half-speed storage decay"),E.state().items.at(Bread).meta.remainingShelfLife,static_cast<int64>(80));
    Meta M;M.durability=2500;auto Sword=give(E,1,"sword",1,M);give(E,1,"wood",2);
    TestTrue(TEXT("Repair accepted"),static_cast<bool>(run(E,alice(),Repair{Sword,{1},9000})));
    TestEqual(TEXT("Durability repaired"),E.state().items.at(Sword).meta.durability,9000);
    TestEqual(TEXT("Repair inputs consumed"),static_cast<int64>(total(E,"wood")),static_cast<int64>(0));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBItemsPremiumVendor,"RBItemEconomy.Core.Premium.VendorRestock",Flags)
bool FRBItemsPremiumVendor::RunTest(const FString& Parameters){
    (void)Parameters;Engine E(catalog(),initial());give(E,3,"bread",1);
    TestTrue(TEXT("Early restock rejected"),run(E,host(),RestockVendor{5,0}).error==RB::Items::Error::NotReady);
    TestTrue(TEXT("Restock clock advance"),static_cast<bool>(run(E,host(),AdvanceTime{20})));
    TestTrue(TEXT("Restock accepted"),static_cast<bool>(run(E,host(),RestockVendor{5,0})));
    TestEqual(TEXT("Restock approaches target"),static_cast<int64>(total(E,"bread")),static_cast<int64>(3));
    TestTrue(TEXT("Old restock generation rejected"),run(E,host(),RestockVendor{5,0}).error==RB::Items::Error::Stale);return true;
}

#endif
