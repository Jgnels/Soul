#if WITH_DEV_AUTOMATION_TESTS
#include <limits>
#include "Misc/AutomationTest.h"
#include "Engine/DataTable.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Engine/WorldInitializationValues.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "RBWeatherBiomeVolume.h"
#include "RBWeatherControlVolume.h"
#include "RBWeatherDirector.h"
#include "RBWeatherEnvironmentalExposure.h"
#include "RBWeatherPresentationProfile.h"
#include "RBWeatherPlayerComponent.h"
#include "RBWeatherSimulation.h"
#include "RBWeatherSubsystem.h"
#include "RBWeatherTemperatureSourceComponent.h"

namespace RBWeatherTests
{
FRBWeatherEnvironment ClearEnvironment()
{
    FRBWeatherEnvironment E;
    E.CloudCoverage = 0.1;
    E.CloudDensity = 0.15;
    E.SunIntensityScale = 1.0;
    E.SkyLightIntensityScale = 1.0;
    E.FogDensity = 0.005;
    E.VisibilityKm = 100.0;
    E.WindSpeedMps = 2.0;
    E.DryingPerGameHour = 0.1;
    return E;
}

FRBWeatherPresetRow MakePreset(const FRBWeatherEnvironment& Env, double ActiveHours = 2.0, double TransitionHours = 0.25)
{
    FRBWeatherPresetRow P;
    P.ActiveMinGameHours = ActiveHours;
    P.ActiveMaxGameHours = ActiveHours;
    P.TransitionMinGameHours = TransitionHours;
    P.TransitionMaxGameHours = TransitionHours;
    P.Environment = Env;
    return P;
}

struct FFixture
{
    UWorld* World = nullptr;
    UDataTable* Presets = nullptr;
    UDataTable* Transitions = nullptr;
    UDataTable* Climates = nullptr;
    ARBWeatherDirector* Director = nullptr;
    URBWeatherSubsystem* System = nullptr;

    explicit FFixture(bool bCreatePhysicsScene = false)
    {
        UWorld::InitializationValues Values;
        Values.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
            .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).CreatePhysicsScene(bCreatePhysicsScene).SetTransactional(false);
        World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
        if (!World) return;
        System = World->GetSubsystem<URBWeatherSubsystem>();

        Presets = NewObject<UDataTable>(World);
        Presets->RowStruct = FRBWeatherPresetRow::StaticStruct();
        auto Clear = ClearEnvironment();
        Presets->AddRow(TEXT("Clear"), MakePreset(Clear));
        auto Rain = Clear;
        Rain.CloudCoverage = 0.95; Rain.CloudDensity = 0.85; Rain.SunIntensityScale = 0.35;
        Rain.PrecipitationType = ERBWeatherPrecipitation::Rain; Rain.PrecipitationIntensity = 0.8;
        Rain.TemperatureOffsetC = -3.0; Rain.WetnessGainPerGameHour = 0.4; Rain.DryingPerGameHour = 0.0;
        Rain.LensWetness = 0.8; Rain.AudioIntensity = 0.8;
        Presets->AddRow(TEXT("Rain"), MakePreset(Rain));
        auto Snow = Clear;
        Snow.CloudCoverage = 0.9; Snow.PrecipitationType = ERBWeatherPrecipitation::Snow;
        Snow.PrecipitationIntensity = 0.7; Snow.TemperatureOffsetC = -8.0; Snow.SnowGainCmPerGameHour = 1.0;
        Presets->AddRow(TEXT("Snow"), MakePreset(Snow));
        auto Storm = Rain;
        Storm.WindSpeedMps = 14.0; Storm.WindGustMps = 24.0; Storm.LightningRatePerGameHour = 4.0;
        Presets->AddRow(TEXT("Storm"), MakePreset(Storm));

        Transitions = NewObject<UDataTable>(World);
        Transitions->RowStruct = FRBWeatherTransitionRow::StaticStruct();
        auto Edge = [](FName From, FName To, double Weight)
        {
            FRBWeatherTransitionRow R; R.FromWeather=From; R.ToWeather=To; R.Weight=Weight; return R;
        };
        Transitions->AddRow(TEXT("Clear_Rain"), Edge(TEXT("Clear"), TEXT("Rain"), 2.0));
        Transitions->AddRow(TEXT("Clear_Snow"), Edge(TEXT("Clear"), TEXT("Snow"), 1.0));
        Transitions->AddRow(TEXT("Rain_Clear"), Edge(TEXT("Rain"), TEXT("Clear"), 1.0));
        Transitions->AddRow(TEXT("Snow_Clear"), Edge(TEXT("Snow"), TEXT("Clear"), 1.0));
        Transitions->AddRow(TEXT("Storm_Rain"), Edge(TEXT("Storm"), TEXT("Rain"), 1.0));

        Climates = NewObject<UDataTable>(World);
        Climates->RowStruct = FRBWeatherClimateRow::StaticStruct();
        FRBWeatherClimateRow Climate;
        Climates->AddRow(TEXT("Temperate"), Climate);

        Director = World->SpawnActor<ARBWeatherDirector>();
        if (Director)
        {
            Director->WeatherPresetTable = Presets;
            Director->WeatherTransitionTable = Transitions;
            Director->ClimateTable = Climates;
            Director->ClimateProfileRow = TEXT("Temperate");
            Director->InitialWeather = TEXT("Clear");
            Director->StableSeed = 12345;
            if (System) System->RegisterDirector(Director);
        }
    }

    ~FFixture()
    {
        if (World) World->DestroyWorld(false);
    }

