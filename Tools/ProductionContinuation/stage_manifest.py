"""Require the recorded local stage resources before launching a qualified binary."""
from pathlib import Path

def verify_manifest_presence(stage_receipt: Path, stage: Path):
    stage=stage.resolve();count=0;missing=[]
    for kind in ['UFS','NonUFS']:
        manifest=stage_receipt.parent/'UAT'/('Manifest_'+kind+'Files_Win64.txt')
        if not manifest.is_file():raise ValueError('Missing UAT '+kind+' resource manifest')
        for line in manifest.read_text(encoding='utf-8-sig').splitlines():
            name=line.split('\t')[0];path=(stage/name).resolve()
            if not path.is_relative_to(stage):raise ValueError('Manifest path escapes the stage')
            count+=1
            if not path.is_file():missing.append(name)
    if missing:raise ValueError('Staged resources are missing: '+str(len(missing))+'; first: '+missing[0])
    if not count:raise ValueError('Empty stage manifests')
    return count
