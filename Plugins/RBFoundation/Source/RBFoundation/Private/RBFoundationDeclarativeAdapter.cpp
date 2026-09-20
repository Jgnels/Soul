#include "RBFoundationDeclarativeAdapter.h"

#include "Interfaces/IPluginManager.h"

bool URBFoundationDeclarativeProductAdapter::Configure(
    const FRBFoundationProductDescriptor& InDescriptor, FString& OutError)
{
    if (!InDescriptor.IsValid(&OutError)) return false;
    Descriptor = InDescriptor;
    bConfigured = true;
    return true;
}

bool URBFoundationDeclarativeProductAdapter::GetFoundationProductDescriptor_Implementation(
    FRBFoundationProductDescriptor& OutDescriptor, FString& OutError) const
{
    if (!bConfigured)
    {
        OutError = TEXT("Declarative adapter has not been configured.");
        return false;
    }
    OutDescriptor = Descriptor;
    return true;
}
bool URBFoundationDeclarativeProductAdapter::FoundationAdapterStartup_Implementation(FString& OutError)
{
    if (!bConfigured)
    {
        OutError = TEXT("Declarative adapter has not been configured.");
        return false;
    }
    if (Descriptor.PluginName.IsEmpty()) return true;
    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(Descriptor.PluginName);
    if (!Plugin.IsValid())
    {
        OutError = FString::Printf(TEXT("Plugin '%s' is not installed."), *Descriptor.PluginName);
        return false;
    }
    const FString Actual = Plugin->GetDescriptor().VersionName;
    if (Actual != Descriptor.SemanticVersion)
    {
        OutError = FString::Printf(TEXT("Plugin '%s' version mismatch: expected %s, found %s."),
            *Descriptor.PluginName, *Descriptor.SemanticVersion, *Actual);
        return false;
    }
    return true;
}

void URBFoundationDeclarativeProductAdapter::FoundationAdapterShutdown_Implementation()
{
}
void URBFoundationDeclarativeProductAdapter::ProbeFoundationAdapterHealth_Implementation(
    TArray<FRBFoundationHealthIssue>& OutIssues) const
{
    if (!bConfigured)
    {
        FRBFoundationHealthIssue Issue;
        Issue.Code = TEXT("AdapterNotConfigured");
        Issue.Message = TEXT("Declarative product adapter has no descriptor.");
        Issue.bBlocking = true;
        OutIssues.Add(MoveTemp(Issue));
        return;
    }
    if (Descriptor.PluginName.IsEmpty()) return;
    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(Descriptor.PluginName);
    if (!Plugin.IsValid())
    {
        FRBFoundationHealthIssue Issue;
        Issue.Code = TEXT("PluginMissing");
        Issue.Message = FString::Printf(TEXT("Plugin '%s' is not installed."), *Descriptor.PluginName);
        Issue.OwnerDomain = Descriptor.ProductId;
        Issue.bBlocking = true;
        OutIssues.Add(MoveTemp(Issue));
        return;
    }
    const FString Actual = Plugin->GetDescriptor().VersionName;
    if (Actual != Descriptor.SemanticVersion)
    {
        FRBFoundationHealthIssue Issue;
        Issue.Code = TEXT("PluginVersionMismatch");
        Issue.Message = FString::Printf(TEXT("Plugin '%s' expected %s, found %s."),
            *Descriptor.PluginName, *Descriptor.SemanticVersion, *Actual);
        Issue.OwnerDomain = Descriptor.ProductId;
        Issue.bBlocking = true;
        OutIssues.Add(MoveTemp(Issue));
    }
}
