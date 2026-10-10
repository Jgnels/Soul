#include "SoulSettlementEnvironmentRegistry.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
bool FSoulSettlementEnvironmentRegistry::Load(TMap<FName,FSoulSettlementEnvironmentBinding>& Out,FString& Error)
{
    FString Text;TSharedPtr<FJsonObject> Root;const TArray<TSharedPtr<FJsonValue>>* Entries=nullptr;
    if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectDir()/TEXT("Data/SettlementEnvironments/EnvironmentRegistry.json")))
        ||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root)||!Root.IsValid()
        ||Root->GetIntegerField(TEXT("schema"))!=1||!Root->TryGetArrayField(TEXT("entries"),Entries))
    {Error=TEXT("Settlement environment registry unavailable or malformed.");return false;}
    TMap<FName,FSoulSettlementEnvironmentBinding> Parsed;
    for(const auto& V:*Entries)
    {
        if(!V.IsValid()||V->Type!=EJson::Object){Error=TEXT("Invalid environment entry.");return false;}
        auto O=V->AsObject();FSoulSettlementEnvironmentBinding B;FString Id,Faction,Type;
        if(!O->TryGetStringField(TEXT("settlement_id"),Id)||Id.IsEmpty()||!O->TryGetStringField(TEXT("faction"),Faction)
            ||!O->TryGetStringField(TEXT("settlement_type"),Type)||!O->TryGetBoolField(TEXT("battle_enabled"),B.bBattleEnabled)
            ||!O->TryGetBoolField(TEXT("authored_environment_available"),B.bAuthoredAvailable))
        {Error=TEXT("Incomplete environment binding.");return false;}
        B.SettlementId=FName(*Id);B.Faction=FName(*Faction);B.SettlementType=FName(*Type);
        O->TryGetStringField(TEXT("visit_environment"),B.VisitEnvironment);O->TryGetStringField(TEXT("city_battle_environment"),B.CityBattleEnvironment);
        O->TryGetStringField(TEXT("siege_environment"),B.SiegeEnvironment);O->TryGetStringField(TEXT("field_battle_environment"),B.FieldBattleEnvironment);
        O->TryGetStringField(TEXT("development_profile"),B.DevelopmentProfile);
        const TArray<TSharedPtr<FJsonValue>>* A=nullptr;
        if(B.bBattleEnabled)
        {
            if(!B.bAuthoredAvailable||!B.CityBattleEnvironment.StartsWith(TEXT("/Game/Soul/"))||!O->TryGetArrayField(TEXT("arena_origin"),A)||A->Num()!=3)
            {Error=TEXT("Enabled city battle requires an owned map and measured origin.");return false;}
            for(const auto& N:*A)if(N->Type!=EJson::Number||!FMath::IsFinite(N->AsNumber())){Error=TEXT("Invalid city battle origin.");return false;}
            B.ArenaOrigin=FVector((*A)[0]->AsNumber(),(*A)[1]->AsNumber(),(*A)[2]->AsNumber());
        }
        if(Parsed.Contains(B.SettlementId)){Error=TEXT("Duplicate settlement environment.");return false;}
        Parsed.Add(B.SettlementId,MoveTemp(B));
    }
    Out=MoveTemp(Parsed);Error.Reset();return true;
}