    bool Initialize() const
    {
        return Director && Director->AdvanceExternalTime(0.0, TEXT("Spring"), 0, 12.0);
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherDataValidation, "RBWeather.Core.DataValidation", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherDataValidation::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; FString Error; FRBWeatherClimateRow C;
    TestTrue(TEXT("Climate resolved"), F.Director && F.Director->GetClimateProfile(NAME_None,C));
    TestTrue(TEXT("Valid tables"), FRBWeatherSimulation::ValidateData(F.Presets,F.Transitions,C,Error));
    FRBWeatherTransitionRow Bad; Bad.FromWeather=TEXT("Missing"); Bad.ToWeather=TEXT("Clear");
    F.Transitions->AddRow(TEXT("Bad"),Bad);
    TestFalse(TEXT("Unknown transition endpoint rejected"),FRBWeatherSimulation::ValidateData(F.Presets,F.Transitions,C,Error)); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherDeterministic, "RBWeather.Core.DeterministicAdvance", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherDeterministic::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; FRBWeatherClimateRow C; F.Director->GetClimateProfile(NAME_None,C);
    FRBWeatherRuntimeState A,B; FRBWeatherSimulationEvents E; FString Error;
    TestTrue(TEXT("Init A"),FRBWeatherSimulation::Initialize(A,F.Presets,F.Transitions,C,TEXT("Clear"),TEXT("Spring"),5,12,99,E,Error));
    TestTrue(TEXT("Init B"),FRBWeatherSimulation::Initialize(B,F.Presets,F.Transitions,C,TEXT("Clear"),TEXT("Spring"),5,12,99,E,Error));
    TestTrue(TEXT("Advance A"),FRBWeatherSimulation::Advance(A,F.Presets,F.Transitions,C,TEXT("Spring"),5,15,20,99,E,Error));
    TestTrue(TEXT("Advance B"),FRBWeatherSimulation::Advance(B,F.Presets,F.Transitions,C,TEXT("Spring"),5,15,20,99,E,Error));
    TestEqual(TEXT("Current deterministic"),A.CurrentWeather,B.CurrentWeather); TestEqual(TEXT("Target deterministic"),A.TargetWeather,B.TargetWeather);
    TestEqual(TEXT("Serial deterministic"),A.TransitionSerial,B.TransitionSerial); TestEqual(TEXT("Wetness deterministic"),A.Wetness,B.Wetness); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherForce, "RBWeather.Core.ForceAndBlend", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherForce::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; FRBWeatherClimateRow C; F.Director->GetClimateProfile(NAME_None,C);
    FRBWeatherRuntimeState S; FRBWeatherSimulationEvents E; FString Error; FRBWeatherEnvironment V; double Alpha=0;
    FRBWeatherSimulation::Initialize(S,F.Presets,F.Transitions,C,TEXT("Clear"),TEXT("Spring"),0,12,11,E,Error);
    TestTrue(TEXT("Force storm"),FRBWeatherSimulation::ForceWeather(S,F.Presets,F.Transitions,C,TEXT("Storm"),2,11,E,Error));
    FRBWeatherSimulation::Advance(S,F.Presets,F.Transitions,C,TEXT("Spring"),0,13,1,11,E,Error);
    TestTrue(TEXT("Visual query"),FRBWeatherSimulation::GetVisualEnvironment(S,F.Presets,F.Transitions,C,V,Alpha,Error));
    TestTrue(TEXT("Halfway alpha"),FMath::IsNearlyEqual(Alpha,0.5,1e-6)); TestTrue(TEXT("Clouds blended"),V.CloudCoverage>0.1 && V.CloudCoverage<0.95); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherTemperature, "RBWeather.Core.ThreeLayerTemperature", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherTemperature::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; FRBWeatherClimateRow C; F.Director->GetClimateProfile(NAME_None,C); bool Valid=false;
    const double Base=FRBWeatherSimulation::ComputeBaseTemperatureC(C,TEXT("Winter"),7,15,55,Valid); TestTrue(TEXT("Base temp valid"),Valid);
    TestTrue(TEXT("Season bounds"),Base>=-15.0 && Base<=7.0);
    const double Local=FRBWeatherSimulation::ApplyLocalTemperatureControl(-30,20,10,5,35,Valid); TestTrue(TEXT("Local valid"),Valid); TestEqual(TEXT("Interior clamps"),Local,5.0); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherSurfaceMemory, "RBWeather.Core.SurfaceMemory", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherSurfaceMemory::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; FRBWeatherClimateRow C; F.Director->GetClimateProfile(NAME_None,C); FRBWeatherRuntimeState S; FRBWeatherSimulationEvents E; FString Error;
    FRBWeatherSimulation::Initialize(S,F.Presets,F.Transitions,C,TEXT("Rain"),TEXT("Spring"),0,12,88,E,Error);
    TestTrue(TEXT("Rain advances"),FRBWeatherSimulation::Advance(S,F.Presets,F.Transitions,C,TEXT("Spring"),0,13,1,88,E,Error));
    TestTrue(TEXT("Wetness accumulates"),S.Wetness>0.0); const double Wet=S.Wetness;
    FRBWeatherSimulation::ForceWeather(S,F.Presets,F.Transitions,C,TEXT("Clear"),0,88,E,Error);
    FRBWeatherSimulation::Advance(S,F.Presets,F.Transitions,C,TEXT("Spring"),0,14,1,88,E,Error); TestTrue(TEXT("Wetness dries"),S.Wetness<Wet); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherDirectorInit, "RBWeather.World.DirectorInitialization", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherDirectorInit::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; TestTrue(TEXT("Initialize director"),F.Initialize()); const auto S=F.Director->GetGlobalState();
    TestTrue(TEXT("Global initialized"),S.bInitialized); TestEqual(TEXT("Season retained"),S.CurrentSeason,FName(TEXT("Spring"))); FRBWeatherSnapshotAtLocation Q;
    TestTrue(TEXT("Global snapshot"),F.Director->GetGlobalSnapshot(Q)); TestEqual(TEXT("Global biome id"),Q.BiomeId,FName(TEXT("Global"))); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherBiomeBlend, "RBWeather.World.BiomeSpatialBlend", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherBiomeBlend::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; TestTrue(TEXT("Initialize"),F.Initialize());
    auto* B=F.World->SpawnActor<ARBWeatherBiomeVolume>(); if(!TestNotNull(TEXT("Biome"),B)) return false; B->BiomeId=TEXT("Highlands"); B->InitialWeather=TEXT("Rain"); B->TransitionWidthCm=1000;
    F.System->RegisterBiome(B); FString Error; TestTrue(TEXT("Biome init"),B->AdvanceFromDirector(F.Director,TEXT("Spring"),0,12,0,Error));
    FRBWeatherSnapshotAtLocation Center,Edge; TestTrue(TEXT("Center query"),F.System->GetWeatherAtLocation(FVector::ZeroVector,Center));
    TestEqual(TEXT("Center uses biome"),Center.BiomeId,FName(TEXT("Highlands"))); TestTrue(TEXT("Center rainy"),Center.Environment.PrecipitationIntensity>0.5);
    const FVector NearEdge(B->Bounds->GetUnscaledBoxExtent().X-500.0,0,0); TestTrue(TEXT("Edge query"),F.System->GetWeatherAtLocation(NearEdge,Edge));
    TestTrue(TEXT("Edge is blended"),Edge.Environment.PrecipitationIntensity>0.0 && Edge.Environment.PrecipitationIntensity<Center.Environment.PrecipitationIntensity); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherBiomePriority, "RBWeather.World.BiomePriority", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherBiomePriority::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; F.Initialize(); FString Error;
    auto* A=F.World->SpawnActor<ARBWeatherBiomeVolume>(); auto* B=F.World->SpawnActor<ARBWeatherBiomeVolume>();
    A->BiomeId=TEXT("A"); A->InitialWeather=TEXT("Rain"); A->Priority=1; B->BiomeId=TEXT("B"); B->InitialWeather=TEXT("Snow"); B->Priority=2;
    F.System->RegisterBiome(A);F.System->RegisterBiome(B);A->AdvanceFromDirector(F.Director,TEXT("Winter"),0,12,0,Error);B->AdvanceFromDirector(F.Director,TEXT("Winter"),0,12,0,Error);
    FRBWeatherSnapshotAtLocation Q; TestTrue(TEXT("Query"),F.System->GetWeatherAtLocation(FVector::ZeroVector,Q)); TestEqual(TEXT("Higher priority wins"),Q.BiomeId,FName(TEXT("B"))); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherControlArea, "RBWeather.World.ControlArea", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherControlArea::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; F.Initialize(); F.Director->ForceGlobalWeather(TEXT("Rain"),0);
    auto* A=F.World->SpawnActor<ARBWeatherControlVolume>(); if(!TestNotNull(TEXT("Area"),A))return false; A->ControlAreaId=TEXT("House");A->TransitionWidthCm=0;A->TargetTemperatureC=20;A->TemperatureAdjustmentLimitC=10;
    F.System->RegisterControlArea(A); FRBWeatherSnapshotAtLocation Q; TestTrue(TEXT("Query"),F.System->GetWeatherAtLocation(FVector::ZeroVector,Q));
    TestEqual(TEXT("Rain hidden indoors"),Q.PrecipitationVisualMultiplier,0.0); TestEqual(TEXT("Lens hidden indoors"),Q.LensEffectMultiplier,0.0); TestTrue(TEXT("Local temp finite"),FMath::IsFinite(Q.LocalTemperatureC)); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherTemperatureSource, "RBWeather.World.TemperatureSource", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherTemperatureSource::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; F.Initialize(); AActor* Host=F.World->SpawnActor<AActor>(); auto* Source=NewObject<URBWeatherTemperatureSourceComponent>(Host);
    Source->RadiusCm=1000;Source->TemperatureOffsetC=12;Source->FalloffExponent=1;F.System->RegisterTemperatureSource(Source);
    FRBWeatherSnapshotAtLocation Q; TestTrue(TEXT("Query"),F.System->GetWeatherAtLocation(FVector::ZeroVector,Q));
    TestTrue(TEXT("Heat source raises local temp"),Q.LocalTemperatureC>Q.OutsideTemperatureC); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherForceBiome, "RBWeather.World.ForceBiomeWeather", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherForceBiome::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; F.Initialize(); auto* B=F.World->SpawnActor<ARBWeatherBiomeVolume>(); B->BiomeId=TEXT("Valley");B->InitialWeather=TEXT("Clear");F.System->RegisterBiome(B); FString Error;
    B->AdvanceFromDirector(F.Director,TEXT("Spring"),0,12,0,Error); TestTrue(TEXT("Force through director"),F.Director->ForceBiomeWeather(TEXT("Valley"),TEXT("Rain"),0)); TestEqual(TEXT("Biome changed"),B->GetWeatherState().CurrentWeather,FName(TEXT("Rain"))); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherSaveRoundTrip, "RBWeather.Save.SerializationRoundTrip", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherSaveRoundTrip::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; F.Initialize(); auto* B=F.World->SpawnActor<ARBWeatherBiomeVolume>();B->BiomeId=TEXT("Valley");B->InitialWeather=TEXT("Rain");F.System->RegisterBiome(B);F.Director->AdvanceExternalTime(1,TEXT("Spring"),0,13);
    FRBWeatherSaveSnapshot Save=F.Director->MakeSnapshot();TArray<uint8> Bytes;
    {FMemoryWriter Raw(Bytes,true);FObjectAndNameAsStringProxyArchive Ar(Raw,false);Ar.ArIsSaveGame=true;FRBWeatherSaveSnapshot::StaticStruct()->SerializeItem(Ar,&Save,nullptr);}
    FRBWeatherSaveSnapshot Loaded;{FMemoryReader Raw(Bytes,true);FObjectAndNameAsStringProxyArchive Ar(Raw,true);Ar.ArIsSaveGame=true;FRBWeatherSaveSnapshot::StaticStruct()->SerializeItem(Ar,&Loaded,nullptr);}
    TestTrue(TEXT("Restore"),F.Director->RestoreSnapshot(Loaded));TestEqual(TEXT("Global current survives"),F.Director->GetGlobalState().CurrentWeather,Loaded.GlobalWeather.CurrentWeather); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherRestoreAtomic, "RBWeather.Save.InvalidRestoreIsAtomic", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherRestoreAtomic::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; F.Initialize();const auto Before=F.Director->GetGlobalState();auto Save=F.Director->MakeSnapshot();Save.GlobalWeather.CurrentWeather=TEXT("Unknown");
    TestFalse(TEXT("Bad snapshot rejected"),F.Director->RestoreSnapshot(Save));TestEqual(TEXT("Global retained"),F.Director->GetGlobalState().CurrentWeather,Before.CurrentWeather); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherDuplicateBiome, "RBWeather.Validation.DuplicateBiomeId", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherDuplicateBiome::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F;auto* A=F.World->SpawnActor<ARBWeatherBiomeVolume>();auto* B=F.World->SpawnActor<ARBWeatherBiomeVolume>();A->BiomeId=TEXT("Same");B->BiomeId=TEXT("Same");F.System->RegisterBiome(A); AddExpectedError(TEXT("Duplicate active BiomeId 'Same' rejected."), EAutomationExpectedErrorFlags::Contains, 1); F.System->RegisterBiome(B); FString Error;
    TestEqual(TEXT("Duplicate actor rejected at registration"),F.System->GetBiomes().Num(),1); TestEqual(TEXT("One canonical biome retained"),F.System->GetCanonicalBiomes().Num(),1); TestTrue(TEXT("Remaining configuration is valid"),F.Director->ValidateConfiguredData(Error)); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherBadSchema, "RBWeather.Save.UnknownSchemaRejected", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherBadSchema::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F;F.Initialize();auto Save=F.Director->MakeSnapshot();Save.SchemaVersion=999;TestFalse(TEXT("Unknown schema rejected"),F.Director->RestoreSnapshot(Save)); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherNoPerSystemTick, "RBWeather.Architecture.NoSimulationActorTick", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherNoPerSystemTick::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; auto* B=F.World->SpawnActor<ARBWeatherBiomeVolume>(); auto* C=F.World->SpawnActor<ARBWeatherControlVolume>();
    AActor* Host=F.World->SpawnActor<AActor>(); auto* T=NewObject<URBWeatherTemperatureSourceComponent>(Host);
    TestFalse(TEXT("Director no tick"),F.Director->PrimaryActorTick.bCanEverTick);TestFalse(TEXT("Biome no tick"),B->PrimaryActorTick.bCanEverTick);
    TestFalse(TEXT("Control no tick"),C->PrimaryActorTick.bCanEverTick);TestFalse(TEXT("Temperature source no tick"),T->PrimaryComponentTick.bCanEverTick);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherFirstDelta, "RBWeather.Hardening.FirstAdvanceConsumesDelta", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherFirstDelta::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; FRBWeatherClimateRow C; F.Director->GetClimateProfile(NAME_None,C);
    FRBWeatherRuntimeState Expected; FRBWeatherSimulationEvents E; FString Error;
    TestTrue(TEXT("Expected init"),FRBWeatherSimulation::Initialize(Expected,F.Presets,F.Transitions,C,TEXT("Clear"),TEXT("Spring"),0,13,12345,E,Error));
    TestTrue(TEXT("Expected advance"),FRBWeatherSimulation::Advance(Expected,F.Presets,F.Transitions,C,TEXT("Spring"),0,13,1,12345,E,Error));
    TestTrue(TEXT("First nonzero advance"),F.Director->AdvanceExternalTime(1,TEXT("Spring"),0,13));
    const auto Actual=F.Director->GetGlobalState();
    TestEqual(TEXT("First delta active time"),Actual.ActiveRemainingGameHours,Expected.ActiveRemainingGameHours);
    TestEqual(TEXT("First delta transition serial"),Actual.TransitionSerial,Expected.TransitionSerial); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherTransactionalAdvance, "RBWeather.Hardening.TransactionalAdvance", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherTransactionalAdvance::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; TestTrue(TEXT("Initialize"),F.Initialize()); const auto Before=F.Director->GetGlobalState();
    auto* B=F.World->SpawnActor<ARBWeatherBiomeVolume>(); B->BiomeId=TEXT("Broken"); B->ClimateProfileOverride=TEXT("MissingClimate"); F.System->RegisterBiome(B);
    AddExpectedError(TEXT("Biome 'Broken' climate profile could not be resolved."), EAutomationExpectedErrorFlags::Contains, 1);
    TestFalse(TEXT("Invalid biome freezes whole transaction"),F.Director->AdvanceExternalTime(1,TEXT("Spring"),0,13));
    const auto After=F.Director->GetGlobalState();
    TestEqual(TEXT("Global serial unchanged"),After.TransitionSerial,Before.TransitionSerial);
    TestEqual(TEXT("Global active time unchanged"),After.ActiveRemainingGameHours,Before.ActiveRemainingGameHours); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherRegistryPersistence, "RBWeather.Hardening.StateRegistrySurvivesUnregister", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherRegistryPersistence::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; TestTrue(TEXT("Initialize"),F.Initialize()); FString Error;
    auto* B=F.World->SpawnActor<ARBWeatherBiomeVolume>(); B->BiomeId=TEXT("Streamed"); B->InitialWeather=TEXT("Rain"); F.System->RegisterBiome(B);
    TestTrue(TEXT("Biome init"),B->AdvanceFromDirector(F.Director,TEXT("Spring"),0,12,0,Error));
    const auto* R0=F.System->FindCanonicalBiome(TEXT("Streamed")); if(!TestNotNull(TEXT("Canonical before unload"),R0)) return false;
    const double BeforeWet=R0->Weather.Wetness; F.System->UnregisterBiome(B);
    TestTrue(TEXT("Advance while actor absent"),F.Director->AdvanceExternalTime(1,TEXT("Spring"),0,13));
    const auto* R1=F.System->FindCanonicalBiome(TEXT("Streamed")); if(!TestNotNull(TEXT("Canonical after unload"),R1)) return false;
    TestTrue(TEXT("Canonical advanced while absent"),R1->Weather.Wetness>BeforeWet);
    auto* B2=F.World->SpawnActor<ARBWeatherBiomeVolume>(); B2->BiomeId=TEXT("Streamed"); B2->InitialWeather=TEXT("Rain"); F.System->RegisterBiome(B2);
    TestEqual(TEXT("Re-entry restores current weather"),B2->GetWeatherState().CurrentWeather,R1->Weather.CurrentWeather);
    TestEqual(TEXT("Re-entry restores wetness"),B2->GetWeatherState().Wetness,R1->Weather.Wetness); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherBiomeTie, "RBWeather.Hardening.BiomeTieDeterministic", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherBiomeTie::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; F.Initialize(); FString Error;
    auto* A=F.World->SpawnActor<ARBWeatherBiomeVolume>(); auto* B=F.World->SpawnActor<ARBWeatherBiomeVolume>();
    A->BiomeId=TEXT("A"); A->InitialWeather=TEXT("Rain"); A->Priority=5; A->TransitionWidthCm=3000; A->SetActorLocation(FVector(0,0,0.1));
    B->BiomeId=TEXT("B"); B->InitialWeather=TEXT("Snow"); B->Priority=5; B->TransitionWidthCm=3000;
    F.System->RegisterBiome(A); F.System->RegisterBiome(B); A->AdvanceFromDirector(F.Director,TEXT("Winter"),0,12,0,Error); B->AdvanceFromDirector(F.Director,TEXT("Winter"),0,12,0,Error);
    const double WA=A->GetLocationWeight(FVector::ZeroVector), WB=B->GetLocationWeight(FVector::ZeroVector);
    TestTrue(TEXT("Weights exercise epsilon tie"),WB>WA && WB-WA<=KINDA_SMALL_NUMBER);
    FRBWeatherSnapshotAtLocation Q; TestTrue(TEXT("Query"),F.System->GetWeatherAtLocation(FVector::ZeroVector,Q));
    TestEqual(TEXT("Lexical tie is deterministic"),Q.BiomeId,FName(TEXT("A"))); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherSchema2Reconstruct, "RBWeather.Hardening.Schema2ReconstructsUnloadedBiome", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherSchema2Reconstruct::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; F.Initialize(); FString Error;
    auto* B=F.World->SpawnActor<ARBWeatherBiomeVolume>(); B->BiomeId=TEXT("Valley"); B->InitialWeather=TEXT("Rain"); F.System->RegisterBiome(B);
    B->AdvanceFromDirector(F.Director,TEXT("Spring"),0,12,0,Error); F.Director->AdvanceExternalTime(1,TEXT("Spring"),0,13);
    const auto Save=F.Director->MakeSnapshot(); TestEqual(TEXT("Schema 2"),Save.SchemaVersion,2); TestEqual(TEXT("One biome saved"),Save.Biomes.Num(),1);
    const auto SavedWeather=Save.Biomes[0].Weather; F.System->UnregisterBiome(B); F.System->GetCanonicalBiomes().Reset();
    TestTrue(TEXT("Restore reconstructs unloaded definition"),F.Director->RestoreSnapshot(Save));
    const auto* R=F.System->FindCanonicalBiome(TEXT("Valley")); if(!TestNotNull(TEXT("Reconstructed canonical"),R)) return false;
    TestEqual(TEXT("Reconstructed wetness"),R->Weather.Wetness,SavedWeather.Wetness);
    auto* B2=F.World->SpawnActor<ARBWeatherBiomeVolume>(); B2->BiomeId=TEXT("Valley"); B2->InitialWeather=TEXT("Rain"); F.System->RegisterBiome(B2);
    TestEqual(TEXT("Late actor receives restored weather"),B2->GetWeatherState().CurrentWeather,SavedWeather.CurrentWeather); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherPartialSnapshot, "RBWeather.Hardening.PartialSnapshotRejectedAtomic", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherPartialSnapshot::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; F.Initialize();
    auto* A=F.World->SpawnActor<ARBWeatherBiomeVolume>(); auto* B=F.World->SpawnActor<ARBWeatherBiomeVolume>(); A->BiomeId=TEXT("A"); B->BiomeId=TEXT("B");
    F.System->RegisterBiome(A); F.System->RegisterBiome(B); TestTrue(TEXT("Initialize biomes"),F.Director->AdvanceExternalTime(0,TEXT("Spring"),0,12));
    auto Save=F.Director->MakeSnapshot(); TestEqual(TEXT("Two biomes saved"),Save.Biomes.Num(),2); Save.Biomes.RemoveAt(0);
    const auto Before=F.Director->GetGlobalState(); TestFalse(TEXT("Partial snapshot rejected"),F.Director->RestoreSnapshot(Save)); const auto After=F.Director->GetGlobalState();
    TestEqual(TEXT("Atomic serial"),After.TransitionSerial,Before.TransitionSerial); TestEqual(TEXT("Atomic active time"),After.ActiveRemainingGameHours,Before.ActiveRemainingGameHours); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherSpatialAuthority, "RBWeather.Hardening.SpatialGameplayAuthority", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherSpatialAuthority::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; TestTrue(TEXT("Initialize"),F.Initialize()); FString Error;
    auto* B=F.World->SpawnActor<ARBWeatherBiomeVolume>(); if(!TestNotNull(TEXT("Biome"),B)) return false;
    B->BiomeId=TEXT("RainZone"); B->InitialWeather=TEXT("Rain"); B->TransitionWidthCm=1000.0;
    F.System->RegisterBiome(B); TestTrue(TEXT("Biome init"),B->AdvanceFromDirector(F.Director,TEXT("Spring"),0,12,0,Error));
    TestTrue(TEXT("Advance rainfall"),F.Director->AdvanceExternalTime(1,TEXT("Spring"),0,13));
    FRBWeatherSnapshotAtLocation Center, Edge;
    TestTrue(TEXT("Center query"),F.System->GetWeatherAtLocation(FVector::ZeroVector,Center));
    const FVector NearEdge(B->Bounds->GetUnscaledBoxExtent().X-250.0,0,0);
    TestTrue(TEXT("Edge query"),F.System->GetWeatherAtLocation(NearEdge,Edge));
    TestEqual(TEXT("Center gameplay biome"),Center.BiomeId,FName(TEXT("RainZone")));
    TestEqual(TEXT("Center gameplay precip"),Center.GameplayEnvironment.PrecipitationType,ERBWeatherPrecipitation::Rain);
    TestEqual(TEXT("Edge gameplay remains global"),Edge.BiomeId,FName(TEXT("Global")));
    TestEqual(TEXT("Edge gameplay precip remains dry"),Edge.GameplayEnvironment.PrecipitationType,ERBWeatherPrecipitation::None);
    TestTrue(TEXT("Edge presentation still blends rain"),Edge.Environment.PrecipitationIntensity>0.0);
    TestTrue(TEXT("Center authoritative wetness"),Center.Wetness>0.0);
    TestEqual(TEXT("Edge authoritative wetness global"),Edge.Wetness,0.0);
    TestTrue(TEXT("Edge visual wetness blends"),Edge.VisualWetness>0.0 && Edge.VisualWetness<Center.VisualWetness);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherMidTransitionSave, "RBWeather.Hardening.MidTransitionSaveContinuation", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherMidTransitionSave::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; TestTrue(TEXT("Initialize"),F.Initialize());
    TestTrue(TEXT("Force rain transition"),F.Director->ForceGlobalWeather(TEXT("Rain"),2.0));
    TestTrue(TEXT("Advance into transition"),F.Director->AdvanceExternalTime(0.75,TEXT("Spring"),0,12.75));
    const auto Save=F.Director->MakeSnapshot();
    TestTrue(TEXT("Transition is in progress"),Save.GlobalWeather.TransitionDurationGameHours>0.0 && Save.GlobalWeather.TransitionElapsedGameHours>0.0);
    TestTrue(TEXT("Advance reference continuation"),F.Director->AdvanceExternalTime(3.0,TEXT("Spring"),0,15.75));
    const auto Reference=F.Director->GetGlobalState();
    TestTrue(TEXT("Restore midpoint"),F.Director->RestoreSnapshot(Save));
    TestTrue(TEXT("Advance restored continuation"),F.Director->AdvanceExternalTime(3.0,TEXT("Spring"),0,15.75));
    const auto Actual=F.Director->GetGlobalState();
    TestEqual(TEXT("Current continues identically"),Actual.CurrentWeather,Reference.CurrentWeather);
    TestEqual(TEXT("Target continues identically"),Actual.TargetWeather,Reference.TargetWeather);
    TestEqual(TEXT("Transition serial continues"),Actual.TransitionSerial,Reference.TransitionSerial);
    TestEqual(TEXT("Lightning serial continues"),Actual.LightningSerial,Reference.LightningSerial);
    TestTrue(TEXT("Wetness continues"),FMath::IsNearlyEqual(Actual.Wetness,Reference.Wetness,1e-9));
    TestTrue(TEXT("Active time continues"),FMath::IsNearlyEqual(Actual.ActiveRemainingGameHours,Reference.ActiveRemainingGameHours,1e-9));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherSaveCorruption, "RBWeather.Hardening.SaveCorruptionRejectedAtomic", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherSaveCorruption::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; TestTrue(TEXT("Initialize"),F.Initialize());
    auto* B=F.World->SpawnActor<ARBWeatherBiomeVolume>(); B->BiomeId=TEXT("Valley"); F.System->RegisterBiome(B);
    TestTrue(TEXT("Initialize biome"),F.Director->AdvanceExternalTime(0,TEXT("Spring"),0,12));
    const auto Before=F.Director->GetGlobalState();
    auto BadNumeric=F.Director->MakeSnapshot(); BadNumeric.GlobalWeather.Wetness=std::numeric_limits<double>::quiet_NaN();
    TestFalse(TEXT("NaN snapshot rejected"),F.Director->RestoreSnapshot(BadNumeric));
    TestEqual(TEXT("NaN rejection atomic"),F.Director->GetGlobalState().TransitionSerial,Before.TransitionSerial);
    auto Duplicate=F.Director->MakeSnapshot(); const FRBWeatherBiomeSaveState DuplicateEntry = Duplicate.Biomes[0]; Duplicate.Biomes.Add(DuplicateEntry);
    TestFalse(TEXT("Duplicate biome save rejected"),F.Director->RestoreSnapshot(Duplicate));
    TestEqual(TEXT("Duplicate rejection atomic"),F.Director->GetGlobalState().TransitionSerial,Before.TransitionSerial);
    auto BadCondition=F.Director->MakeSnapshot(); BadCondition.Biomes[0].Weather.CurrentWeather=TEXT("MissingWeather");
    TestFalse(TEXT("Invalid biome condition rejected"),F.Director->RestoreSnapshot(BadCondition));
    TestEqual(TEXT("Condition rejection atomic"),F.Director->GetGlobalState().TransitionSerial,Before.TransitionSerial);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherShelterTrace, "RBWeather.Hardening.BoundedShelterTrace", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherShelterTrace::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F(true); TestTrue(TEXT("Initialize"),F.Initialize());
    AActor* Player=F.World->SpawnActor<AActor>(); if(!TestNotNull(TEXT("Player"),Player)) return false;
    auto* PlayerRoot=NewObject<UBoxComponent>(Player); Player->SetRootComponent(PlayerRoot); PlayerRoot->SetCollisionEnabled(ECollisionEnabled::NoCollision); PlayerRoot->RegisterComponent(); auto* Weather=NewObject<URBWeatherPlayerComponent>(Player); Player->AddInstanceComponent(Weather); Weather->RegisterComponent();
    Weather->bEnableAutomaticShelter=true; Weather->ShelterTraceHeightCm=2000.0f; Weather->ShelterTraceChannel=ECC_Visibility;
    TestTrue(TEXT("Outdoor shelter query"),Weather->RefreshShelterNow()); TestTrue(TEXT("Outdoor weather query"),Weather->RefreshWeatherNow());
    TestFalse(TEXT("Open sky is not sheltered"),Weather->GetCurrentSnapshot().bSheltered);

    AActor* Roof=F.World->SpawnActor<AActor>(); auto* Box=NewObject<UBoxComponent>(Roof); Roof->SetRootComponent(Box);
    Box->SetBoxExtent(FVector(250,250,20)); Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Box->SetCollisionObjectType(ECC_WorldStatic);
    Box->SetCollisionResponseToAllChannels(ECR_Ignore); Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block); Box->RegisterComponent();
    Roof->SetActorLocation(FVector(0,0,500)); Box->UpdateBounds();
    const auto Before=F.Director->GetGlobalState(); TestTrue(TEXT("Roof shelter query"),Weather->RefreshShelterNow()); TestTrue(TEXT("Roof weather query"),Weather->RefreshWeatherNow());
    const auto Sheltered=Weather->GetCurrentSnapshot(); TestTrue(TEXT("Roof is sheltered"),Sheltered.bSheltered); TestEqual(TEXT("Rain presentation suppressed"),Sheltered.PrecipitationVisualMultiplier,0.0);
    TestEqual(TEXT("Shelter does not mutate climate serial"),F.Director->GetGlobalState().TransitionSerial,Before.TransitionSerial);

