#include "Misc/AutomationTest.h"
#include "SoulRealtimeBattleArena.h"
#include "SoulBattleControlRules.h"
#include "SoulRealtimeBattlePBIL.h"
#include "RBMagicSpellDefinition.h"
#include "AIController.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"

#if WITH_DEV_AUTOMATION_TESTS
// Asset-free fixture with the real battle authority, bindings, collision world
// and RB drivers. No campaign save, level travel or production package is opened.
struct FSoulBattleControlTestFixture
{
    UWorld* World = nullptr;
    ASoulRealtimeArenaGameMode* Host = nullptr;
    FSoulBattleControlTestFixture()
    {
        World = UWorld::CreateWorld(EWorldType::Game, false);
        if (!World || !GEngine) return;
        auto& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
        Context.SetCurrentWorld(World);
        auto* Instance = NewObject<UGameInstance>(GEngine);
        Context.OwningGameInstance = Instance;
        World->SetGameInstance(Instance);
        World->GetWorldSettings()->DefaultGameMode = ASoulRealtimeArenaGameMode::StaticClass();
        World->SetGameMode(FURL());
        Host = World->GetAuthGameMode<ASoulRealtimeArenaGameMode>();
        if (!Host) return;
        Host->bVisualUnits = false;
        Host->bBattlePaused = false;
        Host->bAutobattle = false;
        Host->bTacticalCameraActive = false;
        Host->ControlledSide = 0;
        Box(FVector(0,0,-20), FVector(4000,4000,20));
    }
    ~FSoulBattleControlTestFixture()
    {
        if (!World) return;
        if (GEngine) GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
    }
    AActor* Box(FVector Position, FVector HalfSize)
    {
        auto* Actor = World->SpawnActor<AActor>();
        auto* Shape = NewObject<UBoxComponent>(Actor);
        Actor->AddInstanceComponent(Shape);
        Actor->SetRootComponent(Shape);
        Shape->SetBoxExtent(HalfSize);
        Actor->SetActorLocation(Position);
        Shape->SetMobility(EComponentMobility::Static);
        Shape->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Shape->SetCollisionObjectType(ECC_WorldStatic);
        Shape->SetCollisionResponseToAllChannels(ECR_Block);
        Shape->RegisterComponent();
        return Actor;
    }
    int32 Body(int32 Side, FVector Position, bool Hero = false, bool Player = false)
    {
        FSoulRealtimeArenaCombatant Unit;
        Unit.Id = FGuid::NewGuid();
        Unit.Side = Side;
        Unit.Role = Hero ? ESoulRealtimeFormationRole::Hero : ESoulRealtimeFormationRole::Line;
        Unit.Health = Unit.MaxHealth = 100.f;
        Unit.bNonPlayerHero = Hero && !Player;
        Unit.bPlayerHero = Player;
        const int32 I = Host->Combatants.Add(Unit);
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Actor = World->SpawnActor<ACharacter>(Position, FRotator::ZeroRotator, Params);
        Actor->GetCapsuleComponent()->SetCapsuleSize(42.f,96.f);
        Actor->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Actor->GetCapsuleComponent()->SetCollisionObjectType(ECC_Pawn);
        Actor->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
        Host->Actors.Add(Actor);
        auto* Binding = NewObject<USoulRealtimeArenaBinding>(Actor);
        Actor->AddInstanceComponent(Binding);
        Binding->RegisterComponent();
        Binding->BindCombatant(Host->IdentityAt(I).Core());
        Host->Bindings.Add(Binding);
        Host->Ranged.Add(nullptr);
        Host->VisualRunning.Add(false);
        if (Player) Host->PlayerHero = Actor;
        else World->SpawnActor<AAIController>()->Possess(Actor);
        return I;
    }
    int32 Group(int32 Side, const TArray<int32>& Members, FVector Anchor)
    {
        const int32 G = Host->Groups.AddDefaulted();
        FRBCombatGroup Core;
        Core.Id = FGuid::NewGuid();
        Core.Anchor = Anchor;
        Core.Facing = Side == 0 ? FVector::ForwardVector : -FVector::ForwardVector;
        for (int32 I : Members)
        {
            Core.Members.Add(Host->IdentityAt(I).Core());
            Host->Combatants[I].GroupIndex = G;
        }
        Core.Leader = Core.Members[0];
        FRBHostGroup::FromCore(Core, Host->Groups[G]);
        FSoulBattleFormationState State;
        State.GroupIndex = G;
        State.Side = Side;
        State.InitialBodies = State.PreviousAlive = Members.Num();
        State.SpawnAnchor = State.TacticalAnchor = State.ManualAnchor = Anchor;
        State.ManualFacing = Core.Facing;
        State.DisplayName = Side == 0 ? TEXT("Eastern Knights (test fixture)") : TEXT("Enemy troops (test fixture)");
        if (Side == 0) State.ManualOverrideUntil = TNumericLimits<float>::Max();
        Host->TacticalFormations.Add(State);
        return G;
    }
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulBattleControlSelectionTest,
    "Soul.RealtimeBattle.Control.HeroFormationSeparation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulBattleControlSelectionTest::RunTest(const FString&)
{
    FSoulBattleControlTestFixture F;
    auto* H = F.Host;
    if (!TestNotNull(TEXT("Real battle authority"), H)) return false;
    const int32 Hero = F.Body(0,FVector(-350,0,96),true,true);
    const int32 Troop = F.Body(0,FVector(0,0,96));
    const int32 EnemyHero = F.Body(1,FVector(1500,0,96),true);
    const int32 G = F.Group(0,{Hero,Troop},FVector(0,0,96));
    F.Group(1,{EnemyHero},FVector(1500,0,96));
    H->SelectedAlliedFormation = G;
    H->bSelectAllAllies = false;
    TestTrue(TEXT("Legacy same-group predicate reproduces Eastern Knights bug"),
        H->Combatants[Hero].GroupIndex == H->SelectedAlliedFormation);
    TestFalse(TEXT("Selection policy excludes hero even before membership migration"),H->IsUnitSelected(Hero));
    TestTrue(TEXT("Separate commanders transactionally"),H->SeparateBattleHeroesFromFormations());
    TestTrue(TEXT("Attach actual RB drivers"),H->SetupDrivers());
    TestEqual(TEXT("Only troop groups get selection cards"),H->AlliedFormationCount(),1);
    TestEqual(TEXT("Hero excluded from troop count"),H->AliveInGroup(G),1);
    H->HandleBattleAction(FName(*FString::Printf(TEXT("Unit%d"),Troop)));
    TestTrue(TEXT("Formation click selects troop"),H->IsUnitSelected(Troop));
    TestFalse(TEXT("Formation click excludes hero"),H->IsUnitSelected(Hero));
    H->HandleBattleAction(TEXT("Hero"));
    TestTrue(TEXT("Hero action selects only hero"),H->IsUnitSelected(Hero));
    TestFalse(TEXT("Hero selection excludes formations"),H->IsUnitSelected(Troop));
    const int64 Revision = H->Groups[G].Revision;
    H->HandleBattleAction(TEXT("Charge"));
    H->HandleBattleAction(TEXT("Move"));
    TestEqual(TEXT("Hero-only selection cannot issue troop orders"),H->Groups[G].Revision,Revision);
    TestFalse(TEXT("Hero-only selection cannot arm ground movement"),H->bPlaceFormationOrder);
    H->HandleGamepadAction(TEXT("All"));
    TestTrue(TEXT("All troops selects formation"),H->IsUnitSelected(Troop));
    TestFalse(TEXT("All troops never selects hero"),H->IsUnitSelected(Hero));
    TestFalse(TEXT("Enemy hero cannot become friendly selection"),H->IsUnitSelected(EnemyHero));
    H->Combatants[Troop].Role=ESoulRealtimeFormationRole::Hero;
    TestFalse(TEXT("Hero visual archetype alone is not a commander identity"),H->IsBattleHero(Troop));
    FRBCombatGroup DriverGroup;
    TestFalse(TEXT("Direct hero excluded from AI even during possession gap"),
        H->ReadDriveGroup(H->Combatants[Hero].GroupIndex,DriverGroup));
    TestTrue(TEXT("Enemy hero has an independent normal RB group"),
        H->ReadDriveGroup(H->Combatants[EnemyHero].GroupIndex,DriverGroup));
    TestEqual(TEXT("Enemy commander is not a soldier cohort"),DriverGroup.Members.Num(),1);
    TestTrue(TEXT("Enemy commander physically engages"),DriverGroup.Command == ERBGroupCommand::Charge);
    auto* PC = F.World->SpawnActor<APlayerController>();
    PC->Possess(H->PlayerHero);
    H->HandleBattleAction(TEXT("Hold"));
    TestTrue(TEXT("Troop orders preserve hero possession"),PC->GetPawn() == H->PlayerHero);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulBattleControlOrdersTest,
    "Soul.RealtimeBattle.Control.PersistentOrders",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulBattleControlOrdersTest::RunTest(const FString&)
{
    FSoulBattleControlTestFixture F;
    auto* H = F.Host;
    if (!TestNotNull(TEXT("Real battle authority"),H)) return false;
    const int32 Hero = F.Body(0,FVector(-350,0,96),true,true);
    const int32 Troop = F.Body(0,FVector(0,0,96));
    const int32 Enemy = F.Body(1,FVector(160,0,96),true);
    const int32 G = F.Group(0,{Hero,Troop},FVector(0,0,96));
    F.Group(1,{Enemy},FVector(160,0,96));
    if (!TestTrue(TEXT("Separate / install"),H->SeparateBattleHeroesFromFormations() && H->SetupDrivers())) return false;
    const FVector Destination(900,0,96);
    TestTrue(TEXT("MOVE accepted"),H->IssueFormationOrder(G,ERBHostGroupOrder::Advance,Destination,FVector::ForwardVector,true));
    FRBCombatGroup Core;
    H->ReadDriveGroup(G,Core);
    FRBCombatSituation Situation;
    Situation.Person=H->IdentityAt(Troop).Core();
    Situation.Position=H->Actors[Troop]->GetActorLocation();
    Situation.LeaderPosition=Situation.Position;
    Situation.Target=H->IdentityAt(Enemy).Core();
    Situation.TargetPosition=H->Actors[Enemy]->GetActorLocation();
    Situation.bTargetHostile=Situation.bTargetVisible=true;
    Situation.MeleeReach=175;
    TestTrue(TEXT("Actual RB baseline engages before moving to the ordered destination"),
        RBDecideCombat(Core,Situation,135).Intent == ERBCombatIntent::Attack);
    TestFalse(TEXT("Battle adapter prevents off-slot engagement from preempting MOVE"),
        H->CanDriverEngage(G,H->IdentityAt(Troop),H->IdentityAt(Enemy)));
    Situation.bTargetHostile=false;
    const auto Move = RBDecideCombat(Core,Situation,135);
    TestTrue(TEXT("Normal RB movement now wins"),Move.Intent == ERBCombatIntent::Move);
    TestTrue(TEXT("Movement goal is the ordered destination, not the enemy"),Move.Destination.Equals(Destination));
    H->Drivers[G]->TickComponent(.2f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Actual driver does not start an opportunistic melee wind-up"),H->Combatants[Troop].PendingMeleeSeconds,0.f);
    for (int32 Step=0; Step<20; ++Step) { H->BattleElapsed=100.f+Step*40.f; H->TickFormationTactics(); }
    TestTrue(TEXT("MOVE survives later tactical phases"),H->Groups[G].Order == ERBHostGroupOrder::Advance);
    TestTrue(TEXT("MOVE anchor remains unchanged"),H->Groups[G].Anchor.Equals(Destination));
    H->Actors[Troop]->SetActorLocation(Destination);
    H->Actors[Enemy]->SetActorLocation(Destination+FVector(160,0,0));
    TestTrue(TEXT("Troops may defend after arriving at their slot"),H->CanDriverEngage(G,H->IdentityAt(Troop),H->IdentityAt(Enemy)));
    TestTrue(TEXT("HOLD accepted"),H->IssueFormationOrder(G,ERBHostGroupOrder::Hold,Destination,FVector::ForwardVector,true));
    H->BattleElapsed=5000.f;
    H->TickFormationTactics();
    TestTrue(TEXT("HOLD persists rather than returning to AI charge"),H->Groups[G].Order == ERBHostGroupOrder::Hold && H->Groups[G].Anchor.Equals(Destination));
    const int32 S=H->FindFormationState(G);
    H->TacticalFormations[S].bRouting=true;
    H->IssueFormationOrder(G,ERBHostGroupOrder::FallBack,FVector(-800,0,96),FVector::ForwardVector,false);
    H->TacticalFormations[S].bRouting=false;
    H->RefreshManualOrders();
    TestTrue(TEXT("Rally restores the saved HOLD instead of leaving fallback stuck"),H->Groups[G].Order == ERBHostGroupOrder::Hold && H->Groups[G].Anchor.Equals(Destination));
    TestTrue(TEXT("CHARGE accepted"),H->IssueFormationOrder(G,ERBHostGroupOrder::Charge,Destination,FVector::ForwardVector,true));
    H->Actors[Troop]->SetActorLocation(FVector(0,0,96));
    TestTrue(TEXT("CHARGE explicitly permits engagement away from formation slots"),H->CanDriverEngage(G,H->IdentityAt(Troop),H->IdentityAt(Enemy)));
    const FVector FollowAnchor=H->PlayerHero->GetActorLocation()+FVector(0,250,0);
    TestTrue(TEXT("FOLLOW accepted"),H->IssueFormationOrder(G,ERBHostGroupOrder::Follow,FollowAnchor,FVector::ForwardVector,true));
    H->PlayerHero->SetActorLocation(FVector(100,0,96));
    H->RefreshManualOrders();
    TestTrue(TEXT("FOLLOW persists as the user order"),H->Groups[G].Order == ERBHostGroupOrder::Follow);
    H->ReadDriveGroup(G,Core);
    TestTrue(TEXT("FOLLOW uses normal RB anchor movement, not membership in the hero group"),Core.Command == ERBGroupCommand::Advance);
    TestTrue(TEXT("FOLLOW anchor tracks commander displacement"),Core.Anchor.Equals(FVector(100,250,96)));
    TestFalse(TEXT("FOLLOW never inserts the hero into the troop group"),Core.Members.Contains(H->IdentityAt(Hero).Core()));
    H->Combatants[Hero].Health=0;
    H->RefreshManualOrders();
    TestTrue(TEXT("Downed commander changes FOLLOW to HOLD"),H->Groups[G].Order == ERBHostGroupOrder::Hold);
    H->ReturnSelectedAlliesToAI();
    TestTrue(TEXT("AI is restored only by an explicit release"),H->TacticalFormations[S].ManualOverrideUntil <= H->BattleElapsed);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulHeartlandCompanySpawnTest,
    "Soul.RealtimeBattle.Control.ExactCompanyDeployment",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulHeartlandCompanySpawnTest::RunTest(const FString&)
{
    FSoulBattleControlTestFixture F;auto* H=F.Host;
    if(!TestNotNull(TEXT("Battle authority"),H))return false;
    H->bProof=true;H->bCampaignBattle=true;H->PlayerVisualFaction=TEXT("humans");H->PlayerVisualUnitId=TEXT("human_knight");
    H->Spatial=NewObject<USoulRealtimeBattlePBIL>(H);
    if(!TestTrue(TEXT("Real spatial authority configured before spawning"),H->Spatial->Configure(F.World,FVector::ZeroVector)))return false;
    H->ActiveCap=6;H->StrategicBodies[0]=12;
    H->CampaignCompanies[0]={{TEXT("human_knight"),6},{TEXT("human_archer"),4},{TEXT("human_guard"),2}};
    if(!TestTrue(TEXT("Normal exact-company deployment"),H->SpawnArmy(0)))return false;
    TestEqual(TEXT("Active cap preserved"),H->Combatants.Num(),6);
    TestEqual(TEXT("Three independently commanded companies"),H->TacticalFormations.Num(),3);
    for(const auto& Formation:H->TacticalFormations)
    {
        TSet<FName> IDs;
        for(const auto& Unit:H->Combatants)if(Unit.GroupIndex==Formation.GroupIndex)IDs.Add(Unit.CampaignCompany);
        TestEqual(TEXT("Each command group has one exact company"),IDs.Num(),1);
    }
    int32 Archer=INDEX_NONE;
    for(int32 I=0;I<H->Combatants.Num();++I)if(H->Combatants[I].CampaignCompany==TEXT("human_archer"))
    {Archer=I;TestTrue(TEXT("Paid archers use actual ranged combat"),H->Combatants[I].bRanged&&H->Combatants[I].Arrows>0);}
    if(!TestTrue(TEXT("Archers deployed"),Archer!=INDEX_NONE))return false;
    H->Combatants[Archer].Health=0;
    const auto Survivors=H->SurvivingCampaignCompanies(0,false);
    TestEqual(TEXT("One real casualty debits exact company including reserves"),Survivors.FindRef(TEXT("human_archer")),3);
    TestEqual(TEXT("Unharmed infantry and reserves retained"),Survivors.FindRef(TEXT("human_knight")),6);
    TestEqual(TEXT("Unharmed guard retained"),Survivors.FindRef(TEXT("human_guard")),2);
    H->ActiveCap=10;
    TestTrue(TEXT("Existing reserve spawn supports exact company arrivals"),H->SpawnReinforcementWave(0,4,FVector(-2200,0,100)));
    for(const auto& Formation:H->TacticalFormations)
    {
        TSet<FName> IDs;
        for(const auto& Unit:H->Combatants)if(Unit.GroupIndex==Formation.GroupIndex)IDs.Add(Unit.CampaignCompany);
        TestEqual(TEXT("Reserve command group remains company-homogeneous"),IDs.Num(),1);
    }
    TestEqual(TEXT("Deployment never changes survivor totals"),H->SurvivingCampaignCompanies(0,false).FindRef(TEXT("human_archer")),3);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulSiegeGateOrdersTest,
    "Soul.RealtimeBattle.Control.SiegeGateOrders",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulSiegeGateOrdersTest::RunTest(const FString&)
{
    FSoulBattleControlTestFixture F;auto* H=F.Host;if(!H)return false;
    const int32 Hero=F.Body(0,FVector(-800,0,96),true,true);
    const int32 Troop=F.Body(0,FVector(-650,0,96));
    const int32 Enemy=F.Body(1,FVector(800,0,96));
    const int32 G=F.Group(0,{Hero,Troop},FVector(-650,0,96));
    const int32 D=F.Group(1,{Enemy},FVector(800,0,96));
    H->bSiege=true;H->SiegeGateBase=FVector::ZeroVector;H->SiegeForward=FVector::ForwardVector;
    H->SiegeObjective=FVector(950,0,0);H->SiegeState=FSoulSiegeRules::Begin({});
    if(!H->SeparateBattleHeroesFromFormations()||!H->SetupDrivers())return false;
    FRBCombatGroup Drive;
    H->IssueFormationOrder(G,ERBHostGroupOrder::Hold,FVector(-650,0,96),FVector::ForwardVector,true);
    H->ReadDriveGroup(G,Drive);TestTrue(TEXT("siege does not erase HOLD"),Drive.Command==ERBGroupCommand::Hold);
    H->IssueFormationOrder(G,ERBHostGroupOrder::Advance,FVector(-400,0,96),FVector::ForwardVector,true);
    H->ReadDriveGroup(G,Drive);TestTrue(TEXT("valid outside MOVE remains exact"),Drive.Anchor.Equals(FVector(-400,0,96)));
    H->IssueFormationOrder(G,ERBHostGroupOrder::Charge,FVector(950,0,96),FVector::ForwardVector,true);
    const auto UserOrder=H->Groups[G];
    H->ReadDriveGroup(G,Drive);TestTrue(TEXT("closed assault stops before gate"),Drive.Command==ERBGroupCommand::Advance&&Drive.Anchor.X<0);
    TestFalse(TEXT("no attacks through intact portcullis"),H->CanDriverEngage(G,H->IdentityAt(Troop),H->IdentityAt(Enemy)));
    H->SiegeState.GateIntegrityPermille=0;
    H->ReadDriveGroup(G,Drive);TestTrue(TEXT("breached waypoint clears arrival radius and aperture"),Drive.Command==ERBGroupCommand::Advance&&Drive.Anchor.X>=400);
    H->Actors[Troop]->SetActorLocation(FVector(250,0,96));
    H->ReadDriveGroup(G,Drive);TestTrue(TEXT("inside troops regain native pursuit"),Drive.Command==ERBGroupCommand::Charge);
    TestTrue(TEXT("adapter leaves player order and destination intact"),H->Groups[G].Order==UserOrder.Order&&H->Groups[G].Anchor==UserOrder.Anchor);
    H->SiegeState.GateIntegrityPermille=1000;
    H->IssueFormationOrder(D,ERBHostGroupOrder::Charge,FVector(-450,0,96),-FVector::ForwardVector,false);
    H->ReadDriveGroup(D,Drive);TestTrue(TEXT("defenders hold inside closed gate"),Drive.Command==ERBGroupCommand::Hold&&Drive.Anchor.X>0);
    H->IssueFormationOrder(D,ERBHostGroupOrder::Advance,FVector(550,0,96),-FVector::ForwardVector,true);
    H->ReadDriveGroup(D,Drive);TestTrue(TEXT("interior defender MOVE remains available"),Drive.Command==ERBGroupCommand::Advance&&Drive.Anchor.X==550);
    return true;
}

#endif
