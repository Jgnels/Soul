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
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulCampaignWorldGeometryTest,"Soul.Integration.CampaignWorld.TerrainAndRoutes",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoulCampaignWorldGeometryTest::RunTest(const FString&)
{
    auto* GI=NewObject<UGameInstance>();
    // Composition presents the existing stateful Human miniature. Its real
    // GameInstance owns both campaign and settlement subsystems.
    if(SoulCampaignTerrain::Composition())GI->Init();
    auto* State=SoulCampaignTerrain::Composition()?GI->GetSubsystem<USoulFounderPlaytestStateSubsystem>():NewObject<USoulFounderPlaytestStateSubsystem>(GI);
    if(!TestNotNull(TEXT("campaign authority"),State))return false;
    State->InitializeScenario();
    TestEqual(TEXT("presentation save namespace is isolated without changing domains"), State->GetCampaignSaveSlotName(),
        FString(SoulCampaignTerrain::Composition() ? TEXT("Soul.Composition3500.Founder") : TEXT("Soul.VerticalCampaign")));
    const auto& Locations=ASoulCampaignWorldActor::Locations();
    TestEqual(TEXT("every authoritative region has a presentation"),Locations.Num(),State->World.Regions.Num());
    for(const auto& Region:State->World.Regions)
    {
        const FVector* P=Locations.Find(Region.Key);if(!TestNotNull(TEXT("region location exists"),P))continue;
        const float Scale=SoulCampaignTerrain::Scale();
        const auto Bounds=SoulCampaignTerrain::FocusBounds();
        TestTrue(TEXT("location inside navigable world"),FMath::Abs(P->X)<Bounds.X&&FMath::Abs(P->Y)<Bounds.Y);
        const float Ground=ASoulCampaignWorldActor::HeightAt(P->X,P->Y);
        TestTrue(TEXT("landmark rests on terrain or ford bridge"),Region.Key==TEXT("river_ford") ? P->Z>=Ground&&(P->Z-Ground<60*Scale) : FMath::IsNearlyEqual(Ground,static_cast<float>(P->Z),1.f*Scale));
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
    if(SoulCampaignTerrain::Composition())
    {
        TestEqual(TEXT("all canonical composition regions"),Locations.Num(),36);
        TArray<TPair<FName,FName>> Edges;
        for(const auto& Region:State->World.Regions)for(FName Neighbor:Region.Value.Neighbors)
            if(Region.Key.LexicalLess(Neighbor))Edges.Add({Region.Key,Neighbor});
        TestEqual(TEXT("all legal composition routes"),Edges.Num(),51);
        int32 FerryEdges=0;
        for(const auto& E:Edges)
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
        FRBSaveDomainState Before;
        FString SaveError;
        TestTrue(TEXT("capture current composition through existing save provider"), State->CaptureRBSaveDomain_Implementation(Before, SaveError));
        FRBSaveDomainState WrongShape = Before;
        auto* Field = WrongShape.Fields.FindByPredicate([](const FRBSaveField& F){return F.Name == TEXT("CampaignJson");});
        if (TestNotNull(TEXT("campaign domain contains its authoritative JSON"), Field))
        {
            TSharedPtr<FJsonObject> Root;
            if (TestTrue(TEXT("captured domain JSON parses"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Field->StringValue), Root)))
            {
                const auto Owners = Root->GetObjectField(TEXT("owners"));
                TArray<FString> Keys; for (const auto& Pair : Owners->Values) Keys.Add(FString(*Pair.Key)); Keys.Sort();
                for (int32 I = 9; I < Keys.Num(); ++I) Owners->RemoveField(Keys[I]);
                Field->StringValue.Reset();
                FJsonSerializer::Serialize(Root.ToSharedRef(), TJsonWriterFactory<>::Create(&Field->StringValue));
                TestFalse(TEXT("nine-region checkpoint is not silently migrated into composition"), State->RestoreRBSaveDomain_Implementation(WrongShape, SaveError));
                FRBSaveDomainState After;
                TestTrue(TEXT("capture after rejected restore"), State->CaptureRBSaveDomain_Implementation(After, SaveError));
                const auto* OldField = Before.Fields.FindByPredicate([](const FRBSaveField& F){return F.Name == TEXT("CampaignJson");});
                const auto* NewField = After.Fields.FindByPredicate([](const FRBSaveField& F){return F.Name == TEXT("CampaignJson");});
                TestTrue(TEXT("rejected cross-shape save leaves campaign unchanged"), OldField && NewField && OldField->StringValue == NewField->StringValue);
            }
        }
        GI->Shutdown();
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
