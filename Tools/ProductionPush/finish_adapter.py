from pathlib import Path
import json,sys,runpy
R=Path.cwd();E=R/'Evidence/ProductionPush-20261008'
sys.path.insert(0,str(R/'Tools/MapPolish'));import route_surface
route_surface.OUT=E
runpy.run_path(str(R/'Tools/MapPolish/summarize_native.py'),run_name='__main__')
p=R/'Tools/ProductionPush/export_runtime.py';t=p.read_text();needle="profile['source_routes_sha256']="
insert="actors=json.loads((E/'candidate-scale-actors.json').read_text());human=next(a for a in actors if a['label']=='Composition_Scale_human_capital')\nprofile['miniatures']={'human_capital':{k:human[k] for k in ('location','rotation','scale')}}\n"
assert needle in t;t=t.replace(needle,insert+needle,1);p.write_text(t)
runpy.run_path(str(p),run_name='__main__')
p=R/'Source/Soul/Private/SoulCampaignTerrain.cpp';t=p.read_text();t=t.replace('    TMap<FName,FString> Names;','    TMap<FName,FString> Names;\n    TMap<FName,FTransform> Miniatures;',1)
needle='            for(const auto& Value:Root->GetArrayField(TEXT("routes")))'
insert='''            for(const auto& Entry:Root->GetObjectField(TEXT("miniatures"))->Values)
            {
                const auto O=Entry.Value->AsObject();
                const auto& L=O->GetArrayField(TEXT("location"));const auto& R=O->GetArrayField(TEXT("rotation"));const auto& S=O->GetArrayField(TEXT("scale"));
                if(L.Num()!=3||R.Num()!=3||S.Num()!=3)return;
                Miniatures.Add(FName(*Entry.Key),FTransform(FRotator(R[0]->AsNumber(),R[1]->AsNumber(),R[2]->AsNumber()),FVector(L[0]->AsNumber(),L[1]->AsNumber(),L[2]->AsNumber()),FVector(S[0]->AsNumber(),S[1]->AsNumber(),S[2]->AsNumber())));
            }
'''
assert needle in t;t=t.replace(needle,insert+needle,1)
needle='bool FerryTravel(';idx=t.index(needle);t=t[:idx]+'''bool MiniaturePlacement(FName Region,FTransform& Out)
{
    if(!Composition())return false;
    if(const FTransform* T=Bake().Miniatures.Find(Region)){Out=*T;return true;}
    return false;
}
'''+t[idx:];p.write_text(t)
p=R/'Source/Soul/Public/SoulCampaignTerrain.h';t=p.read_text().replace('    bool FerryTravel(','    bool MiniaturePlacement(FName Region,FTransform& Out);\n    bool FerryTravel(',1);p.write_text(t)
p=R/'Source/Soul/Private/SoulPlaytestRegionActor.cpp';t=p.read_text().replace('                    Transform.AddToTranslation(Location);','                    Transform.AddToTranslation(Location);\n                    SoulCampaignTerrain::MiniaturePlacement(RegionId,Transform);',1);p.write_text(t)
p=R/'Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp';t=p.read_text().replace('if(bComposition&&(bExpansion||FParse::Param','if(bComposition&&(bDwarfEnvironmentProof||bExpansion||FParse::Param',1);p.write_text(t)
print('Stateful miniature placement patched')
