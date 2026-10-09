#include "SoulRealtimeBattleArena.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"

void ASoulRealtimeArenaGameMode::DressDragonBattlefield()
{
    // Runtime set dressing only. The licensed showcase and all donor assets remain untouched.
    // Keep the authored combat/entry lanes clear; decorative bones do not own navigation.
    struct FRelic { const TCHAR* Path; FVector Offset; float Yaw; float LongestSide; };
    const FRelic Relics[]={
        {TEXT("/Game/Dragon_graveyard/Meshes/Skeleton/SM_Skeleton_01"),FVector(-200,3700,0),25,1300},
        {TEXT("/Game/Dragon_graveyard/Meshes/Skeleton/SM_Skeleton_03"),FVector(600,-3800,0),-35,1100},
        {TEXT("/Game/Dragon_graveyard/Meshes/Skulls/SM_skull_01"),FVector(-2850,1350,0),-35,850},
        {TEXT("/Game/Dragon_graveyard/Meshes/Skulls/SM_skull_04"),FVector(2850,-1400,0),145,900},
        {TEXT("/Game/Dragon_graveyard/Meshes/Skeleton/SM_back_bone_01"),FVector(-1200,-3700,0),65,750}};
    for(const auto& Relic:Relics)
    {
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,Relic.Path);
        if(!Mesh) continue;
        FVector Location=ArenaOrigin+Relic.Offset;
        FHitResult Ground;
        if(!GetWorld()->LineTraceSingleByChannel(Ground,Location+FVector(0,0,8000),Location-FVector(0,0,8000),ECC_WorldStatic)) continue;
        // Avoid balancing a landmark on the showcase's steep enclosing cliffs.
        if(Ground.ImpactNormal.Z<.65f) continue;
        const auto Bounds=Mesh->GetBounds();
        const float Scale=Relic.LongestSide/FMath::Max(1.f,float(Bounds.BoxExtent.GetMax()*2));
        auto* Prop=GetWorld()->SpawnActor<AActor>();
        if(!Prop) continue;
        auto* Component=NewObject<UStaticMeshComponent>(Prop);
        Prop->SetRootComponent(Component);Prop->AddInstanceComponent(Component);
        Component->SetStaticMesh(Mesh);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetCanEverAffectNavigation(false);
        Component->SetMobility(EComponentMobility::Movable);
        Component->RegisterComponent();
        Prop->SetActorScale3D(FVector(Scale));
        Prop->SetActorRotation(FRotator(0,Relic.Yaw,0));
        const FVector Pivot=FRotator(0,Relic.Yaw,0).RotateVector(FVector(Bounds.Origin.X,Bounds.Origin.Y,0)*Scale);
        Location=Ground.ImpactPoint-Pivot;
        Location.Z-=(Bounds.Origin.Z-Bounds.BoxExtent.Z)*Scale;
        Prop->SetActorLocation(Location);
        Prop->Tags.Add(TEXT("SoulBattleRelic"));
        UE_LOG(LogTemp,Display,TEXT("SOUL_BATTLE_RELIC mesh=%s position=%s scale=%.3f"),Relic.Path,*Location.ToCompactString(),Scale);
    }
}

