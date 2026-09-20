#include "SoulSettlementStateSubsystem.h"

#include "Dom/JsonObject.h"
#include "RBSaveSubsystem.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Subsystems/SubsystemCollection.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoulSettlementSave, Log, All);

namespace
{
    constexpr int32 SoulSettlementSaveSchema = 1;

    int32 ConditionToInt(ESoulBuildingCondition Condition)
    {
        return static_cast<int32>(Condition);
    }

    bool IntToCondition(int32 Value, ESoulBuildingCondition& Out)
    {
        if (Value < static_cast<int32>(ESoulBuildingCondition::Unbuilt)
            || Value > static_cast<int32>(ESoulBuildingCondition::Ruined))
        {
            return false;
        }
        Out = static_cast<ESoulBuildingCondition>(Value);
        return true;
    }

    FName ConditionName(ESoulBuildingCondition Condition)
    {
        switch (Condition)
        {
        case ESoulBuildingCondition::Building: return TEXT("Building");
        case ESoulBuildingCondition::Intact: return TEXT("Intact");
        case ESoulBuildingCondition::Damaged: return TEXT("Damaged");
        case ESoulBuildingCondition::Ruined: return TEXT("Ruined");
        default: return TEXT("Unbuilt");
        }
    }
}

void USoulSettlementStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<URBSaveSubsystem>();
    SaveSubsystem = GetGameInstance()
        ? GetGameInstance()->GetSubsystem<URBSaveSubsystem>()
        : nullptr;

    if (!SaveSubsystem.IsValid())
    {
        UE_LOG(LogSoulSettlementSave, Error, TEXT("RB Save subsystem unavailable."));
        return;
    }

    FString Error;
    if (!SaveSubsystem->RegisterDomainProvider(this, Error))
    {
        UE_LOG(LogSoulSettlementSave, Error,
            TEXT("Failed to register Soul settlement save provider: %s"), *Error);
    }
}

void USoulSettlementStateSubsystem::Deinitialize()
{
    if (SaveSubsystem.IsValid())
    {
        SaveSubsystem->UnregisterDomainProvider(this);
    }
    SaveSubsystem.Reset();
    Super::Deinitialize();
}

FName USoulSettlementStateSubsystem::GetRBSaveDomainId_Implementation() const
{
    return TEXT("Soul.Settlements");
}

int32 USoulSettlementStateSubsystem::GetRBSaveSchemaVersion_Implementation() const
{
    return SoulSettlementSaveSchema;
}

FSoulSettlementState& USoulSettlementStateSubsystem::FindOrAddSettlement(FName SettlementId)
{
    FSoulSettlementState& State = Settlements.FindOrAdd(SettlementId);
    if (State.SettlementId.IsNone())
    {
        State.SettlementId = SettlementId;
    }
    return State;
}

FSoulSettlementState* USoulSettlementStateSubsystem::FindSettlement(FName SettlementId)
{
    return Settlements.Find(SettlementId);
}

const FSoulSettlementState* USoulSettlementStateSubsystem::FindSettlement(FName SettlementId) const
{
    return Settlements.Find(SettlementId);
}

bool USoulSettlementStateSubsystem::ApplySiegeAftermath(
    FName SettlementId,
    const FSoulSiegeAftermath& Aftermath)
{
    FSoulSettlementState* Settlement = Settlements.Find(SettlementId);
    if (!Settlement)
    {
        return false;
    }
    FSoulSiegeAftermathRules::Apply(*Settlement, Aftermath);
    return true;
}

bool USoulSettlementStateSubsystem::HasSettlement(FName SettlementId) const
{
    return Settlements.Contains(SettlementId);
}

int32 USoulSettlementStateSubsystem::GetWallIntegrity(FName SettlementId) const
{
    const FSoulSettlementState* Settlement = Settlements.Find(SettlementId);
    return Settlement ? Settlement->WallIntegrityPermille : -1;
}

FName USoulSettlementStateSubsystem::GetBuildingConditionName(
    FName SettlementId,
    FName BuildingId) const
{

    const FSoulSettlementState* Settlement = Settlements.Find(SettlementId);
    if (!Settlement)
    {
        return NAME_None;
    }
    const FSoulBuildingState* Building = Settlement->Buildings.Find(BuildingId);
    return Building ? ConditionName(Building->Condition) : NAME_None;
}

