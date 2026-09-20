#include "Misc/AutomationTest.h"
#include "SoulBattlefieldRecipe.h"
#include "SoulCampaign.h"
#include "SoulCombat.h"
#include "SoulHex.h"
#include "SoulMagic.h"
#include "SoulMemory.h"
#include "SoulSiege.h"
#include "SoulStrategyAI.h"
#include "SoulVeterancy.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
    FSoulBattlefield MakeRadiusBoard(int32 Radius)
    {
        FSoulBattlefield Board;
        for (int32 Q = -Radius; Q <= Radius; ++Q)
        {
            for (int32 R = -Radius; R <= Radius; ++R)
            {
                const FSoulHex H(Q, R);
                if (FSoulHexRules::Distance(FSoulHex(0, 0), H) <= Radius)
                {
                    FSoulHexCell Cell;
                    Cell.Coord = H;
                    Board.Cells.Add(H, Cell);
                }
            }
        }
        return Board;
    }

    FSoulRegimentDefinition MakeDef(FName Id, int32 Speed)
    {
        FSoulRegimentDefinition D;
        D.Id = Id;
        D.Speed = Speed;
        D.MinDamage = 2;
        D.MaxDamage = 4;
        D.HitPointsPerUnit = 10;
        return D;
    }

    FSoulRegimentState MakeReg(FName GroupId, FName DefId, FName Side, int32 Count)
    {
        FSoulRegimentState R;
        R.GroupId = GroupId;
        R.DefinitionId = DefId;
        R.SideId = Side;
        R.Count = Count;
        R.CurrentHitPoints = Count * 10;
        return R;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulHexPathTest, "Soul.Core.Hex.Pathfinding", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulHexPathTest::RunTest(const FString&)
{
    FSoulBattlefield Board = MakeRadiusBoard(2);
    Board.Cells[FSoulHex(1, 0)].Terrain = ESoulTerrain::DeepWater;
    TSet<FSoulHex> Occupied;

    const TArray<FSoulHex> Path = FSoulHexRules::ShortestPath(Board, FSoulHex(0, 0), FSoulHex(2, 0), 1, 0, ESoulMovementMode::Ground, Occupied);
    TestTrue(TEXT("detour exists"), Path.Num() >= 4);
    TestFalse(TEXT("path avoids deep water"), Path.Contains(FSoulHex(1, 0)));
    TestEqual(TEXT("hex distance"), FSoulHexRules::Distance(FSoulHex(0, 0), FSoulHex(2, -1)), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulHexFootprintTest, "Soul.Core.Hex.MultiHexFootprint", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulHexFootprintTest::RunTest(const FString&)
{
    FSoulBattlefield Board = MakeRadiusBoard(2);
    TSet<FSoulHex> Occupied;
    Board.Cells[FSoulHex(1, 0)].bBlocksLargeUnits = true;

    TestTrue(TEXT("normal unit can enter"), FSoulHexRules::CanOccupy(Board, FSoulHex(0, 0), 1, 0, ESoulMovementMode::Ground, Occupied));
    TestFalse(TEXT("large unit blocked by narrow terrain"), FSoulHexRules::CanOccupy(Board, FSoulHex(0, 0), 2, 0, ESoulMovementMode::Ground, Occupied));
    TestTrue(TEXT("flying large unit ignores large-unit terrain block"), FSoulHexRules::CanOccupy(Board, FSoulHex(0, 0), 2, 0, ESoulMovementMode::Flying, Occupied));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulInitiativeTest, "Soul.Core.Combat.InitiativeWait", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulInitiativeTest::RunTest(const FString&)
{
    TMap<FName, FSoulRegimentDefinition> Defs;
    Defs.Add(TEXT("fast"), MakeDef(TEXT("fast"), 7));
    Defs.Add(TEXT("mid"), MakeDef(TEXT("mid"), 5));
    Defs.Add(TEXT("slow"), MakeDef(TEXT("slow"), 3));

    TArray<FSoulRegimentState> Units;
    Units.Add(MakeReg(TEXT("A"), TEXT("fast"), TEXT("Player"), 5));
    Units.Add(MakeReg(TEXT("B"), TEXT("slow"), TEXT("Enemy"), 5));
    Units.Add(MakeReg(TEXT("C"), TEXT("mid"), TEXT("Player"), 5));
    FSoulCombatRules::MarkWait(Units[0]);

    const TArray<FName> Order = FSoulCombatRules::InitiativeOrder(Units, Defs);
    TestTrue(TEXT("mid acts before slow"), Order.Num() == 3 && Order[0] == TEXT("C") && Order[1] == TEXT("B"));
    TestTrue(TEXT("waited fast unit goes to wait lane"), Order.Num() == 3 && Order[2] == TEXT("A"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulRetaliationTest, "Soul.Core.Combat.RetaliationOnce", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulRetaliationTest::RunTest(const FString&)
{
    FSoulRegimentState A = MakeReg(TEXT("A"), TEXT("a"), TEXT("P"), 3);
    FSoulRegimentState B = MakeReg(TEXT("B"), TEXT("b"), TEXT("E"), 3);
    A.Anchor = FSoulHex(0, 0);
    B.Anchor = FSoulHex(1, 0);

    TestTrue(TEXT("adjacent defender may retaliate"), FSoulCombatRules::CanRetaliate(B, A));
    B.bRetaliated = true;
    TestFalse(TEXT("second retaliation is denied"), FSoulCombatRules::CanRetaliate(B, A));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulDamageDeterminismTest, "Soul.Core.Combat.DeterministicDamage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulDamageDeterminismTest::RunTest(const FString&)
{
    FSoulRegimentDefinition AttackDef = MakeDef(TEXT("attack"), 5);
    AttackDef.Attack = 8;
    AttackDef.Luck = 2;
    FSoulRegimentDefinition DefenseDef = MakeDef(TEXT("defense"), 5);
    DefenseDef.Defense = 6;

    FSoulRegimentState Attack = MakeReg(TEXT("A"), TEXT("attack"), TEXT("P"), 20);
    FSoulRegimentState D1 = MakeReg(TEXT("D"), TEXT("defense"), TEXT("E"), 20);
    FSoulRegimentState D2 = D1;
    FRandomStream R1(117);
    FRandomStream R2(117);

    const FSoulAttackResult X = FSoulCombatRules::ApplyAttack(Attack, AttackDef, D1, DefenseDef, R1);
    const FSoulAttackResult Y = FSoulCombatRules::ApplyAttack(Attack, AttackDef, D2, DefenseDef, R2);
    TestEqual(TEXT("same seed produces same damage"), X.Damage, Y.Damage);
    TestEqual(TEXT("same seed produces same survivors"), X.DefenderCountAfter, Y.DefenderCountAfter);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulVeterancyTest, "Soul.Core.Regiment.Veterancy", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulVeterancyTest::RunTest(const FString&)
{
    FSoulRegimentState R = MakeReg(TEXT("Veterans"), TEXT("unit"), TEXT("P"), 5);
    FSoulVeterancy::AddExperience(R, 750);
    TestTrue(TEXT("750 XP is elite"), R.Rank == ESoulRegimentRank::Elite);
    TestTrue(TEXT("rank bonus is bounded"), FSoulVeterancy::CombatBonusPermille(R.Rank) <= 100);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulCommanderMemoryTest, "Soul.Core.Memory.CommanderOnlyContext", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulCommanderMemoryTest::RunTest(const FString&)
{
    FSoulCommanderState C;
    C.CommanderId = TEXT("commander_orc");
    FSoulMemoryRules::RecordBattle(C, TEXT("battle_1"), TEXT("knight_commander"), TEXT("ford"), 1, false, 1000);
    FSoulMemoryRules::RecordPlaceLoss(C, TEXT("place_1"), TEXT("knight_commander"), TEXT("ford"), 1, 800);

    TestTrue(TEXT("recent defeat creates caution"), FSoulMemoryRules::RivalBias(C, TEXT("knight_commander"), 2) < 0);
    TestTrue(TEXT("lost place creates reclaim motive"), FSoulMemoryRules::PlaceReclaimBonus(C, TEXT("ford"), 2) > 0);
    TestEqual(TEXT("old battle memory decays"), FSoulMemoryRules::RivalBias(C, TEXT("knight_commander"), 8), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulStrategyAITest, "Soul.Core.AI.MemoryChangesChoice", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulStrategyAITest::RunTest(const FString&)
{
    FSoulStrategySnapshot Base;
    Base.ActingFactionId = TEXT("orcs");
    Base.ActingArmyId = TEXT("army_orcs");
    Base.ActingCommanderId = TEXT("orc_commander");
    Base.Strength = 1000;
    Base.Readiness = 1000;
    Base.Turn = 2;

    FSoulStrategicRegion Resource;
    Resource.Id = TEXT("quarry");
    Resource.OwnerFactionId = TEXT("castle");
    Resource.Value = 400;
    Resource.ResourceValue = 400;
    Resource.Feasibility = 650;
    Resource.TravelCost = 40;
    Resource.bAdjacent = true;
    Base.Regions.Add(Resource);

    FSoulVisibleArmy Enemy;
    Enemy.Id = TEXT("knight_army");
    Enemy.CommanderId = TEXT("knight_commander");
    Enemy.FactionId = TEXT("castle");
    Enemy.Strength = 1000;
    Enemy.bAdjacent = true;
    Base.VisibleEnemyArmies.Add(Enemy);

    FSoulStrategyDecision NoMemory = FSoulStrategyAI::Choose(Base);
    TestTrue(TEXT("without memory army confrontation wins"), NoMemory.bOk && NoMemory.Chosen.Action == ESoulStrategyAction::AttackVisibleArmy);

    FSoulMemoryRules::RecordBattle(Base.Commander, TEXT("defeat"), TEXT("knight_commander"), TEXT("ford"), 2, false, 1000);
    FSoulStrategyDecision Remembered = FSoulStrategyAI::Choose(Base);
    TestTrue(TEXT("remembered defeat changes best strategic choice"), Remembered.bOk && Remembered.Chosen.Action == ESoulStrategyAction::CaptureResourceRegion);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulCampaignTest, "Soul.Core.Campaign.ActionEconomyGrowth", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulCampaignTest::RunTest(const FString&)
{
    FSoulCampaignEconomy C;
    C.Resources.Add(TEXT("gold"), 500);
    C.DailyIncome.Add(TEXT("gold"), 100);

    FSoulRecruitmentPool Pool;
    Pool.UnitId = TEXT("swordsman");
    Pool.Available = 2;
    Pool.WeeklyGrowth = 3;
    Pool.Capacity = 10;
    Pool.CostPerUnit.Add(TEXT("gold"), 100);
    C.RecruitmentPools.Add(Pool.UnitId, Pool);

    TestTrue(TEXT("action can be spent"), FSoulCampaignRules::SpendAction(C));
    TestEqual(TEXT("action point decreased"), C.ActionPoints, 2);
    TestTrue(TEXT("finite pool recruits"), FSoulCampaignRules::Recruit(C, TEXT("swordsman"), 2));
    TestEqual(TEXT("pool exhausted"), C.RecruitmentPools[TEXT("swordsman")].Available, 0);

    for (int32 Day = 0; Day < 7; ++Day) FSoulCampaignRules::AdvanceDay(C);
    TestEqual(TEXT("day eight"), C.Day, 8);
    TestEqual(TEXT("weekly growth replenishes"), C.RecruitmentPools[TEXT("swordsman")].Available, 3);
    TestEqual(TEXT("daily income recurs"), C.Resources[TEXT("gold")], 1000);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulSiegeTest, "Soul.Core.Siege.PreparedLayeredDefense", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulSiegeTest::RunTest(const FString&)
{
    FSoulSiegePreparation Bare;
    TestFalse(TEXT("no pocket ladders"), FSoulSiegeRules::CanAssaultWalls(Bare, false));
    Bare.bBatteringRam = true;
    TestTrue(TEXT("prepared ram enables wall assault route"), FSoulSiegeRules::CanAssaultWalls(Bare, false));

    FSoulSiegePreparation Defended;
    Defended.bReinforcedGate = true;
    Defended.bMagicalWard = true;
    FSoulSiegeState S = FSoulSiegeRules::Begin(Defended);
    TestEqual(TEXT("reinforced gate starts stronger"), S.GateIntegrityPermille, 1300);
    TestTrue(TEXT("ward active"), S.bMagicWardActive);
    FSoulSiegeRules::CaptureObjective(S, ESoulSiegeObjective::MageTower);
    TestFalse(TEXT("taking mage tower drops ward"), S.bMagicWardActive);
    FSoulSiegeRules::CaptureObjective(S, ESoulSiegeObjective::Keep);
    TestTrue(TEXT("keep resolves siege"), S.bVictory && S.Layer == ESoulSiegeLayer::Resolved);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulBattlefieldRecipeTest, "Soul.Core.Battlefield.StrategicContextSelectsMap", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulBattlefieldRecipeTest::RunTest(const FString&)
{
    FSoulBattleContext Context;
    Context.Biome = TEXT("Temperate");
    Context.Landform = TEXT("Valley");
    Context.Feature = TEXT("RiverCrossing");

    FSoulBattlefieldTemplate Ridge;
    Ridge.Id = TEXT("ForestRidge");
    Ridge.Biomes.Add(TEXT("Temperate"));
    Ridge.Landforms.Add(TEXT("Ridge"));

    FSoulBattlefieldTemplate River;
    River.Id = TEXT("RiverFord");
    River.Biomes.Add(TEXT("Temperate"));
    River.Landforms.Add(TEXT("Valley"));
    River.Features.Add(TEXT("RiverCrossing"));

    const FName Selected = FSoulBattlefieldRecipeRules::SelectBest(Context, {Ridge, River});
    TestTrue(TEXT("river crossing on campaign map yields river battlefield"), Selected == TEXT("RiverFord"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulMagicTest, "Soul.Core.Magic.HexArea", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulMagicTest::RunTest(const FString&)
{
    FSoulBattlefield Board = MakeRadiusBoard(2);
    FSoulSpellDefinition Spell;
    Spell.Id = TEXT("fire_burst");
    Spell.CastRange = 5;
    Spell.Shape = ESoulTargetShape::Radius;
    Spell.Radius = 1;

    TestTrue(TEXT("target in cast range"), FSoulMagicRules::IsLegalTarget(Board, FSoulHex(0, 0), FSoulHex(1, 0), Spell));
    TestEqual(TEXT("radius one covers seven hexes"), FSoulMagicRules::AffectedCells(Board, FSoulHex(0, 0), Spell).Num(), 7);
    return true;
}

#endif

#if WITH_DEV_AUTOMATION_TESTS

#include "SoulLogistics.h"
#include "SoulTerrainTactics.h"
#include "SoulWorld.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulLineOfSightTest, "Soul.Core.Hex.LineOfSight", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulLineOfSightTest::RunTest(const FString&)
{
    FSoulBattlefield Board = MakeRadiusBoard(3);
    TestTrue(TEXT("clear line is visible"), FSoulHexRules::HasLineOfSight(Board, FSoulHex(0, 0), FSoulHex(2, 0)));
    Board.Cells[FSoulHex(1, 0)].bBlocksLineOfSight = true;
    TestFalse(TEXT("blocking terrain occludes"), FSoulHexRules::HasLineOfSight(Board, FSoulHex(0, 0), FSoulHex(2, 0)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulTerrainTacticsTest, "Soul.Core.Terrain.TacticalModifiers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulTerrainTacticsTest::RunTest(const FString&)
{
    FSoulBattlefield Board = MakeRadiusBoard(2);
    Board.Cells[FSoulHex(0, 0)].Elevation = 2;
    Board.Cells[FSoulHex(1, 0)].Elevation = 0;
    Board.Cells[FSoulHex(1, 0)].Terrain = ESoulTerrain::Forest;

    FSoulRegimentDefinition Archer = MakeDef(TEXT("archer"), 6);
    Archer.AttackRange = 5;
    FSoulRegimentDefinition Guard = MakeDef(TEXT("guard"), 4);
    Guard.AttackRange = 1;

    FSoulRegimentState A = MakeReg(TEXT("A"), TEXT("archer"), TEXT("P"), 5);
    FSoulRegimentState Df = MakeReg(TEXT("D"), TEXT("guard"), TEXT("E"), 5);
    A.Anchor = FSoulHex(0, 0);
    Df.Anchor = FSoulHex(1, 0);
    Df.Facing = 3;

    const int32 Modifier = FSoulTerrainTactics::AttackModifierPermille(Board, A, Archer, Df, Guard);
    TestTrue(TEXT("terrain modifier remains bounded"), Modifier >= -400 && Modifier <= 400);

    TMap<FName, FSoulRegimentDefinition> Defs;
    Defs.Add(Guard.Id, Guard);
    TArray<FSoulRegimentState> Regiments = {Df};
    TestTrue(TEXT("ground mover entering melee threat enters zone of control"), FSoulTerrainTactics::EntersEnemyZoneOfControl(FSoulHex(0, 0), ESoulMovementMode::Ground, TEXT("P"), Regiments, Defs));
    TestFalse(TEXT("flying mover ignores ground zone of control"), FSoulTerrainTactics::EntersEnemyZoneOfControl(FSoulHex(0, 0), ESoulMovementMode::Flying, TEXT("P"), Regiments, Defs));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulLogisticsTest, "Soul.Core.Campaign.Logistics", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulLogisticsTest::RunTest(const FString&)
{
    FSoulArmyLogisticsState Road;
    FSoulArmyLogisticsState Wild;
    FSoulLogisticsRules::ApplyTravel(Road, 10, true, false);
    FSoulLogisticsRules::ApplyTravel(Wild, 10, false, true);
    TestTrue(TEXT("roads preserve supply"), Road.SupplyPermille > Wild.SupplyPermille);
    TestTrue(TEXT("hostile off-road travel creates more fatigue"), Wild.FatiguePermille > Road.FatiguePermille);
    TestTrue(TEXT("healthy army can force march"), FSoulLogisticsRules::ForceMarch(Road));
    const int32 BeforeRest = Road.ReadinessPermille;
    FSoulLogisticsRules::EndDay(Road, true, true);
    TestTrue(TEXT("friendly settlement restores readiness"), Road.ReadinessPermille > BeforeRest);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulWorldTest, "Soul.Core.World.GeographyDrivesBattle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulWorldTest::RunTest(const FString&)
{
    FSoulWorldState World;
    FSoulRegionState West;
    West.Id = TEXT("west_road");
    West.Neighbors.Add(TEXT("river_ford"));

    FSoulRegionState Ford;
    Ford.Id = TEXT("river_ford");
    Ford.Biome = TEXT("Temperate");
    Ford.Landform = TEXT("Valley");
    Ford.Feature = TEXT("RiverCrossing");
    Ford.Neighbors.Add(TEXT("west_road"));
    Ford.RoadNeighbors.Add(TEXT("west_road"));
    Ford.ApproachFromNeighbor.Add(TEXT("west_road"), TEXT("West"));
    World.Regions.Add(West.Id, West);
    World.Regions.Add(Ford.Id, Ford);

    TestTrue(TEXT("adjacent strategic move legal"), FSoulWorldRules::CanMove(World, TEXT("west_road"), TEXT("river_ford")));
    FSoulWorldRules::RefreshVision(World, TEXT("castle"), TEXT("west_road"));
    TestTrue(TEXT("explored geography persists"), FSoulWorldRules::IsExplored(World, TEXT("castle"), TEXT("river_ford")));

    const FSoulBattleContext Context = FSoulWorldRules::BuildBattleContext(World, TEXT("west_road"), TEXT("river_ford"), TEXT("Rain"), TEXT("Dusk"), false);
    TestTrue(TEXT("battle inherits river context"), Context.Feature == TEXT("RiverCrossing"));
    TestTrue(TEXT("battle inherits approach direction"), Context.AttackerApproach == TEXT("West"));
    TestTrue(TEXT("battle inherits road"), Context.bRoadPresent);
    return true;
}

#endif

#if WITH_DEV_AUTOMATION_TESTS

#include "SoulHero.h"
#include "SoulSettlement.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulSettlementPersistenceTest, "Soul.Core.Settlement.PersistentDamageProjection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulSettlementPersistenceTest::RunTest(const FString&)
{
    FSoulSettlementState Town;
    Town.SettlementId = TEXT("castle_capital");
    Town.FactionId = TEXT("castle");
    Town.RegionId = TEXT("capital_region");

    FSoulBuildingDefinition Hall;
    Hall.Id = TEXT("great_hall");
    Hall.BuildDays = 1;
    Hall.MaxLevel = 2;

    TestTrue(TEXT("construction starts"), FSoulSettlementRules::BeginConstruction(Town, Hall));
    FSoulSettlementRules::AdvanceDay(Town);
    TestTrue(TEXT("building becomes intact"), Town.Buildings[Hall.Id].Condition == ESoulBuildingCondition::Intact);

    TestTrue(TEXT("siege can damage building"), FSoulSettlementRules::DamageBuilding(Town, Hall.Id, 450, TEXT("scar_hall_fire")));
    const FSoulSettlementProjection Damaged = FSoulSettlementRules::Project(Town);
    TestTrue(TEXT("same presentation projection shows damaged building"), Damaged.DamagedBuildings.Contains(Hall.Id));
    TestTrue(TEXT("siege scar persists in projection"), Damaged.Scars.Contains(TEXT("scar_hall_fire")));

    FSoulSettlementRules::DamageBuilding(Town, Hall.Id, 700, TEXT("scar_hall_collapse"));
    const FSoulSettlementProjection Ruined = FSoulSettlementRules::Project(Town);
    TestTrue(TEXT("destroyed building remains as ruined presentation state"), Ruined.RuinedBuildings.Contains(Hall.Id));

    FSoulSettlementRules::RepairBuilding(Town, Hall.Id, 1000);
    const FSoulSettlementProjection Repaired = FSoulSettlementRules::Project(Town);
    TestFalse(TEXT("repair removes active ruined condition"), Repaired.RuinedBuildings.Contains(Hall.Id));
    TestTrue(TEXT("historical scar remains after repair"), Repaired.Scars.Contains(TEXT("scar_hall_collapse")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulSettlementPrerequisiteTest, "Soul.Core.Settlement.Prerequisites", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulSettlementPrerequisiteTest::RunTest(const FString&)
{
    FSoulSettlementState Town;

    FSoulBuildingDefinition Hall;
    Hall.Id = TEXT("hall");

    FSoulBuildingDefinition Mage;
    Mage.Id = TEXT("mage_tower");
    Mage.Prerequisites.Add(TEXT("hall"));

    TestFalse(TEXT("mage tower locked without hall"), FSoulSettlementRules::BeginConstruction(Town, Mage));
    TestTrue(TEXT("hall construction starts"), FSoulSettlementRules::BeginConstruction(Town, Hall));
    FSoulSettlementRules::AdvanceDay(Town);
    TestTrue(TEXT("mage tower unlocks after prerequisite"), FSoulSettlementRules::BeginConstruction(Town, Mage));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulHeroProgressionTest, "Soul.Core.Hero.ProgressionAndMagic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulHeroProgressionTest::RunTest(const FString&)
{
    FSoulHeroState Hero;
    Hero.HeroId = TEXT("hero_test");
    Hero.MaxMana = 100;
    Hero.Mana = 100;

    TestTrue(TEXT("hero levels from experience"), FSoulHeroRules::AddExperience(Hero, 300));
    TestTrue(TEXT("level up grants skill point"), Hero.UnspentSkillPoints > 0);
    TestTrue(TEXT("skill point spends"), FSoulHeroRules::SpendSkillPoint(Hero, TEXT("war_magic")));
    TestTrue(TEXT("spell learns once"), FSoulHeroRules::LearnSpell(Hero, TEXT("fire_burst")));
    TestFalse(TEXT("duplicate spell rejected"), FSoulHeroRules::LearnSpell(Hero, TEXT("fire_burst")));
    TestTrue(TEXT("mana spends"), FSoulHeroRules::SpendMana(Hero, 35));
    TestEqual(TEXT("mana remainder"), Hero.Mana, 65);
    FSoulHeroRules::RestoreMana(Hero, 1000);
    TestEqual(TEXT("mana restores to cap"), Hero.Mana, 100);
    return true;
}

#endif

#if WITH_DEV_AUTOMATION_TESTS

#include "SoulObjectives.h"
#include "SoulRetreat.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulObjectiveTest, "Soul.Core.Objectives.ControlAndSurvival", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulObjectiveTest::RunTest(const FString&)
{
    TArray<FSoulRegimentState> Units;
    FSoulRegimentState Holder = MakeReg(TEXT("holder"), TEXT("unit"), TEXT("P"), 5);
    Holder.Anchor = FSoulHex(0, 0);
    Units.Add(Holder);

    FSoulBattleObjective Hold;
    Hold.Id = TEXT("ridge");
    Hold.Kind = ESoulObjectiveKind::HoldHexes;
    Hold.OwnerSide = TEXT("P");
    Hold.Cells.Add(FSoulHex(0, 0));
    Hold.RequiredRounds = 2;

    FSoulObjectiveRules::EndRound(Hold, Units);
    TestEqual(TEXT("hold progress one"), Hold.Progress, 1);
    FSoulObjectiveRules::EndRound(Hold, Units);
    const FSoulObjectiveEvaluation Done = FSoulObjectiveRules::Evaluate(Hold, Units, 2);
    TestTrue(TEXT("two uncontested rounds completes hold"), Done.bComplete);

    FSoulBattleObjective Survive;
    Survive.Kind = ESoulObjectiveKind::SurviveRounds;
    Survive.OwnerSide = TEXT("P");
    Survive.RequiredRounds = 3;
    TestFalse(TEXT("survival not early"), FSoulObjectiveRules::Evaluate(Survive, Units, 2).bComplete);
    TestTrue(TEXT("survival completes on round"), FSoulObjectiveRules::Evaluate(Survive, Units, 3).bComplete);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulRetreatTest, "Soul.Core.Combat.RetreatConsequences", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulRetreatTest::RunTest(const FString&)
{
    TArray<FSoulRegimentState> Base;
    Base.Add(MakeReg(TEXT("A"), TEXT("unit"), TEXT("P"), 20));
    Base.Add(MakeReg(TEXT("B"), TEXT("unit"), TEXT("P"), 20));

    TArray<FSoulRegimentState> Normal = Base;
    TArray<FSoulRegimentState> Safe = Base;
    FSoulArmyLogisticsState LN;
    FSoulArmyLogisticsState LS;
    FRandomStream RN(117);
    FRandomStream RS(117);

    const FSoulRetreatResult NormalResult = FSoulRetreatRules::Resolve(Normal, TEXT("P"), LN, true, false, RN);
    const FSoulRetreatResult SafeResult = FSoulRetreatRules::Resolve(Safe, TEXT("P"), LS, true, true, RS);
    TestTrue(TEXT("retreat allowed"), NormalResult.bAllowed);
    TestTrue(TEXT("safe-retreat ability reduces expected losses under same seed"), SafeResult.UnitsLost <= NormalResult.UnitsLost);
    TestTrue(TEXT("retreat carries readiness consequence"), LN.ReadinessPermille < 1000);
    return true;
}

#endif

#if WITH_DEV_AUTOMATION_TESTS

#include "SoulFaction.h"
#include "SoulSiegeAftermath.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulFactionContractTest, "Soul.Core.Faction.SevenUnitContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulFactionContractTest::RunTest(const FString&)
{
    FSoulFactionDefinition F;
    F.FactionId = TEXT("castle");
    F.CoreRoster = {
        {TEXT("fighter1"), ESoulUnitRole::Fighter},
        {TEXT("fighter2"), ESoulUnitRole::Fighter},
        {TEXT("fighter3"), ESoulUnitRole::Fighter},
        {TEXT("fighter4"), ESoulUnitRole::Fighter},
        {TEXT("ranged"), ESoulUnitRole::Ranged},
        {TEXT("beast"), ESoulUnitRole::Beast},
        {TEXT("support"), ESoulUnitRole::SupportMagic}
    };
    F.HeroIds = {TEXT("hero_a"), TEXT("hero_b")};

    TestTrue(TEXT("intended seven-unit shape is valid"), FSoulFactionRules::Validate(F).bValid);
    F.CoreRoster[6].Role = ESoulUnitRole::Fighter;
    TestFalse(TEXT("missing support slot is rejected"), FSoulFactionRules::Validate(F).bValid);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulSiegeAftermathTest, "Soul.Core.Siege.AnnexesPersistentSettlementDamage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulSiegeAftermathTest::RunTest(const FString&)
{
    FSoulSettlementState Town;
    Town.WallIntegrityPermille = 1000;

    FSoulBuildingState Hall;
    Hall.Id = TEXT("hall");
    Hall.Level = 1;
    Hall.IntegrityPermille = 1000;
    Hall.Condition = ESoulBuildingCondition::Intact;
    Town.Buildings.Add(Hall.Id, Hall);

    FSoulSiegeAftermath A;
    A.WallDamagePermille = 400;
    A.BuildingDamagePermille.Add(TEXT("hall"), 550);
    A.ScarIds.Add(TEXT("west_wall_breach"));
    FSoulSiegeAftermathRules::Apply(Town, A);

    const FSoulSettlementProjection P = FSoulSettlementRules::Project(Town);
    TestEqual(TEXT("wall damage persists after battle"), P.WallIntegrityPermille, 600);
    TestTrue(TEXT("building damage persists after battle"), P.DamagedBuildings.Contains(TEXT("hall")));
    TestTrue(TEXT("breach scar persists after battle"), P.Scars.Contains(TEXT("west_wall_breach")));
    return true;
}

#endif
