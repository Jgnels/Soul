#pragma once
#include "RBCombatContracts.h"

// One router per attacker binding. Game thread only. No save data or durable truth.
class REFINEDBADGERCOMBATCORE_API FRBCombatRouter
{
public:
    static constexpr int32 Capacity = 128;
    FGuid BeginContact();
    void EndContact();
    FGuid GetActiveContact() const { return ActiveContact; }
    bool Publish(const FRBCombatHit& Hit, IRBCombatConsequenceSink& Sink, FString& OutError);
    int32 NumRemembered() const { return Published.Num(); }
private:
    struct FKey
    {
        FGuid Contact;
        FRBCombatantRef Attacker;
        FRBCombatantRef Victim;
        bool operator==(const FKey& Other) const
        { return Contact == Other.Contact && Attacker == Other.Attacker && Victim == Other.Victim; }
    };
    TArray<FKey> Published;
    TSet<FRBCombatantRef> ActiveVictims;
    FGuid ActiveContact;
    bool bPublishing = false;
};