bool USoulSettlementStateSubsystem::SerializeToJson(
    FString& OutJson,
    FString& OutError) const
{
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> SettlementValues;

    TArray<FName> SettlementIds;
    Settlements.GetKeys(SettlementIds);
    SettlementIds.Sort(FNameLexicalLess());

    for (const FName SettlementId : SettlementIds)
    {
        const FSoulSettlementState& S = Settlements.FindChecked(SettlementId);
        TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
        Obj->SetStringField(TEXT("id"), S.SettlementId.ToString());
        Obj->SetStringField(TEXT("faction"), S.FactionId.ToString());
        Obj->SetStringField(TEXT("region"), S.RegionId.ToString());
        Obj->SetNumberField(TEXT("fortification"), S.FortificationLevel);
        Obj->SetNumberField(TEXT("wall_integrity"), S.WallIntegrityPermille);

        TArray<TSharedPtr<FJsonValue>> ScarValues;
        TArray<FName> Scars = S.PermanentScars.Array();
        Scars.Sort(FNameLexicalLess());
        for (const FName Scar : Scars)
        {
            ScarValues.Add(MakeShared<FJsonValueString>(Scar.ToString()));
        }
        Obj->SetArrayField(TEXT("scars"), ScarValues);

        TArray<TSharedPtr<FJsonValue>> BuildingValues;
        TArray<FName> BuildingIds;
        S.Buildings.GetKeys(BuildingIds);
        BuildingIds.Sort(FNameLexicalLess());

        for (const FName BuildingId : BuildingIds)
        {
            const FSoulBuildingState& B = S.Buildings.FindChecked(BuildingId);
            TSharedRef<FJsonObject> BObj = MakeShared<FJsonObject>();
            BObj->SetStringField(TEXT("id"), B.Id.ToString());
            BObj->SetNumberField(TEXT("level"), B.Level);
            BObj->SetNumberField(TEXT("integrity"), B.IntegrityPermille);
            BObj->SetNumberField(TEXT("days"), B.ConstructionDaysRemaining);
            BObj->SetNumberField(TEXT("condition"), ConditionToInt(B.Condition));
            BuildingValues.Add(MakeShared<FJsonValueObject>(BObj));
        }
        Obj->SetArrayField(TEXT("buildings"), BuildingValues);
        SettlementValues.Add(MakeShared<FJsonValueObject>(Obj));
    }

    Root->SetArrayField(TEXT("settlements"), SettlementValues);

    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);
    if (!FJsonSerializer::Serialize(Root, Writer))
    {
        OutError = TEXT("Failed to serialize Soul settlement state.");
        return false;
    }
    OutError.Reset();
    return true;
}

bool USoulSettlementStateSubsystem::CaptureRBSaveDomain_Implementation(
    FRBSaveDomainState& OutState,
    FString& OutError) const
{
    FString Json;
    if (!SerializeToJson(Json, OutError))
    {
        return false;
    }

    OutState = FRBSaveDomainState();
    OutState.DomainId = GetRBSaveDomainId_Implementation();
    OutState.SchemaVersion = SoulSettlementSaveSchema;

    FRBSaveField Field;
    Field.Name = TEXT("StateJson");
    Field.Type = ERBSaveFieldType::String;
    Field.StringValue = MoveTemp(Json);
    OutState.Fields.Add(MoveTemp(Field));
    return true;
}

