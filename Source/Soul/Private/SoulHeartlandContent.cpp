#include "SoulHeartlandContent.h"
#include "SoulSettlementScenarioData.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
bool FSoulHeartlandContent::Load(USoulSettlementScenarioData* Scenario,FSoulHeartlandContent& Out,FString& Error)
{
    FString Text;TSharedPtr<FJsonObject> Root;
    if(!Scenario||!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectDir()/TEXT("Data/SettlementEnvironments/HeartlandDevelopment.json")))
        ||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root)||!Root.IsValid())
    {Error=TEXT("Heartland development content missing.");return false;}
    const TArray<TSharedPtr<FJsonValue>>* Buildings=nullptr;
    if(!Root->TryGetArrayField(TEXT("buildings"),Buildings)){Error=TEXT("Missing Heartland buildings.");return false;}
    for(const auto& V:*Buildings)
    {
        if(V->Type!=EJson::Object)return false;auto O=V->AsObject();FSoulBuildingDevelopmentSpec D;
        D.BuildingId=FName(*O->GetStringField(TEXT("id")));D.DisplayName=FText::FromString(O->GetStringField(TEXT("name")));
        D.Category=TEXT("heartland");D.BuildDays=O->GetIntegerField(TEXT("days"));D.MaxLevel=O->GetIntegerField(TEXT("max_level"));D.BuildCost.Add(TEXT("gold"),O->GetIntegerField(TEXT("gold")));
        if(D.BuildDays<1||D.BuildDays>3){Error=TEXT("Heartland buildings must take 1-3 days.");return false;}
        for(const auto& P:O->GetArrayField(TEXT("prerequisites")))D.Prerequisites.Add(FName(*P->AsString()));
        for(const auto& U:O->GetArrayField(TEXT("unlocks")))D.UnlockIds.Add(FName(*U->AsString()));
        if(!Scenario->Buildings.ContainsByPredicate([&](const auto& B){return B.BuildingId==D.BuildingId;}))
        {FSoulInitialBuildingSpec B;B.BuildingId=D.BuildingId;B.bBuilt=false;B.Level=0;B.IntegrityPermille=0;Scenario->Buildings.Add(B);}
        Scenario->DevelopmentDefinitions.RemoveAll([&](const auto& B){return B.BuildingId==D.BuildingId;});Scenario->DevelopmentDefinitions.Add(D);
    }
    if(!Scenario->ValidateDefinition(Error))return false;
    FSoulHeartlandContent C;auto A=Root->GetObjectField(TEXT("hero_affinity"));C.HeroId=FName(*A->GetStringField(TEXT("hero_id")));C.PrimarySchool=FName(*A->GetStringField(TEXT("primary")));
    for(const auto& V:A->GetArrayField(TEXT("secondary")))C.SecondarySchools.Add(FName(*V->AsString()));
    for(const auto& V:A->GetArrayField(TEXT("forbidden")))C.ForbiddenSchools.Add(FName(*V->AsString()));
    for(const auto& V:Root->GetArrayField(TEXT("spells")))
    {auto O=V->AsObject();FSoulHeartlandSpell S;S.Id=FName(*O->GetStringField(TEXT("id")));S.School=FName(*O->GetStringField(TEXT("school")));S.RequiredBuilding=FName(*O->GetStringField(TEXT("building")));if(!Scenario->FindDevelopmentDefinition(S.RequiredBuilding)){Error=TEXT("Spell requires an undefined building.");return false;}C.Spells.Add(S);}
    for(const auto& V:Root->GetArrayField(TEXT("sites")))
    {auto O=V->AsObject();FSoulHeartlandSite S;S.Id=FName(*O->GetStringField(TEXT("id")));S.Region=FName(*O->GetStringField(TEXT("region")));S.Name=O->GetStringField(TEXT("name"));S.Effect=FName(*O->GetStringField(TEXT("effect")));S.Amount=O->GetIntegerField(TEXT("amount"));S.ActionCost=O->GetIntegerField(TEXT("cost_ap"));if(S.Amount<1||S.ActionCost<1||(S.Effect!=TEXT("gold")&&S.Effect!=TEXT("mana")))return false;C.Sites.Add(S);}
    Out=MoveTemp(C);Error.Reset();return true;
}