void ASoulRealtimeArenaGameMode::EquipVisualWeapons(
    ACharacter* Actor,int32 Side,ESoulRealtimeFormationRole FormationRole)
{
    if(!Actor) return;
    if(!UsesOrcCampaignRoster(Side) && !UsesVikingCampaignRoster(Side) && (FormationRole==ESoulRealtimeFormationRole::Ranged ||
        FormationRole==ESoulRealtimeFormationRole::Apex ||
        FormationRole==ESoulRealtimeFormationRole::Breaker)) return;
    const TCHAR* Weapon=nullptr;
    FName WeaponHand=TEXT("hand_r");
    if(UsesOrcCampaignRoster(Side))
    {
        Weapon=TEXT("/Game/Fantasy_Pack/Characters/Orc_Hummer/Mesh/SM_Hummer");
        WeaponHand=TEXT("CATRigRArmPalm");
    }
    else if(UsesVikingCampaignRoster(Side))
        Weapon=TEXT("/Game/Fantasy_Pack/Characters/Viking_Ulf/Mesh/SM_Viking_Axe");
    else if(Side==0)
    {
        // Aurora carries her authored weapon as part of the hero mesh.
        if(FormationRole==ESoulRealtimeFormationRole::Hero) return;
        Weapon=TEXT("/Game/Knights_Pack/Meshes/Knight_02/Weapons/SM_Knight_02_Sword");
    }
    else if(!UsesEvilVisualRoster())
        Weapon=(FormationRole==ESoulRealtimeFormationRole::Guard || FormationRole==ESoulRealtimeFormationRole::Hero)
            ? TEXT("/Game/Dwarf_Pack/King/Mesh/SM_Axe")
            : TEXT("/Game/Dwarf_Pack/Bedvar/Mesh/SM_Dwarf_Bedvar_Hammer_Mesh");
    else
    {
        if(FormationRole==ESoulRealtimeFormationRole::Guard || FormationRole==ESoulRealtimeFormationRole::Hero)
        {
            // This donor uses a CAT rig and a separate hammer, not the warrior
            // hand_r chain. Keep the attachment cosmetic and leave contact to RB.
            Weapon=TEXT("/Game/Fantasy_Pack/Characters/Orc_Hummer/Mesh/SM_Hummer");
            WeaponHand=TEXT("CATRigRArmPalm");
        }
        else Weapon=FormationRole==ESoulRealtimeFormationRole::Support
                ? TEXT("/Game/Fantasy_Pack/Characters/Fantasy_Barbarian/Mesh/SM_Fantasy_Barbarian_Weapon_01")
                : TEXT("/Game/Fantasy_Pack/Characters/Barbarian/Mesh/SM_Axel");
    }
    auto Attach=[&](const TCHAR* Path,FName Hand)
    {
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,Path);
        if(!Mesh || !Actor->GetMesh()->DoesSocketExist(Hand)) return;
        auto* Piece=NewObject<UStaticMeshComponent>(Actor);
        Actor->AddInstanceComponent(Piece);
        Piece->SetupAttachment(Actor->GetMesh(),Hand);
        if(Hand==TEXT("CATRigRArmPalm"))
        {
            // The CAT bones carry the donor's 2.54 authoring-unit conversion;
            // the separate static mesh is already in centimeters. Cancel that
            // inherited bone scale, while still following character scale.
            const FVector BoneScale=Actor->GetMesh()->GetSocketTransform(Hand,RTS_Component).GetScale3D();
            Piece->SetRelativeScale3D(BoneScale.Reciprocal());
            // The hammer's +Z shaft is opposite this palm's grip direction.
            // Grip ten centimeters above the butt, rather than at its end.
            Piece->SetRelativeRotation(FRotator(0,0,180));
            Piece->SetRelativeLocation(FVector(0,0,10)*BoneScale.Reciprocal());
        }
        Piece->SetStaticMesh(Mesh);
        Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Piece->SetCanEverAffectNavigation(false);
        Piece->ComponentTags.Add(TEXT("SoulVisualWeapon"));
        Piece->RegisterComponent();
        UE_LOG(LogTemp,Display,TEXT("SOUL_VISUAL_WEAPON mesh=%s hand=%s"),*Mesh->GetName(),*Hand.ToString());
    };
    // The qualified Dwarf Warrior rig's Weapon_Soket is identity on hand_r.
    // The knight shares that parent chain, so use the bone without modifying a donor socket.
    Attach(Weapon,WeaponHand);
    if(Side==0 && FormationRole==ESoulRealtimeFormationRole::Guard)
        Attach(TEXT("/Game/Knights_Pack/Meshes/Knight_04/Weapon/SM_Knight_04_Shield"),TEXT("hand_l"));
}
