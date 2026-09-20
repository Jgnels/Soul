#include "RBFoundationSubsystem.h"

#include "Dom/JsonObject.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
bool ReadNameArray(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field,
    TArray<FName>& Out, FString& OutError)
{
    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!Object->TryGetArrayField(Field, Values)) return true;
    for (const TSharedPtr<FJsonValue>& Value : *Values)
    {
        FString Text;
        if (!Value.IsValid() || !Value->TryGetString(Text) || Text.IsEmpty())
        {
            OutError = FString::Printf(TEXT("Manifest field '%s' contains a non-string value."), Field);
            return false;
        }
        Out.Add(FName(*Text));
    }
    return true;
}
}
bool URBFoundationSubsystem::RegisterInstalledDeclarativeProducts(FString& OutError)
{
    const TSharedPtr<IPlugin> FoundationPlugin = IPluginManager::Get().FindPlugin(TEXT("RBFoundation"));
    if (!FoundationPlugin.IsValid())
    {
        OutError = TEXT("RBFoundation plugin metadata is unavailable.");
        return false;
    }
    const FString ManifestPath = FPaths::Combine(FoundationPlugin->GetBaseDir(), TEXT("StackManifest.json"));
    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *ManifestPath))
    {
        OutError = FString::Printf(TEXT("Stack manifest not found: %s"), *ManifestPath);
        return false;
    }

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutError = TEXT("Stack manifest is not valid JSON.");
        return false;
    }
    int32 ManifestAdapterApi = 0;
    if (!Root->TryGetNumberField(TEXT("adapter_api"), ManifestAdapterApi)
        || ManifestAdapterApi != GetSupportedAdapterApiVersion())
    {
        OutError = TEXT("Stack manifest adapter API is unsupported.");
        return false;
    }
    FString EngineRange;
    const TSharedPtr<FJsonObject>* FoundationObject = nullptr;
    if (Root->TryGetObjectField(TEXT("foundation"), FoundationObject) && FoundationObject && FoundationObject->IsValid())
        (*FoundationObject)->TryGetStringField(TEXT("engine_range"), EngineRange);

    const TArray<TSharedPtr<FJsonValue>>* Products = nullptr;
    if (!Root->TryGetArrayField(TEXT("products"), Products))
    {
        OutError = TEXT("Stack manifest is missing products array.");
        return false;
    }

    for (const TSharedPtr<FJsonValue>& ProductValue : *Products)
    {
        const TSharedPtr<FJsonObject>* ProductObjectPtr = nullptr;
        if (!ProductValue.IsValid() || !ProductValue->TryGetObject(ProductObjectPtr)
            || !ProductObjectPtr || !ProductObjectPtr->IsValid())
        {
            OutError = TEXT("Stack manifest contains invalid product entry.");
            return false;
        }
        const TSharedPtr<FJsonObject>& Product = *ProductObjectPtr;
        FString AdapterMode;
        Product->TryGetStringField(TEXT("runtime_adapter"), AdapterMode);
        if (!AdapterMode.Equals(TEXT("declarative"), ESearchCase::IgnoreCase)) continue;

        FString PluginName;
        Product->TryGetStringField(TEXT("plugin"), PluginName);
        const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(PluginName);
        bool bRuntimeRequired = false;
        Product->TryGetBoolField(TEXT("runtime_required"), bRuntimeRequired);
        if (!Plugin.IsValid() || !Plugin->IsEnabled())
        {
            if (bRuntimeRequired)
            {
                OutError = FString::Printf(TEXT("Required declarative plugin '%s' is not enabled."), *PluginName);
                return false;
            }
            continue;
        }

        FRBFoundationProductDescriptor Descriptor;
        Descriptor.AdapterApiVersion = ManifestAdapterApi;
        double ProductAdapterApi = 0.0;
        if (Product->TryGetNumberField(TEXT("adapter_api"), ProductAdapterApi))
            Descriptor.AdapterApiVersion = static_cast<int32>(ProductAdapterApi);
        FString Text;
        if (Product->TryGetStringField(TEXT("id"), Text)) Descriptor.ProductId = FName(*Text);
        Descriptor.ProductName = Plugin.IsValid() && !Plugin->GetDescriptor().FriendlyName.IsEmpty()
            ? Plugin->GetDescriptor().FriendlyName
            : Text;
        Product->TryGetStringField(TEXT("version"), Descriptor.SemanticVersion);
        Descriptor.PluginName = PluginName;
        Descriptor.EngineCompatibility = EngineRange;

        FString Tag;
        FString Commit;
        Product->TryGetStringField(TEXT("tag"), Tag);
        Product->TryGetStringField(TEXT("commit"), Commit);
        Descriptor.ReleaseIdentity = Tag.IsEmpty() ? Commit : Tag + TEXT("@") + Commit;
        Product->TryGetStringField(TEXT("payload_sha256"), Descriptor.BuildIdentity);
        if (!ReadNameArray(Product, TEXT("authorities"), Descriptor.AuthorityDomains, OutError)) return false;
        const TArray<TSharedPtr<FJsonValue>>* CapabilityValues = nullptr;
        if (Product->TryGetArrayField(TEXT("capabilities"), CapabilityValues))
        {
            for (const TSharedPtr<FJsonValue>& CapabilityValue : *CapabilityValues)
            {
                const TSharedPtr<FJsonObject>* CapabilityObjectPtr = nullptr;
                if (!CapabilityValue->TryGetObject(CapabilityObjectPtr) || !CapabilityObjectPtr || !CapabilityObjectPtr->IsValid())
                {
                    OutError = TEXT("Manifest capability entry is invalid.");
                    return false;
                }
                FRBFoundationCapabilityDeclaration Capability;
                FString CapabilityId;
                FString Domain;
                (*CapabilityObjectPtr)->TryGetStringField(TEXT("id"), CapabilityId);
                (*CapabilityObjectPtr)->TryGetStringField(TEXT("domain"), Domain);
                Capability.Capability = FName(*CapabilityId);
                Capability.AuthorityDomain = FName(*Domain);
                bool bExclusive = true;
                (*CapabilityObjectPtr)->TryGetBoolField(TEXT("exclusive"), bExclusive);
                Capability.bExclusive = bExclusive;
                Descriptor.Capabilities.Add(MoveTemp(Capability));
            }
        }

        const TArray<TSharedPtr<FJsonValue>>* DependencyValues = nullptr;
        if (Product->TryGetArrayField(TEXT("dependencies"), DependencyValues))
        {
            for (const TSharedPtr<FJsonValue>& DependencyValue : *DependencyValues)
            {
                const TSharedPtr<FJsonObject>* DependencyObjectPtr = nullptr;
                if (!DependencyValue->TryGetObject(DependencyObjectPtr) || !DependencyObjectPtr || !DependencyObjectPtr->IsValid())
                {
                    OutError = TEXT("Manifest dependency entry is invalid.");
                    return false;
                }
                FRBFoundationDependencyDeclaration Dependency;
                FString ProductId;
                (*DependencyObjectPtr)->TryGetStringField(TEXT("product"), ProductId);
                Dependency.ProductId = FName(*ProductId);
                (*DependencyObjectPtr)->TryGetStringField(TEXT("version"), Dependency.VersionRange);
                (*DependencyObjectPtr)->TryGetBoolField(TEXT("optional"), Dependency.bOptional);
                Descriptor.Dependencies.Add(MoveTemp(Dependency));
            }
        }

        const TSharedPtr<FJsonObject>* EventsObject = nullptr;
        if (Product->TryGetObjectField(TEXT("events"), EventsObject) && EventsObject && EventsObject->IsValid())
        {
            if (!ReadNameArray(*EventsObject, TEXT("emits"), Descriptor.EmitsEvents, OutError)) return false;
            if (!ReadNameArray(*EventsObject, TEXT("consumes"), Descriptor.ConsumesEvents, OutError)) return false;
        }
        if (!ReadNameArray(Product, TEXT("persistence_domains"), Descriptor.PersistenceDomains, OutError)) return false;
        Descriptor.bParticipatesInPersistence = !Descriptor.PersistenceDomains.IsEmpty();

        FString RegisterError;
        if (!RegisterDeclarativeProduct(Descriptor, RegisterError))
        {
            OutError = FString::Printf(TEXT("Declarative product '%s' failed to register: %s"),
                *Descriptor.ProductId.ToString(), *RegisterError);
            return false;
        }
    }
    return true;
}

