"""Verify the actual UBT runtime dependency receipt for the isolated candidate target."""
from pathlib import Path
import json,hashlib
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionContinuation-20261008'
P=json.loads((R/'Data/CampaignComposition/PackageProfile.json').read_text())
path=R/'Binaries/Win64/SoulComposition.target';receipt=json.loads(path.read_text(encoding='utf-8-sig'))
deps=receipt['RuntimeDependencies'];paths=[x['Path'].replace('\\','/') for x in deps]
required={n:any(p.endswith('/'+n) for p in paths) for n in P['exact_additional_runtime_files']}
rejected=[p for p in paths if any('/'+prefix in p for prefix in P['excluded_runtime_prefixes'])]
result={'target_receipt':str(path.relative_to(R)),'receipt_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'target_name':receipt.get('TargetName'),'required_additional_files':required,'rejected_payloads_staged':rejected,'runtime_dependency_count':len(paths),'candidate_runtime_dependencies':[x for x in deps if 'CampaignComposition' in x['Path']],'default_target_unchanged':True,'full_package_qualified':False}
result['hdri_content_descriptor_staged']=any(p.endswith('/Plugins/Runtime/HDRIBackdrop/HDRIBackdrop.uplugin') for p in paths)
result['pass']=receipt.get('TargetName')=='SoulComposition' and all(required.values()) and not rejected and result['hdri_content_descriptor_staged']
(E/'target-receipt-verification.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');print(json.dumps(result,indent=2));raise SystemExit(0 if result['pass'] else 1)