    Player->SetActorLocation(FVector(600,0,0)); TestTrue(TEXT("Doorway/outside refresh"),Weather->RefreshShelterNow()); Weather->RefreshWeatherNow();
    TestFalse(TEXT("Outside roof footprint is unsheltered"),Weather->GetCurrentSnapshot().bSheltered);
    Roof->SetActorLocation(FVector(600,0,500)); Box->UpdateBounds(); TestTrue(TEXT("Moving cover refresh"),Weather->RefreshShelterNow()); Weather->RefreshWeatherNow();
    TestTrue(TEXT("Moving cover shelters new position"),Weather->GetCurrentSnapshot().bSheltered); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherPresentationProfileTest, "RBWeather.Presentation.ProfileResolution", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherPresentationProfileTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    auto* Profile=NewObject<URBWeatherPresentationProfile>();
    FRBWeatherSnapshotAtLocation S; S.CurrentWeather=TEXT("Rain"); S.Environment.PrecipitationType=ERBWeatherPrecipitation::Rain;
    TestTrue(TEXT("Rain resolves default slot"),Profile->ResolveSlot(S)==&Profile->Rain);
    FRBWeatherPresentationOverride Override; Override.WeatherId=TEXT("Blizzard"); Profile->WeatherOverrides.Add(Override);
    S.CurrentWeather=TEXT("Blizzard"); S.Environment.PrecipitationType=ERBWeatherPrecipitation::Snow;
    TestTrue(TEXT("Named weather override wins"),Profile->ResolveSlot(S)==&Profile->WeatherOverrides[0].Slot);
    S.CurrentWeather=TEXT("Clear"); S.Environment.PrecipitationType=ERBWeatherPrecipitation::None;
    TestTrue(TEXT("Clear needs no precipitation art"),Profile->ResolveSlot(S)==nullptr); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherExposureBands, "RBWeather.Gameplay.EnvironmentalExposure", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherExposureBands::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; TestTrue(TEXT("Initialize"),F.Initialize());
    AActor* Character=F.World->SpawnActor<AActor>(); auto* Exposure=NewObject<URBWeatherEnvironmentalExposureComponent>(Character);
    Character->AddInstanceComponent(Exposure); Exposure->bUseAutomaticShelterTrace=false; Exposure->RegisterComponent();
    TestEqual(TEXT("Very cold band"),Exposure->ClassifyTemperature(-20),ERBWeatherTemperatureBand::VeryCold);
    TestEqual(TEXT("Cold band"),Exposure->ClassifyTemperature(-5),ERBWeatherTemperatureBand::Cold);
    TestEqual(TEXT("Normal band"),Exposure->ClassifyTemperature(10),ERBWeatherTemperatureBand::Normal);
    TestEqual(TEXT("Hot band"),Exposure->ClassifyTemperature(30),ERBWeatherTemperatureBand::Hot);
    TestEqual(TEXT("Very hot band"),Exposure->ClassifyTemperature(40),ERBWeatherTemperatureBand::VeryHot);
    TestTrue(TEXT("Force rain"),F.Director->ForceGlobalWeather(TEXT("Rain"),0));
    TestTrue(TEXT("Rain exposure refresh"),Exposure->RefreshExposureNow());
    auto E=Exposure->GetCurrentExposure();
    TestTrue(TEXT("Rain creates wet exposure"),E.bWetExposure);
    TestTrue(TEXT("Rain exposure intensity"),E.PrecipitationExposure01>0.5);
    TestFalse(TEXT("Rain alone is not warming"),E.bWarmingExposure);

