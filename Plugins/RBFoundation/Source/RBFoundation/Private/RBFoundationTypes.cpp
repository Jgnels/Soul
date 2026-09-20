#include "RBFoundationTypes.h"

bool FRBFoundationTimeState::IsValid(FString* OutError) const
{
    auto Fail = [OutError](const TCHAR* Message)
    {
        if (OutError) *OutError = Message;
        return false;
    };

    if (GameSeconds < 0) return Fail(TEXT("GameSeconds must be non-negative."));
    if (DayIndex < 0) return Fail(TEXT("DayIndex must be non-negative."));
    if (MinuteOfDay < 0 || MinuteOfDay >= 1440)
        return Fail(TEXT("MinuteOfDay must be in [0, 1439]."));
    if (!FMath::IsFinite(HourOfDay) || HourOfDay < 0.0 || HourOfDay >= 24.0)
        return Fail(TEXT("HourOfDay must be finite and in [0, 24)."));
    if (Season.IsNone()) return Fail(TEXT("Season must be set."));

    const double MinutesFromHour = HourOfDay * 60.0;
    if (FMath::Abs(MinutesFromHour - static_cast<double>(MinuteOfDay)) > 1.0)
        return Fail(TEXT("HourOfDay and MinuteOfDay disagree by more than one minute."));
    return true;
}

bool FRBFoundationProductDescriptor::IsValid(FString* OutError) const
{
    auto Fail = [OutError](const FString& Message)
    {
        if (OutError) *OutError = Message;
        return false;
    };
    if (AdapterApiVersion <= 0) return Fail(TEXT("AdapterApiVersion must be positive."));
    if (ProductId.IsNone()) return Fail(TEXT("ProductId is required."));
    if (SemanticVersion.TrimStartAndEnd().IsEmpty()) return Fail(TEXT("SemanticVersion is required."));

    TSet<FName> Domains;
    for (FName Domain : AuthorityDomains)
    {
        if (Domain.IsNone() || Domains.Contains(Domain))
            return Fail(TEXT("Authority domains must be non-empty and unique."));
        Domains.Add(Domain);
    }
    TSet<FName> CapabilityIds;
    for (const FRBFoundationCapabilityDeclaration& Capability : Capabilities)
    {
        if (Capability.Capability.IsNone() || Capability.AuthorityDomain.IsNone())
            return Fail(TEXT("Capability declarations require capability and authority domain."));
        if (!Domains.Contains(Capability.AuthorityDomain))
            return Fail(FString::Printf(TEXT("Capability '%s' references undeclared authority '%s'."),
                *Capability.Capability.ToString(), *Capability.AuthorityDomain.ToString()));
        if (CapabilityIds.Contains(Capability.Capability))
            return Fail(FString::Printf(TEXT("Capability '%s' is declared more than once."), *Capability.Capability.ToString()));
        CapabilityIds.Add(Capability.Capability);
    }
    TSet<FName> DependencyIds;
    for (const FRBFoundationDependencyDeclaration& Dependency : Dependencies)
    {
        if (Dependency.ProductId.IsNone() || Dependency.ProductId == ProductId)
            return Fail(TEXT("Dependencies require a non-self ProductId."));
        if (Dependency.VersionRange.TrimStartAndEnd().IsEmpty())
            return Fail(TEXT("Dependency VersionRange cannot be empty."));
        if (DependencyIds.Contains(Dependency.ProductId))
            return Fail(FString::Printf(TEXT("Dependency '%s' is declared more than once."), *Dependency.ProductId.ToString()));
        DependencyIds.Add(Dependency.ProductId);
    }
    if (bParticipatesInPersistence && PersistenceDomains.IsEmpty())
        return Fail(TEXT("Persistence participants must declare at least one persistence domain."));
    for (FName Domain : PersistenceDomains) if (Domain.IsNone())
        return Fail(TEXT("Persistence domain cannot be None."));
    for (FName EventType : EmitsEvents) if (EventType.IsNone())
        return Fail(TEXT("Emitted event type cannot be None."));
    for (FName EventType : ConsumesEvents) if (EventType.IsNone())
        return Fail(TEXT("Consumed event type cannot be None."));
    return true;
}
