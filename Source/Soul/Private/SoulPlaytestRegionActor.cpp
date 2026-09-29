#include "SoulPlaytestRegionActor.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "SoulCampaignWorldActor.h"
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
    auto Make=[this](const TCHAR* Shape,FLinearColor Color){return ASoulCampaignWorldActor::MakeInstances(this,Shape,Color);};
    auto Add=[](UInstancedStaticMeshComponent* M,FVector P,FVector S,FRotator R=FRotator::ZeroRotator){M->AddInstance(FTransform(R,P,S));};
    auto* Stone=Make(TEXT("Cube"),FLinearColor(.42f,.40f,.32f));
    auto* DarkStone=Make(TEXT("Cube"),FLinearColor(.20f,.23f,.22f));
    auto* Wood=Make(TEXT("Cube"),FLinearColor(.23f,.13f,.065f));
    auto* Roof=Make(TEXT("Cone"),FLinearColor(.20f,.24f,.28f));
    auto* WarmRoof=Make(TEXT("Cone"),FLinearColor(.38f,.18f,.095f));
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
        Add(Roof,FVector(0,50,H*1.4f+35),FVector(2.f,2.f,1.2f));
        // Visible gate and crenellations give stonework a settlement silhouette.
        Add(Wood,FVector(0,-R-4,42),FVector(.7f,.12f,.84f));
        for(int32 I=-3;I<=3;++I) for(int32 Side:{-1,1})
            Add(Wall,FVector(I*R/3,Side*R,H*.75f),FVector(.22f,.46f,.28f));
        if(Capital)
        {
            for(int32 I=0;I<9;++I)
            {
                FVector P(-270+(I%3)*90,-270-(I/3)*90,40);
                Add(Stone,P,FVector(.70f,.7f,.8f));Add(WarmRoof,P+FVector(0,0,65),FVector(1.f,1.f,.8f));
            }
        }
    }
    else if(RegionId==TEXT("ancient_shrine"))
    {
        for(int32 Step=0;Step<3;++Step)Add(Stone,FVector(0,0,Step*18),FVector(3.4f-Step*.5f,2.7f-Step*.4f,.2f));
        for(int32 Side:{-1,1})for(int32 I=0;I<3;++I)Add(Stone,FVector(Side*95,-80+I*80,120),FVector(.3f,.3f,1.8f));
        Add(Stone,FVector(0,0,222),FVector(2.5f,2.5f,.28f));
        Add(Roof,FVector(0,0,260),FVector(2.7f,2.7f,.65f));
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
        Add(WarmRoof,FVector(0,45,135),FVector(1.7f,1.4f,1.f));
        Add(Wood,FVector(-120,-80,55),FVector(.12f,.12f,1.1f));
        Add(Wood,FVector(-120,-80,95),FVector(.8f,.12f,.15f));
        if(RegionId==TEXT("forest_edge"))
            for(int32 I=0;I<5;++I)Add(Wood,FVector(120,I*22,15),FVector(.9f,.17f,.17f));
    }
    auto* Pole=Make(TEXT("Cylinder"),FLinearColor(.30f,.25f,.16f));
    Add(Pole,FVector(200,-95,110),FVector(.05f,.05f,2.2f));
    Standard=Make(TEXT("Cube"),FLinearColor::White);
    Add(Standard,FVector(232,-95,193),FVector(.64f,.07f,.42f));
    BannerMaterial=Cast<UMaterialInstanceDynamic>(Standard->GetMaterial(0));
    Selection=Make(TEXT("Cube"),FLinearColor(.86f,.69f,.30f));
    for(int32 X:{-1,1})for(int32 Y:{-1,1})
    {
        Add(Selection,FVector(X*220,Y*220,12),FVector(.7f,.055f,.05f));
        Add(Selection,FVector(X*250,Y*190,12),FVector(.055f,.65f,.05f));
    }
    Garrison=Make(TEXT("Cube"),FLinearColor(.46f,.20f,.11f));
    for(int32 I=0;I<6;++I)Add(Garrison,FVector(75+(I%3)*35,-95-(I/3)*35,28),FVector(.19f,.19f,.56f));
}
void ASoulPlaytestRegionActor::SetVisualState(const FLinearColor& Color,bool bExplored,bool bCurrent,bool bVisible,bool bSelected,int32 Defenders)
{
    SetActorHiddenInGame(!bExplored);
    SetActorEnableCollision(bExplored);
    if(!bExplored)return;
    if(BannerMaterial)BannerMaterial->SetVectorParameterValue(TEXT("Tint"),bVisible?Color:FLinearColor(.22f,.25f,.27f));
    if(Selection)Selection->SetVisibility(bSelected||bCurrent);
    if(Garrison)Garrison->SetVisibility(bVisible&&Defenders>0);
    Label->SetTextRenderColor(bSelected?FColor(255,221,139):bVisible?FColor(234,229,206):FColor(151,163,171));
    if(auto* PC=GetWorld()->GetFirstPlayerController())if(PC->PlayerCameraManager)
        Label->SetWorldRotation((-PC->PlayerCameraManager->GetCameraRotation().Vector()).Rotation());
}
