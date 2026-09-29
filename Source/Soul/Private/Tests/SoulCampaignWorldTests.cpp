#include "Misc/AutomationTest.h"
#include "SoulCampaignWorldActor.h"
#include "SoulCampaignTerrain.h"
#include "SoulCampaignCamera.h"
#include "SoulPlaytestRegionActor.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Materials/MaterialInterface.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulCampaignWorldGeometryTest,"Soul.Integration.CampaignWorld.TerrainAndRoutes",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoulCampaignWorldGeometryTest::RunTest(const FString&)
{
    auto* GI=NewObject<UGameInstance>();
    auto* State=NewObject<USoulFounderPlaytestStateSubsystem>(GI);State->InitializeScenario();
    const auto& Locations=ASoulCampaignWorldActor::Locations();
    TestEqual(TEXT("every authoritative region has a presentation"),Locations.Num(),State->World.Regions.Num());
    for(const auto& Region:State->World.Regions)
    {
        const FVector* P=Locations.Find(Region.Key);if(!TestNotNull(TEXT("region location exists"),P))continue;
        const float Scale=SoulCampaignTerrain::Scale();
        TestTrue(TEXT("location inside navigable world"),FMath::Abs(P->X)<ASoulCampaignCamera::MaxFocusX*Scale&&FMath::Abs(P->Y)<ASoulCampaignCamera::MaxFocusY*Scale);
        const float Ground=ASoulCampaignWorldActor::HeightAt(P->X,P->Y);
        TestTrue(TEXT("landmark rests on terrain or ford bridge"),Region.Key==TEXT("river_ford") ? P->Z>=Ground&&P->Z-Ground<60*Scale : FMath::IsNearlyEqual(Ground,static_cast<float>(P->Z),1.f*Scale));
        const FVector Party=ASoulCampaignWorldActor::PartyAnchor(Region.Key);
        TestTrue(TEXT("stationed company clears terrain"),Party.Z>=ASoulCampaignWorldActor::HeightAt(Party.X,Party.Y));
        for(FName To:Region.Value.Neighbors) for(int32 I=0;I<=20;++I)
        {
            const float T=I/20.f;
            const FVector Point=ASoulCampaignWorldActor::RoadPoint(Region.Key,To,T);
            const FVector Reverse=ASoulCampaignWorldActor::RoadPoint(To,Region.Key,1-T);
            TestTrue(TEXT("one physical road in both directions"),Point.Equals(Reverse,.1f));
            TestTrue(TEXT("road remains above terrain"),Point.Z>=ASoulCampaignWorldActor::HeightAt(Point.X,Point.Y));
            TestFalse(TEXT("finite geometry"),Point.ContainsNaN());
        }
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulCampaignWorldDefaultsTest,"Soul.Integration.CampaignWorld.CameraAndSelection",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoulCampaignWorldDefaultsTest::RunTest(const FString&)
{
    const auto* Camera=GetDefault<ASoulCampaignCamera>();
    TestTrue(TEXT("perspective strategy camera"),Camera->GetCameraComponent()->ProjectionMode==ECameraProjectionMode::Perspective);
    TestTrue(TEXT("bounded useful zoom interval"),ASoulCampaignCamera::MinDistance>500&&ASoulCampaignCamera::MaxDistance>ASoulCampaignCamera::MinDistance);
    const auto* Region=GetDefault<ASoulPlaytestRegionActor>();
    TestTrue(TEXT("hidden collision proxy"),Region->Marker->bHiddenInGame);
    TestEqual(TEXT("location blocks cursor trace"),Region->Marker->GetCollisionResponseToChannel(ECC_Visibility),ECR_Block);
    TestNotNull(TEXT("project-owned terrain material exists"),LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Soul/Campaign/M_CampaignTerrain.M_CampaignTerrain")));
    return true;
}
#endif
