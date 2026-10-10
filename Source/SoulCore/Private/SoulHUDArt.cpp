#include "SoulHUDArt.h"
#include "ImageUtils.h"
#include "Engine/Texture2D.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
bool SoulHUDArt::Enabled(){return FParse::Param(FCommandLine::Get(),TEXT("SoulHeartland"));}
UTexture2D* SoulHUDArt::Texture(const TCHAR* Name)
{
    static TMap<FString,UTexture2D*> Cache;
    const FString Key(Name);
    if(auto* Found=Cache.Find(Key))return *Found;
    // Only project-specified basenames, never user-controlled paths.
    if(Key.Contains(TEXT("/"))||Key.Contains(TEXT("\\"))||Key.Contains(TEXT("..")))return nullptr;
    UTexture2D* T=FImageUtils::ImportFileAsTexture2D(FPaths::ProjectDir()/TEXT("Data/UI/Kenney")/(Key+TEXT(".png")));
    if(T){T->AddToRoot();T->Filter=TF_Nearest;T->NeverStream=true;T->UpdateResource();}
    else UE_LOG(LogTemp,Warning,TEXT("SOUL_UI_ART_MISSING %s"),*Key);
    Cache.Add(Key,T);return T;
}
