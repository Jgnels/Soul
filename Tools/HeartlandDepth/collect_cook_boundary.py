"""Explicit additive cook boundary against an already verified local loose stage.
Not a standalone/distribution cook: stop at packages already present in the base.
Never admit unrelated experimental packages or rewrite shared shader libraries.
"""
import unreal as u,json,hashlib,re
from pathlib import Path
r=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));e=r/'Evidence/HumanHeartlandDepth-20261010'
prior=r/'Evidence/HumanHeartland-20261009/Local/stage-heartland-r3/Diagnostics/receipt.json';base=json.loads(prior.read_text());stage=Path(base['stage'])/'Windows'
roots=['/Game/Soul/Maps/Settlements/SL_HumanCapital_Houses','/Game/Soul/Maps/Battles/L_Heartland_Woodland','/Game/Soul/Maps/Battles/L_Heartland_RiverBridge']
roots += ['/Game/Soul/CampaignProxies/Heartland/SM_HumanCapital_'+n+'_Depth_r1' for n in ('Base','ArcaneHall','Barracks','Market')]
roots += re.findall(r'TEXT\("(/Game/[^" ]+)"\)',(r/'Source/Soul/Private/SoulHeartlandSites.cpp').read_text())
roots += ['/Game/Knights_Pack/Meshes/Knight_04/Mesh_UE4/Full_Mesh/SK_Knight_04_Full_01','/Game/Knights_Pack/Demoscene_UE4/Animations/ThirdPersonIdle','/Game/ParagonSparrow/Characters/Heroes/Sparrow/Meshes/Sparrow']
reg=u.AssetRegistryHelpers.get_asset_registry();reg.wait_for_completion()
opts=u.AssetRegistryDependencyOptions(include_soft_package_references=True,include_hard_package_references=True,include_searchable_names=False,include_soft_management_references=False,include_hard_management_references=False)
forced={p for p in roots if p.startswith("/Game/Soul/")};todo=list(roots);seen=set();cook=[];existing=[];missing=[];non_game=set()
def digest(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
while todo:
 p=todo.pop()
 if p in seen:continue
 seen.add(p)
 if not p.startswith('/Game/'):
  if not p.startswith('/Script/'):non_game.add(p)
  continue
 assert not any(x in p for x in ('SoulCampaignWorld','CampaignExpansion','CampaignWorldTerrain'))
 tail=p[6:];staged=next((stage/'Soul/Content'/(tail+ext) for ext in ('.uasset','.umap') if (stage/'Soul/Content'/(tail+ext)).is_file()),None)
 if staged and p not in forced:
  existing.append({'package':p,'relative':str(staged.relative_to(stage)).replace('\\','/'),'sha256':digest(staged)});continue
 source=next((r/'Content'/(tail+ext) for ext in ('.uasset','.umap') if (r/'Content'/(tail+ext)).is_file()),None)
 if not source:missing.append(p);continue
 cook.append({'package':p,'source':str(source.relative_to(r)).replace('\\','/'),'source_sha256':digest(source),'source_bytes':source.stat().st_size,'replaces_base':bool(staged)})
 todo += [str(x) for x in reg.get_dependencies(p,opts)]
result={'roots':sorted(set(roots)),'cook_packages':sorted(cook,key=lambda x:x['package']),'base_dependencies':existing,'non_game_references':sorted(non_game),'missing_source':missing,'base_receipt':str(prior),'base_receipt_sha256':digest(prior),'base_binary_sha256':base['binary_sha256'],'standalone_cook':False,'pass':not missing}
(e/'Local/depth-cook-boundary.json').write_text(json.dumps(result,indent=2))
print('DEPTH_COOK_BOUNDARY',len(cook),'packages',sum(x['source_bytes'] for x in cook),'source bytes',len(existing),'base packages',missing)
assert not missing
