from pathlib import Path
import json,hashlib,difflib,subprocess
R=Path.cwd();E=R/'Evidence/ProductionPush-20261008';B=E/'Local/Inherited';out=[];patch=[]
for p in R.joinpath('Source').rglob('*'):
 if not p.is_file() or p.suffix not in ['.cpp','.h','.cs']:continue
 n=p.relative_to(R).as_posix();old=B/n
 if old.exists():base=old.read_bytes()
 else:
  x=subprocess.run(['git','show','HEAD:'+n],capture_output=True)
  if x.returncode:
   if n!='Source/Soul/Private/SoulCompositionTraversalQualification.cpp':continue
   base=b''
  else:base=x.stdout
 current=p.read_bytes()
 if current.decode("utf-8-sig").replace("\r\n","\n")==base.decode("utf-8-sig").replace("\r\n","\n"):continue
 out.append({'path':n,'baseline_sha256':hashlib.sha256(base).hexdigest() if base else None,'current_sha256':hashlib.sha256(current).hexdigest(),'inherited_modified':old.exists()})
 patch.extend(difflib.unified_diff(base.decode('utf-8-sig').replace('\r\n','\n').splitlines(True),current.decode('utf-8-sig').replace('\r\n','\n').splitlines(True),fromfile='a/'+n if base else '/dev/null',tofile='b/'+n))
(E/'source-delta-working.patch').write_text(''.join(patch));(E/'source-delta-working.json').write_text(json.dumps(out,indent=2));print('Changed from mission baseline',len(out))
