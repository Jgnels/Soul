from pathlib import Path
R=Path.cwd()
def edit(name,changes):
 p=R/name;s=p.read_text()
 for old,new in changes:
  assert old in s,(name,old[:100]);s=s.replace(old,new,1)
 p.write_text(s)
branch='''        if(Composition())
        {
            if(FParse::Param(FCommandLine::Get(),TEXT("SoulWorldTerrain")) || FParse::Param(FCommandLine::Get(),TEXT("SoulCampaignExpansion")))
            {UE_LOG(LogTemp,Error,TEXT("SOUL_COMPOSITION_FAIL conflicting experimental profile"));return;}
            Resolution=2041;HeightUnit=100.f/128.f;Minimum=-175000;Extent=350000;
            MinimumXY=FVector2D(Minimum,Minimum);ExtentXY=FVector2D(Extent,Extent);
            Package=TEXT("/Game/SoulCampaignComposition/L_Composition_3500_r2");
            if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectDir()/TEXT("Data/CampaignComposition/presentation.json")))
                ||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root)
                ||!FFileHelper::LoadFileToArray(Heights,*(FPaths::ProjectDir()/TEXT("Data/CampaignCompositionLocal/Composition_3500_r2.r16")))
                ||Heights.Num()!=static_cast<int64>(Resolution)*Resolution*2)return;
            for(const auto& Entry:Root->GetObjectField(TEXT("regions"))->Values)
            {
                const auto& P=Entry.Value->AsArray();if(P.Num()!=3)return;
                const FName Id(*Entry.Key);const FVector V(P[0]->AsNumber(),P[1]->AsNumber(),P[2]->AsNumber());
                Places.Add(Id,V);AllPlaces.Add(Id,V);
            }
            for(const auto& Entry:Root->GetObjectField(TEXT("display_names"))->Values)Names.Add(FName(*Entry.Key),Entry.Value->AsString());
            for(const auto& Value:Root->GetArrayField(TEXT("routes")))
            {
                const auto O=Value->AsObject();FString A=O->GetStringField(TEXT("a")),B=O->GetStringField(TEXT("b"));
                TArray<FVector> Points;TArray<FVector2D> Ferries;
                for(const auto& Item:O->GetArrayField(TEXT("points")))
                {const auto& P=Item->AsArray();if(P.Num()!=3)return;Points.Add(FVector(P[0]->AsNumber(),P[1]->AsNumber(),P[2]->AsNumber()));}
                if(Points.Num()<2 || !Places.Contains(FName(*A)) || !Places.Contains(FName(*B)))return;
                double Length=0;for(int32 I=1;I<Points.Num();++I)Length+=FVector::Dist2D(Points[I-1],Points[I]);
                if(Length<=0)return;
                for(const auto& Item:O->GetArrayField(TEXT("ferries")))
                {const auto F=Item->AsObject();double Start=F->GetNumberField(TEXT("start_cm")),End=F->GetNumberField(TEXT("end_cm"));
                 if(Start<0||End<Start||End>Length+1)return;Ferries.Add(A>B?FVector2D(Length-End,Length-Start):FVector2D(Start,End));}
                if(A>B){Swap(A,B);Algo::Reverse(Points);}
                const FString Key=A+TEXT("|")+B;if(Routes.Contains(Key))return;
                TArray<double> Arc;Arc.Add(0);for(int32 I=1;I<Points.Num();++I)Arc.Add(Arc.Last()+FVector::Dist2D(Points[I-1],Points[I]));
                RouteArcs.Add(Key,MoveTemp(Arc));FerryRanges.Add(Key,MoveTemp(Ferries));Routes.Add(Key,MoveTemp(Points));Edges.Add({FName(*A),FName(*B)});
            }
            FString CanonicalText;TSharedPtr<FJsonObject> Canonical;
            if(!FFileHelper::LoadFileToString(CanonicalText,*(FPaths::ProjectDir()/TEXT("Data/soul_world_overmap_v1_20260922.json")))
                ||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(CanonicalText),Canonical))return;
            for(const auto& V:Canonical->GetArrayField(TEXT("nodes")))if(!Places.Contains(FName(*V->AsObject()->GetStringField(TEXT("id")))))return;
            for(const auto& V:Canonical->GetArrayField(TEXT("edges")))
            {FString A=V->AsObject()->GetStringField(TEXT("a")),B=V->AsObject()->GetStringField(TEXT("b"));if(A>B)Swap(A,B);if(!Routes.Contains(A+TEXT("|")+B))return;}
            bValid=Places.Num()==36&&Routes.Num()==51;
            UE_LOG(LogTemp,Display,TEXT("SOUL_COMPOSITION_PROFILE valid=%d regions=%d routes=%d world_experiment=0 expansion_experiment=0"),bValid,Places.Num(),Routes.Num());
            return;
        }
'''
edit('Source/Soul/Private/SoulCampaignTerrain.cpp',[
 ('#include "Algo/Reverse.h"','#include "Algo/Reverse.h"\n#include "Algo/UpperBound.h"\n#include "LandscapeProxy.h"\n#include "EngineUtils.h"'),
 ('    TArray<TPair<FName,FName>> Edges;','    TArray<TPair<FName,FName>> Edges;\n    TMap<FString,TArray<double>> RouteArcs;\n    TMap<FString,TArray<FVector2D>> FerryRanges;'),
 ('        FString Text;TSharedPtr<FJsonObject> Root;','        FString Text;TSharedPtr<FJsonObject> Root;\n'+branch),
 ('bool World(){return FParse::Param','bool Composition(){return FParse::Param(FCommandLine::Get(),TEXT("SoulComposition"));}\nbool World(){return !Composition()&&FParse::Param'),
 ('bool EvilCorridor(){return !World()','bool EvilCorridor(){return !Composition()&&!World()'),
 ('if(EvilCorridor())if(const auto* N=Bake().Names.Find(Id))','if(EvilCorridor()||Composition())if(const auto* N=Bake().Names.Find(Id))'),
 ('bool Mesa(){return !World()','bool Mesa(){return !Composition()&&!World()'),
 ('bool Enabled(){return World()','bool Enabled(){return Composition()||World()'),
 ('float Scale(){return World()','float Scale(){return Composition()||World()'),
 ('float RegionScale(){return World()','float RegionScale(){return Composition()||World()'),
 ('FVector2D FocusBounds(){return World()?','FVector2D FocusBounds(){return (Composition()||World())?'),
 ('    const float T=FMath::Clamp(Alpha,0.f,1.f)*(P->Num()-1);','    if(Composition())\n    {\n        const auto* Arc=Bake().RouteArcs.Find(A+TEXT("|")+B);if(!Arc||Arc->Num()!=P->Num())return FVector::ZeroVector;\n        const double Distance=FMath::Clamp(Alpha,0.f,1.f)*Arc->Last();const int32 I=FMath::Clamp(Algo::UpperBound(*Arc,Distance)-1,0,P->Num()-2);\n        return FMath::Lerp((*P)[I],(*P)[I+1],FMath::Clamp((Distance-(*Arc)[I])/FMath::Max((*Arc)[I+1]-(*Arc)[I],.001),0.,1.));\n    }\n    const float T=FMath::Clamp(Alpha,0.f,1.f)*(P->Num()-1);'),
 ('float RoadSurface(float X,float Y)\n{','bool FerryTravel(FName From,FName To,float Alpha)\n{\n    if(!Composition())return false;FString A=From.ToString(),B=To.ToString();if(A>B){Swap(A,B);Alpha=1-Alpha;}\n    const FString Key=A+TEXT("|")+B;const auto* Arc=Bake().RouteArcs.Find(Key);const auto* Ranges=Bake().FerryRanges.Find(Key);\n    if(!Arc||Arc->IsEmpty()||!Ranges)return false;const double Distance=FMath::Clamp(Alpha,0.f,1.f)*Arc->Last();\n    for(const auto& Range:*Ranges)if(Distance>Range.X&&Distance<Range.Y)return true;return false;\n}\nfloat RoadSurface(float X,float Y)\n{'),
 ('void DressRoad(AActor* Owner,USceneComponent* RoadComponent,FName From,FName To)\n{','void DressRoad(AActor* Owner,USceneComponent* RoadComponent,FName From,FName To)\n{\n    if(Composition())return;'),
 ('    // Remove study-only actors from this transient instance, never save the package.','''    if(Composition())
    {
        // The candidate already contains reviewed scenery, roads, water and lighting.
        // Remove only transient review actors; never mutate its saved package.
        for(AActor* Actor:TArray<TObjectPtr<AActor>>(Level->GetLoadedLevel()->Actors))
            if(Actor&&(Actor->IsA<ACameraActor>()||Actor->ActorHasTag(TEXT("SoulCompositionReference"))))Actor->Destroy();
        FCollisionQueryParams Query;for(TActorIterator<AActor> It(Owner->GetWorld());It;++It)if(!It->IsA<ALandscapeProxy>())Query.AddIgnoredActor(*It);
        int32 Hits=0,Probes=0;double Error=0;
        for(int32 Y=1;Y<10;++Y)for(int32 X=1;X<10;++X)
        {
            const double PX=-175000+X*35000,PY=-175000+Y*35000;FHitResult Hit;++Probes;
            if(Owner->GetWorld()->LineTraceSingleByChannel(Hit,FVector(PX+.1,PY+.1,100000),FVector(PX+.1,PY+.1,-100000),ECC_Visibility,Query))
            {++Hits;Error=FMath::Max(Error,FMath::Abs(Hit.ImpactPoint.Z-Height(PX+.1,PY+.1)));}
        }
        UE_LOG(LogTemp,Display,TEXT("SOUL_COMPOSITION_LOADED map=%s hits=%d/%d max_error_cm=%.4f baked_roads=1"),Package,Hits,Probes,Error);
        if(Hits!=Probes||Error>5)FPlatformMisc::RequestExitWithStatus(false,1);
        return;
    }
    // Remove study-only actors from this transient instance, never save the package.'''),
 ('bool DressRegion(AActor* Owner,FName Id)\n{','bool DressRegion(AActor* Owner,FName Id)\n{\n    if(Composition())return true;'),
 ('void DressSettlementSurroundings(AActor* Owner,FName Id)\n{','void DressSettlementSurroundings(AActor* Owner,FName Id)\n{\n    if(Composition())return;')])
edit('Source/Soul/Public/SoulCampaignTerrain.h', [('    bool Enabled();','    bool Enabled();\n    bool Composition();\n    bool FerryTravel(FName From,FName To,float Alpha);')])
print('TERRAIN_ADAPTER_PATCHED')
