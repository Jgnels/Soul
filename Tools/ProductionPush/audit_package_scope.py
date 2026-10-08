from pathlib import Path
import json,re,subprocess
R=Path.cwd();E=R/'Evidence/ProductionPush-20261008';B=E/'Local/Inherited'
def baseline(n):
 p=B/n
 return p.read_text(encoding='utf-8-sig') if p.exists() else subprocess.check_output(['git','show','HEAD:'+n],text=True)
state=baseline('Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp');rules=baseline('Source/Soul/Soul.Build.cs')
required={'Data/'+n for n in re.findall(r'ReadData\(TEXT\("([^"]+)"\)',state)}
required.update(['Plugins/RBFoundation/StackManifest.json','Data/CampaignTerrainV2/presentation.json','Data/CampaignMesa/presentation.json','Data/CampaignEvilCorridor/presentation.json','Data/CampaignWorldTerrain/presentation.json','Data/CampaignWorldTerrain/settlement_surface.json'])
staged=set(re.findall(r'"((?:Data/|Plugins/RBFoundation/)[^"]+\.json)"',rules))
dwarf='/Game/Soul/CampaignProxies/Dwarven/SM_DwarfHold_Base_r9'
occ=[]
for p in B.rglob('*.cpp'):
 if dwarf in p.read_text(errors='replace'):occ.append(str(p.relative_to(B)))
occ += [n for n in ['Source/Soul/Private/SoulCampaignExpansion.cpp'] if ('?? '+n) in (E/'baseline-status.txt').read_text() and dwarf in (R/n).read_text()]
report=dict(baseline_dependency_missing=sorted(required-staged),baseline_dependency_extra=sorted(staged-required),inherited_r9_literal_in=occ,current_candidate_cook_root_deliberately_not_added=True,default_package_not_qualified=True,reason='No automatic promotion: editor/game module compilation and editor-game qualification only. Existing cook contract also lacks inherited Dwarf r9 and CampaignExpansion data; those drafts are not silently staged.')
(E/'packaging-scope.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
