"""Verify actual staged candidate data, isolation and configuration; no UE launch."""
from pathlib import Path
import argparse,hashlib,json,re
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionContinuation-20261008'
p=argparse.ArgumentParser();p.add_argument('--receipt',type=Path,required=True);p.add_argument('--evidence-root',type=Path,default=E);a=p.parse_args();E=a.evidence_root.resolve();assert E.is_relative_to((R/'Evidence').resolve());E.mkdir(parents=True,exist_ok=True)
assert a.receipt.resolve().is_relative_to((E/'Local').resolve())
s=json.loads(a.receipt.read_text(encoding='utf-8'));assert s['pass'];root=Path(s['stage'])/'Windows';project=root/'Soul'
profile=json.loads((R/'Data/CampaignComposition/PackageProfile.json').read_text());rows=[]
for name in profile['exact_additional_runtime_files']:
 source=R/name;dest=project/name
 rows.append({'path':name,'exists':dest.is_file(),'sha256':hashlib.sha256(dest.read_bytes()).hexdigest() if dest.exists() else None,'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest()})
for x in rows:assert x['exists'] and x['sha256']==x['source_sha256'],x['path']
for name in ['Data/soul_world_overmap_v1_20260922.json','Data/soul_campaign_start_states_v1_20260922.json']:
 assert (project/name).read_bytes()==(R/name).read_bytes(),name
rejected=[str(x.relative_to(project)).replace('\\','/') for x in project.rglob('*') if x.is_file() and any(v in str(x) for v in ['CampaignWorldTerrain','CampaignWorldLocal','CampaignExpansion','SoulCampaignWorld'])]
assert not rejected,rejected
hdri=root/'Engine/Plugins/Runtime/HDRIBackdrop/HDRIBackdrop.uplugin';hdri_source=Path('C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Runtime/HDRIBackdrop/HDRIBackdrop.uplugin')
assert hdri.is_file() and hdri.read_bytes()==hdri_source.read_bytes(),'Missing or modified cooked content-plugin descriptor'
forbidden_names={'.mcp.json','.env','AGENTS.md'}
unexpected_support=[str(x.relative_to(root)) for x in root.rglob('*') if x.is_file() and x.name in forbidden_names]
assert not unexpected_support
engine=project/'Config/DefaultEngine.ini';text=engine.read_text(encoding='utf-8-sig');assert re.search(r'^GameDefaultMap=/Engine/Maps/Entry',text,re.M)
section='';token_nonempty=False
for line in text.splitlines():
 if line.strip().startswith('['):section=line.strip()
 if section=='[/Script/AndroidFileServerEditor.AndroidFileServerRuntimeSettings]' and re.match(r'^\s*SecurityToken\s*=',line):token_nonempty=bool(line.split('=',1)[1].strip().strip(chr(34)))
assert not token_nonempty
r={'pass':True,'stage':str(root),'kind':s['stage_kind'],'additional_data':rows,'hdri_descriptor_present_and_source_identical':True,'canonical_graph_and_start_data_identical':True,'rejected_experimental_payloads':rejected,'unexpected_machine_support_files':unexpected_support,'default_map_is_entry':True,'machine_editor_security_token_absent_or_empty':True,'source_configuration_unchanged':s['configuration_unchanged'],'cooked_files_unchanged':s['source_cooked_bytes_preserved'],'automatic_promotion':False,'runtime_acceptance_not_inferred':True}
(E/'staged-isolation-verification.json').write_text(json.dumps(r,indent=2),encoding='utf-8');print('STAGED_ISOLATION_PASS',len(rows),'exact additional files; no experimental payloads or editor token')