    auto* Heat=NewObject<URBWeatherTemperatureSourceComponent>(Character);
    Character->AddInstanceComponent(Heat); Heat->RadiusCm=1000; Heat->TemperatureOffsetC=12; Heat->FalloffExponent=1; Heat->RegisterComponent();
    F.System->RegisterTemperatureSource(Heat);
    TestTrue(TEXT("Heated exposure refresh"),Exposure->RefreshExposureNow()); E=Exposure->GetCurrentExposure();
    TestTrue(TEXT("Local heat delta detected"),E.LocalTemperatureDeltaC>10.0);
    TestTrue(TEXT("Warming exposure detected"),E.bWarmingExposure);
    TestTrue(TEXT("Environmental temperature remains outside layer"),FMath::IsNearlyEqual(E.EnvironmentalTemperatureC,E.LocalTemperatureC-E.LocalTemperatureDeltaC,1e-6));
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherProfileScopedSeasons, "RBWeather.Hardening.ProfileScopedSeasonFilters", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherProfileScopedSeasons::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F;
    FRBWeatherTransitionRow TemperateOnly; TemperateOnly.FromWeather=TEXT("Clear"); TemperateOnly.ToWeather=TEXT("Rain"); TemperateOnly.Weight=10.0; TemperateOnly.AllowedSeasons={TEXT("Winter")};
    F.Transitions->AddRow(TEXT("TemperateOnly"),TemperateOnly);
    FRBWeatherClimateRow Tropical;
    Tropical.Season0.SeasonName=TEXT("Dry"); Tropical.Season0.MinimumTemperatureC=20; Tropical.Season0.MaximumTemperatureC=31;
    Tropical.Season1.SeasonName=TEXT("EarlyWet"); Tropical.Season1.MinimumTemperatureC=22; Tropical.Season1.MaximumTemperatureC=33;
    Tropical.Season2.SeasonName=TEXT("Wet"); Tropical.Season2.MinimumTemperatureC=23; Tropical.Season2.MaximumTemperatureC=34;
    Tropical.Season3.SeasonName=TEXT("LateWet"); Tropical.Season3.MinimumTemperatureC=22; Tropical.Season3.MaximumTemperatureC=33;
    FString Error; TestTrue(TEXT("Shared table validates for profile with different season names"),FRBWeatherSimulation::ValidateData(F.Presets,F.Transitions,Tropical,Error));
    FRBWeatherRuntimeState State; FRBWeatherSimulationEvents Events;
    TestTrue(TEXT("Tropical state initializes"),FRBWeatherSimulation::Initialize(State,F.Presets,F.Transitions,Tropical,TEXT("Clear"),TEXT("Dry"),0,12,99,Events,Error));
    TestTrue(TEXT("Tropical state advances"),FRBWeatherSimulation::Advance(State,F.Presets,F.Transitions,Tropical,TEXT("Dry"),0,13,1,99,Events,Error));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBWeatherTemperatureSourceScaling, "RBWeather.Performance.TemperatureSourceScaling", EAutomationTestFlags::EditorContext|EAutomationTestFlags::ClientContext|EAutomationTestFlags::EngineFilter)
