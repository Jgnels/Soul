#include "SoulPlaytestRegionActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "SoulFounderPlaytestCampaignActor.h"
#include "UObject/ConstructorHelpers.h"

ASoulPlaytestRegionActor::ASoulPlaytestRegionActor()
{
    PrimaryActorTick.bCanEverTick = false;

    Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
    SetRootComponent(Marker);
    Marker->SetCollisionProfileName(TEXT("BlockAll"));
    Marker->SetGenerateOverlapEvents(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Sphere.Succeeded())
    {
        Marker->SetStaticMesh(Sphere.Object);
    }
    Marker->SetRelativeScale3D(FVector(1.25f, 1.25f, 0.45f));

    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(Marker);
    Label->SetRelativeLocation(FVector(0.0f, 0.0f, 170.0f));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetVerticalAlignment(EVRTA_TextCenter);
    Label->SetWorldSize(72.0f);
    Label->SetTextRenderColor(FColor::White);
    Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASoulPlaytestRegionActor::Configure(
    FName InRegionId,
    const FString& DisplayName,
    const FVector& Location)
{
    RegionId = InRegionId;
    SetActorLocation(Location);
    Label->SetText(FText::FromString(DisplayName));
}

void ASoulPlaytestRegionActor::SetVisualState(
    const FLinearColor& Color,
    bool bVisible,
    bool bCurrent)
{
    SetActorHiddenInGame(!bVisible);
    SetActorEnableCollision(bVisible);
    if (!bVisible) return;

    if (!DynamicMaterial && Marker)
    {
        DynamicMaterial = Marker->CreateAndSetMaterialInstanceDynamic(0);
    }
    if (DynamicMaterial)
    {
        DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
    }

    Marker->SetRelativeScale3D(
        bCurrent ? FVector(1.65f, 1.65f, 0.60f) : FVector(1.25f, 1.25f, 0.45f));
    Label->SetTextRenderColor(bCurrent ? FColor::Yellow : FColor::White);
}

void ASoulPlaytestRegionActor::NotifyActorOnClicked(FKey ButtonPressed)
{
    Super::NotifyActorOnClicked(ButtonPressed);
    if (ButtonPressed != EKeys::LeftMouseButton || !GetWorld()) return;

    for (TActorIterator<ASoulFounderPlaytestCampaignActor> It(GetWorld()); It; ++It)
    {
        It->HandleRegionClicked(RegionId);
        return;
    }
}
