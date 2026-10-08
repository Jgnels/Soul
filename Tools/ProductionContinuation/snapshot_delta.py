"""Preserve exact continuation changes separately from inherited worker drafts."""
from pathlib import Path
import json,hashlib,difflib,subprocess
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionContinuation-20261008';B=E/'Local/Inherited';out=[];patch=[]
paths=['Source/Soul/Private/SoulAuthoredSettlementQualification.cpp','Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp','Source/Soul/Private/SoulSettlementDevelopmentQualification.cpp','Source/Soul/Private/Tests/SoulCampaignWorldTests.cpp','Source/Soul/Public/SoulFounderPlaytestStateSubsystem.h','Source/Soul/Soul.Build.cs','Source/SoulComposition.Target.cs','Tools/build_soul_terrain.ps1','Tools/test_package_soul_weekend.py','Tools/package_soul_weekend.ps1','Tools/qualify_soul_vertical.py','Tools/test_qualify_soul_vertical.py','Tools/Stage-SoulTcatResources.ps1']
for n in paths:
 p=R/n;old=B/n
 if old.exists():base=old.read_bytes()
 else:
  x=subprocess.run(['git','show','8ea786c180f2f8e431831b92b7d8867e2237c818:'+n],cwd=R,capture_output=True);base=x.stdout if x.returncode==0 else b''
 current=p.read_bytes();before=base.decode('utf-8-sig').replace('\r\n','\n');after=current.decode('utf-8-sig').replace('\r\n','\n')
 if before==after:continue
 out.append({'path':n,'baseline_sha256':hashlib.sha256(base).hexdigest() if base else None,'current_sha256':hashlib.sha256(current).hexdigest(),'inherited_modified':old.exists()})
 patch.extend(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+n if base else '/dev/null',tofile='b/'+n))
(E/'source-delta-working.patch').write_text(''.join(patch),encoding='utf-8');(E/'source-delta-working.json').write_text(json.dumps(out,indent=2)+'\n',encoding='utf-8');print('Continuation source/tool deltas',len(out))