bool FRBWeatherTemperatureSourceScaling::RunTest(const FString& Parameters)
{
    (void)Parameters; RBWeatherTests::FFixture F; TestTrue(TEXT("Initialize"),F.Initialize());
    AActor* Host=F.World->SpawnActor<AActor>(); if(!TestNotNull(TEXT("Source host"),Host)) return false;
    TArray<URBWeatherTemperatureSourceComponent*> Sources; Sources.Reserve(10000);
    const int32 Milestones[] = {10,100,1000,10000}; int32 Built=0;
    for (const int32 Target : Milestones)
    {
        const double RegisterStart=FPlatformTime::Seconds();
        while (Built<Target)
        {
            auto* Source=NewObject<URBWeatherTemperatureSourceComponent>(Host);
            Source->RadiusCm=100000.0; Source->TemperatureOffsetC=0.001; Source->FalloffExponent=1.0;
            Sources.Add(Source); F.System->RegisterTemperatureSource(Source); ++Built;
        }
        const double RegisterMs=(FPlatformTime::Seconds()-RegisterStart)*1000.0;
        FRBWeatherSnapshotAtLocation Q; TestTrue(TEXT("Warm query"),F.System->GetWeatherAtLocation(FVector(500,0,0),Q));
        const int32 QueryCount=Target<10000 ? 100 : 30; const double Start=FPlatformTime::Seconds();
        for(int32 Index=0;Index<QueryCount;++Index) TestTrue(TEXT("Measured query"),F.System->GetWeatherAtLocation(FVector(500,0,0),Q));
        const double MsPerQuery=((FPlatformTime::Seconds()-Start)*1000.0)/QueryCount;
        AddInfo(FString::Printf(TEXT("RBWeatherPerf TemperatureSources=%d RegisterDeltaMs=%.3f QueryMs=%.4f Queries=%d LocalTempC=%.3f"),Target,RegisterMs,MsPerQuery,QueryCount,Q.LocalTemperatureC));
        TestTrue(TEXT("Measured local temperature finite"),FMath::IsFinite(Q.LocalTemperatureC));
    }
    return true;
}

#endif