bool USoulSettlementStateSubsystem::RestoreFromJson(
    const FString& Json,
    FString& OutError)
{

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutError = TEXT("Invalid Soul settlement JSON.");
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* SettlementValues = nullptr;
    if (!Root->TryGetArrayField(TEXT("settlements"), SettlementValues))
    {
        OutError = TEXT("Soul settlement JSON is missing settlements array.");
        return false;
    }

    TMap<FName, FSoulSettlementState> Restored;
    for (const TSharedPtr<FJsonValue>& Value : *SettlementValues)
    {
        const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
        if (!Value.IsValid() || !Value->TryGetObject(ObjPtr) || !ObjPtr || !ObjPtr->IsValid())
        {
            OutError = TEXT("Soul settlement entry is not an object.");
            return false;
        }
        const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

        FString IdText;
        if (!Obj->TryGetStringField(TEXT("id"), IdText) || IdText.IsEmpty())
        {
            OutError = TEXT("Soul settlement entry is missing id.");
            return false;
        }

        FSoulSettlementState S;
        S.SettlementId = FName(*IdText);

        FString Text;
        if (Obj->TryGetStringField(TEXT("faction"), Text)) S.FactionId = FName(*Text);
        if (Obj->TryGetStringField(TEXT("region"), Text)) S.RegionId = FName(*Text);

        double Number = 0.0;
        if (Obj->TryGetNumberField(TEXT("fortification"), Number))
            S.FortificationLevel = FMath::RoundToInt(Number);
        if (Obj->TryGetNumberField(TEXT("wall_integrity"), Number))
            S.WallIntegrityPermille = FMath::Clamp(FMath::RoundToInt(Number), 0, 1000);

        const TArray<TSharedPtr<FJsonValue>>* ScarValues = nullptr;
        if (Obj->TryGetArrayField(TEXT("scars"), ScarValues))
        {
            for (const TSharedPtr<FJsonValue>& ScarValue : *ScarValues)
            {
                FString ScarText;
                if (ScarValue.IsValid() && ScarValue->TryGetString(ScarText) && !ScarText.IsEmpty())
                {
                    S.PermanentScars.Add(FName(*ScarText));
                }
            }
        }

        const TArray<TSharedPtr<FJsonValue>>* BuildingValues = nullptr;
        if (Obj->TryGetArrayField(TEXT("buildings"), BuildingValues))
        {
            for (const TSharedPtr<FJsonValue>& BuildingValue : *BuildingValues)
            {
                const TSharedPtr<FJsonObject>* BObjPtr = nullptr;
                if (!BuildingValue.IsValid()
                    || !BuildingValue->TryGetObject(BObjPtr)
                    || !BObjPtr
                    || !BObjPtr->IsValid())
                {
                    OutError = TEXT("Soul building entry is not an object.");
                    return false;
                }

                const TSharedPtr<FJsonObject>& BObj = *BObjPtr;
                FString BuildingIdText;
                if (!BObj->TryGetStringField(TEXT("id"), BuildingIdText)
                    || BuildingIdText.IsEmpty())
                {
                    OutError = TEXT("Soul building entry is missing id.");
                    return false;
                }

                FSoulBuildingState B;
                B.Id = FName(*BuildingIdText);
                if (BObj->TryGetNumberField(TEXT("level"), Number))
                    B.Level = FMath::Max(0, FMath::RoundToInt(Number));
                if (BObj->TryGetNumberField(TEXT("integrity"), Number))
                    B.IntegrityPermille = FMath::Clamp(FMath::RoundToInt(Number), 0, 1000);
                if (BObj->TryGetNumberField(TEXT("days"), Number))
                    B.ConstructionDaysRemaining = FMath::Max(0, FMath::RoundToInt(Number));

                int32 ConditionValue = static_cast<int32>(ESoulBuildingCondition::Unbuilt);
                if (BObj->TryGetNumberField(TEXT("condition"), Number))
                    ConditionValue = FMath::RoundToInt(Number);
                if (!IntToCondition(ConditionValue, B.Condition))
                {
                    OutError = FString::Printf(
                        TEXT("Invalid building condition for %s."),
                        *B.Id.ToString());
                    return false;
                }
                S.Buildings.Add(B.Id, B);
            }
        }
        Restored.Add(S.SettlementId, MoveTemp(S));
    }

    Settlements = MoveTemp(Restored);
    OutError.Reset();
    return true;
}

bool USoulSettlementStateSubsystem::RestoreRBSaveDomain_Implementation(
    const FRBSaveDomainState& State,
    FString& OutError)
{
    if (State.DomainId != GetRBSaveDomainId_Implementation())
    {
        OutError = FString::Printf(
            TEXT("Unexpected Soul settlement domain: %s"),
            *State.DomainId.ToString());
        return false;
    }
    if (State.SchemaVersion != SoulSettlementSaveSchema)
    {
        OutError = FString::Printf(
            TEXT("Unsupported Soul settlement schema: %d"),
            State.SchemaVersion);
        return false;
    }

    const FRBSaveField* Field = State.Fields.FindByPredicate(
        [](const FRBSaveField& Candidate)
        {
            return Candidate.Name == TEXT("StateJson")
                && Candidate.Type == ERBSaveFieldType::String;
        });

    if (!Field)
    {
        OutError = TEXT("Soul settlement save is missing StateJson.");
        return false;
    }
    return RestoreFromJson(Field->StringValue, OutError);
}
