#include "SoulPlaytestRegionActor.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameInstance.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulSettlementScenarioData.h"
#include "SoulSettlementBuildingActor.h"
#include "SoulCampaignWorldActor.h"
#include "SoulCampaignTerrain.h"
#include "SoulHeartlandSites.h"
#include "Misc/Parse.h"
#include "Misc/CommandLine.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

ASoulPlaytestRegionActor::ASoulPlaytestRegionActor()
{
    PrimaryActorTick.bCanEverTick=false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("LocationRoot")));
    Marker=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SelectionCollision"));
    Marker->SetupAttachment(RootComponent);
    Marker->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Marker->SetCollisionProfileName(TEXT("BlockAll"));
    Marker->SetGenerateOverlapEvents(false);
    Marker->SetRelativeLocation(FVector(0,0,125));
    Marker->SetRelativeScale3D(FVector(4.5f,4.5f,4.f));
    Marker->SetHiddenInGame(true);
    Marker->SetCastShadow(false);
    Label=CreateDefaultSubobject<UTextRenderComponent>(TEXT("LocationName"));
    Label->SetupAttachment(RootComponent);
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetVerticalAlignment(EVRTA_TextCenter);
    Label->SetWorldSize(80.f);
    Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Label->SetRelativeLocation(FVector(0,-180,410));
}
void ASoulPlaytestRegionActor::Configure(FName InRegionId,const FString& DisplayName,const FVector& Location)
{
    RegionId=InRegionId;SetActorLocation(Location);Label->SetText(FText::FromString(DisplayName));
    const float SettlementScale=SoulCampaignTerrain::EvilCorridor() ?
        (RegionId==TEXT("human_capital") ? 6.f : RegionId==TEXT("orc_camp") ? 4.f : 2.5f) :
        SoulCampaignTerrain::RegionScale();
    SetActorScale3D(FVector(SettlementScale));
    auto Make=[this](const TCHAR* Shape,FLinearColor Color){return ASoulCampaignWorldActor::MakeInstances(this,Shape,Color);};
    auto Add=[](UInstancedStaticMeshComponent* M,FVector P,FVector S,FRotator R=FRotator::ZeroRotator){M->AddInstance(FTransform(R,P,S));};
    bool bAuthoredBinding = false;
    if (auto* GI = GetWorld()->GetGameInstance())
        if (auto* State = GI->GetSubsystem<USoulFounderPlaytestStateSubsystem>())
            if (const auto* Scenario = State->GetSettlementScenario(); Scenario && Scenario->RegionId == RegionId
                && !Scenario->MiniatureBaseMesh.IsNull() && !Scenario->MiniatureUpgradeMesh.IsNull())
            {
                bAuthoredBinding = true;
                AuthoredMiniature = GetWorld()->SpawnActor<ASoulSettlementBuildingActor>();
                if (AuthoredMiniature)
                {
                    AuthoredMiniature->SetOwner(this);
                    AuthoredMiniature->SettlementId = Scenario->SettlementId;
                    AuthoredMiniature->BuildingId = State->GetTavernBuildingId();
                    FTransform Transform = Scenario->MiniatureTransform;
                    Transform.AddToTranslation(Location);
                    SoulCampaignTerrain::MiniaturePlacement(RegionId,Transform);
                    AuthoredMiniature->SetActorTransform(Transform);
                    UStaticMesh* Base=Scenario->MiniatureBaseMesh.LoadSynchronous();
                    if(State->IsHeartlandEnabled() && RegionId==TEXT("human_capital"))
                    {
                        auto* DepthBase=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Soul/CampaignProxies/Heartland/SM_HumanCapital_Base_Depth_r1"));
                        const TCHAR* Names[]={TEXT("ArcaneHall"),TEXT("Barracks"),TEXT("Market")};
                        const FName Ids[]={TEXT("human.arcane_hall"),TEXT("human.barracks"),TEXT("human.market")};
                        UStaticMesh* Pieces[3]={};bool Ready=DepthBase!=nullptr;
                        for(int32 I=0;I<3;++I)
                        {
                            Pieces[I]=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Soul/CampaignProxies/Heartland/SM_HumanCapital_%s_Depth_r1"),Names[I]));
                            Ready&=Pieces[I]!=nullptr;
                        }
                        if(Ready)
                        {
                            Base=DepthBase;
                            for(int32 I=0;I<3;++I)
                            {
                                auto* Piece=GetWorld()->SpawnActor<ASoulSettlementBuildingActor>();
                                if(!Piece)continue;
                                Piece->SetOwner(this);Piece->SettlementId=Scenario->SettlementId;Piece->BuildingId=Ids[I];
                                Piece->MinimumBuildingLevel=I==0?1:2;Piece->SetActorTransform(Transform);
                                Piece->ConfigureMiniature(nullptr,Pieces[I],true);DevelopmentMiniatures.Add(Piece);
                            }
                            UE_LOG(LogTemp,Display,TEXT("SOUL_HEARTLAND_DEVELOPMENT_MINIATURES groups=%d shared_state=1"),DevelopmentMiniatures.Num());
                        }
                        else UE_LOG(LogTemp,Error,TEXT("SOUL_HEARTLAND_DEVELOPMENT_MINIATURES missing_owned_derivative"));
                    }
                    if (!AuthoredMiniature->ConfigureMiniature(Base,
                        Scenario->MiniatureUpgradeMesh.LoadSynchronous()))
                    {
                        UE_LOG(LogTemp, Error, TEXT("SOUL_SETTLEMENT_MINIATURE_FAIL region=%s"), *RegionId.ToString());
                    }
                    else
                    {
                        FVector Center, Extent;
                        AuthoredMiniature->GetActorBounds(false, Center, Extent);
                        Marker->SetWorldLocation(Center);
                        Marker->SetWorldScale3D((Extent + FVector(150,150,100))/50.f);
                        Marker->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                        Marker->SetCollisionResponseToAllChannels(ECR_Ignore);
                        Marker->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
                        Marker->SetCanEverAffectNavigation(false);
                        UE_LOG(LogTemp, Display, TEXT("SOUL_SETTLEMENT_MINIATURE_READY settlement=%s building=%s base=%s upgrade=%s"),
                            *Scenario->SettlementId.ToString(), *AuthoredMiniature->BuildingId.ToString(),
                            *Scenario->MiniatureBaseMesh.ToString(), *Scenario->MiniatureUpgradeMesh.ToString());
                    }
                }
            }
    const bool bHeartlandSite=DressHeartlandSite(this,RegionId);
    if(bAuthoredBinding)SoulCampaignTerrain::DressSettlementSurroundings(this,RegionId);
    const bool bProofRouteOnly = FParse::Param(FCommandLine::Get(), TEXT("SoulDwarfSettlementProof"))
        && (RegionId == TEXT("dwarf_forge_approach") || RegionId == TEXT("dwarf_snow_basin"));
    // These canonical nodes are route locations, not towns. The authored-city
    // qualification must not dress them with generic placeholder houses.
    if(!bAuthoredBinding && !bHeartlandSite && !bProofRouteOnly && !SoulCampaignTerrain::DressRegion(this,RegionId))
    {
    auto* Stone=Make(TEXT("Cube"),FLinearColor(.42f,.40f,.32f));
    auto* DarkStone=Make(TEXT("Cube"),FLinearColor(.20f,.23f,.22f));
    auto* Wood=Make(TEXT("Cube"),FLinearColor(.23f,.13f,.065f));
    auto* Roof=Make(TEXT("Cone"),FLinearColor(.20f,.24f,.28f));
    auto* WarmRoof=Make(TEXT("Cone"),FLinearColor(.38f,.18f,.095f));
    auto* Tile=Make(TEXT("Cube"),FLinearColor(.38f,.18f,.095f));
    auto* Slate=Make(TEXT("Cube"),FLinearColor(.20f,.24f,.28f));
    auto Gable=[&](UInstancedStaticMeshComponent* M,FVector P,float Width,float Depth)
    {
        for(int32 Side:{-1,1})Add(M,P+FVector(0,Side*Depth*.23f,0),FVector(Width*.01f,Depth*.0062f,.12f),FRotator(0,0,Side*35.f));
    };
    const bool Capital=RegionId==TEXT("human_capital"),Fort=RegionId==TEXT("orc_camp"),Watch=RegionId==TEXT("orc_watch"),Pass=RegionId==TEXT("north_pass");
    if(Capital||Fort||Watch||Pass)
    {
        const float R=Capital||Fort?170.f:100.f;
        const float H=Fort?220.f:Capital?175.f:125.f;
        auto* Wall=Fort||Watch||Pass?DarkStone:Stone;
        for(int32 Side:{-1,1})
        {
            Add(Wall,FVector(Side*R,0,H*.35f),FVector(.4f,R*.022f,H*.007f));
            Add(Wall,FVector(0,Side*R,H*.35f),FVector(R*.022f,.4f,H*.007f));
            for(int32 Other:{-1,1})
            {
                Add(Wall,FVector(Side*R,Other*R,H*.55f),FVector(.8f,.8f,H*.011f));
                Add(Fort?WarmRoof:Roof,FVector(Side*R,Other*R,H+50),FVector(1.1f,1.1f,1.2f));
            }
        }
        Add(Wall,FVector(0,50,H*.7f),FVector(1.4f,1.5f,H*.014f));
        Gable(Slate,FVector(0,50,H*1.4f+30),190.f,190.f);
        // Visible gate and crenellations give stonework a settlement silhouette.
        Add(Wood,FVector(0,-R-4,42),FVector(.7f,.12f,.84f));
        for(int32 I=-3;I<=3;++I) for(int32 Side:{-1,1})
            Add(Wall,FVector(I*R/3,Side*R,H*.75f),FVector(.22f,.46f,.28f));
        if(Capital)
        {
            for(int32 I=0;I<9;++I)
            {
                FVector P(-270+(I%3)*90,-270-(I/3)*90,40);
                Add(Stone,P,FVector(.70f,.7f,.8f));Gable(Tile,P+FVector(0,0,60),95.f,100.f);
                Add(Stone,P+FVector(25,15,83),FVector(.16f,.16f,.6f));
            }
        }
    }
    else if(RegionId==TEXT("ancient_shrine"))
    {
        for(int32 Step=0;Step<3;++Step)Add(Stone,FVector(0,0,Step*18),FVector(3.4f-Step*.5f,2.7f-Step*.4f,.2f));
        for(int32 Side:{-1,1})for(int32 I=0;I<3;++I)Add(Stone,FVector(Side*95,-80+I*80,120),FVector(.3f,.3f,1.8f));
        Add(Stone,FVector(0,0,222),FVector(2.5f,2.5f,.28f));
        Gable(Slate,FVector(0,0,265),280.f,280.f);
    }
    else if(RegionId==TEXT("river_ford"))
    {
        for(int32 I=-3;I<=3;++I)Add(Wood,FVector(I*50.f,0,18),FVector(.46f,1.8f,.12f));
        for(int32 Side:{-1,1})Add(Wood,FVector(0,Side*90,48),FVector(3.6f,.07f,.07f));
        for(int32 Side:{-1,1})for(int32 X:{-1,1})Add(Wood,FVector(X*155,Side*90,30),FVector(.1f,.1f,.65f));
    }
    else if(RegionId==TEXT("old_quarry"))
    {
        for(int32 I=0;I<12;++I)Add(Stone,FVector(-150+(I%4)*80,(I/4)*70,25+(I/4)*20),FVector(.65f,.65f,.5f),FRotator(0,I*9.f,0));
        Add(Wood,FVector(-100,-80,100),FVector(.2f,.2f,2.f));
        Add(Wood,FVector(0,-80,185),FVector(2.2f,.2f,.2f));
        Add(Wood,FVector(95,-80,120),FVector(.045f,.045f,1.3f));
    }
    else
    {
        Add(Stone,FVector(0,45,50),FVector(1.2f,.9f,1.f));
        Gable(Tile,FVector(0,45,127),155.f,155.f);
        Add(Stone,FVector(42,60,155),FVector(.2f,.2f,.8f));
        Add(Wood,FVector(0,92,36),FVector(.25f,.035f,.7f));
        Add(Wood,FVector(-120,-80,55),FVector(.12f,.12f,1.1f));
        Add(Wood,FVector(-120,-80,95),FVector(.8f,.12f,.15f));
        if(RegionId==TEXT("forest_edge"))
            for(int32 I=0;I<5;++I)Add(Wood,FVector(120,I*22,15),FVector(.9f,.17f,.17f));
    }
    }
    const bool bRetainedPopulation = SoulCampaignTerrain::EvilCorridor() && !bAuthoredBinding
        && !FParse::Param(FCommandLine::Get(),TEXT("SoulDwarfSettlementProof"))
        && RegionId != TEXT("human_capital")
        && !FParse::Param(FCommandLine::Get(),TEXT("SoulPopulationBaseline"));
    if(bRetainedPopulation||bHeartlandSite)
    {
        // Fit the existing founder hit target to its visible settlement, before
        // standards, selection marks and garrisons add unrelated decoration.
        FBox Footprint(Location-FVector(600,600,100),Location+FVector(600,600,800));
        TArray<UStaticMeshComponent*> Components;GetComponents(Components);
        for(auto* Mesh:Components)if(Mesh!=Marker&&Mesh->GetStaticMesh())
        {
            const FBox MeshBox=Mesh->GetStaticMesh()->GetBounds().GetBox();
            if(auto* Instances=Cast<UInstancedStaticMeshComponent>(Mesh))
            {
                for(int32 I=0;I<Instances->GetInstanceCount();++I)
                {
                    FTransform InstanceWorld;
                    if(Instances->GetInstanceTransform(I,InstanceWorld,true))Footprint+=MeshBox.TransformBy(InstanceWorld);
                }
            }
            else Footprint+=MeshBox.TransformBy(Mesh->GetComponentTransform());
        }
        Footprint=Footprint.ExpandBy(FVector(200,200,100));
        Marker->SetWorldLocation(Footprint.GetCenter());
        Marker->SetWorldScale3D(Footprint.GetSize()/(Marker->GetStaticMesh()->GetBounds().BoxExtent*2));
        Marker->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Marker->SetCollisionResponseToAllChannels(ECR_Ignore);
        Marker->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
        Marker->SetCanEverAffectNavigation(false);
    }
    const bool Fort=RegionId==TEXT("orc_camp");
    const bool bTerrainMiniature=SoulCampaignTerrain::Enabled();
    FVector BannerBase=bTerrainMiniature?FVector(120,-65,0):FVector(200,-95,0);
    if(bTerrainMiniature)
    {
        const float Scale=GetActorScale3D().X;
        BannerBase.Z=(ASoulCampaignWorldActor::HeightAt(Location.X+BannerBase.X*Scale,
            Location.Y+BannerBase.Y*Scale)-Location.Z)/Scale;
    }
    Standard=Make(TEXT("Cube"),FLinearColor::White);
    if(bTerrainMiniature)
    {
        // Restrained settlement standard: allegiance is readable in the world.
        auto* Pole=Make(TEXT("Cylinder"),FLinearColor(.30f,.25f,.16f));
        Add(Pole,BannerBase+FVector(0,0,75),FVector(.035f,.035f,1.5f));
        Add(Standard,BannerBase+FVector(28,0,128),FVector(.56f,.045f,.38f));
    }
    else
    {
        auto* Pole=Make(TEXT("Cylinder"),FLinearColor(.30f,.25f,.16f));
        Add(Pole,BannerBase+FVector(0,0,110),FVector(.05f,.05f,2.2f));
        Add(Standard,BannerBase+FVector(32,0,193),FVector(.64f,.07f,.42f));
    }
    BannerMaterial=Cast<UMaterialInstanceDynamic>(Standard->GetMaterial(0));
    Selection=Make(TEXT("Cube"),FLinearColor(.86f,.69f,.30f));
    for(int32 X:{-1,1})for(int32 Y:{-1,1})
    {
        Add(Selection,FVector(X*220,Y*220,12),FVector(.7f,.055f,.05f));
        Add(Selection,FVector(X*250,Y*190,12),FVector(.055f,.65f,.05f));
    }
    Garrison=Make(TEXT("Cube"),FLinearColor(.46f,.20f,.11f));
    for(int32 I=0;I<6;++I)
    {
        const float Miniature=SoulCampaignTerrain::Enabled()?.25f:1.f;
        FVector P((-35+(I%3)*35)*Miniature,(Fort?270.f:190.f)+(I/3)*35*Miniature,0);
        const float Scale=GetActorScale3D().X;
        P.Z=(ASoulCampaignWorldActor::HeightAt(Location.X+P.X*Scale,Location.Y+P.Y*Scale)-Location.Z)/Scale;
        Add(Garrison,P+FVector(0,0,28*Miniature),FVector(.19f,.19f,.56f)*Miniature);
        Add(Garrison,P+FVector(0,0,66*Miniature),FVector(.18f,.18f,.18f)*Miniature);
    }
}
void ASoulPlaytestRegionActor::SetVisualState(const FLinearColor& Color,bool bExplored,bool bCurrent,bool bVisible,bool bSelected,int32 Defenders)
{
    SetActorHiddenInGame(!bExplored);
    if (AuthoredMiniature) AuthoredMiniature->SetActorHiddenInGame(!bExplored);
    for(const auto& Piece:DevelopmentMiniatures)if(Piece)Piece->SetActorHiddenInGame(!bExplored);
    SetActorEnableCollision(bExplored);
    if(!bExplored)return;
    if(BannerMaterial)BannerMaterial->SetVectorParameterValue(TEXT("Tint"),bVisible?Color:FLinearColor(.22f,.25f,.27f));
    if(Selection)Selection->SetVisibility(bSelected||bCurrent);
    if(Garrison)Garrison->SetVisibility(bVisible&&Defenders>0);
    Label->SetTextRenderColor(bSelected?FColor(255,221,139):bVisible?FColor(234,229,206):FColor(151,163,171));
    if(auto* PC=GetWorld()->GetFirstPlayerController())if(PC->PlayerCameraManager)
    {
        Label->SetWorldRotation((-PC->PlayerCameraManager->GetCameraRotation().Vector()).Rotation());
        const float Distance=FVector::Distance(PC->PlayerCameraManager->GetCameraLocation(),Label->GetComponentLocation());
        if(SoulCampaignTerrain::EvilCorridor())Label->SetWorldSize(Distance/GetActorScale3D().X*.014f);
        else Label->SetWorldSize(FMath::Clamp(Distance/SoulCampaignTerrain::Scale()*(SoulCampaignTerrain::Enabled()?.012f:.016f),20.f,80.f));
    }
}
