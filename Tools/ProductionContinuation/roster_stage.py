"""Append only fresh cooked Viking axe packages to a new local loose stage.
The base registry/shader libraries remain untouched; this is not a distribution cook.
"""
from pathlib import Path
import hashlib,shutil,os,json
PACKAGES=['Materials/M_Ulf_Axe','Mesh/SM_Viking_Axe','Textures/Axe/T_Axe_Ulf_Albedo','Textures/Axe/T_Axe_Ulf_Ao','Textures/Axe/T_Axe_Ulf_Metallic','Textures/Axe/T_Axe_Ulf_Normals']
PREFIX=Path('Soul/Content/Fantasy_Pack/Characters/Viking_Ulf')
def append_viking_cook(cooked,stage,manifest):
 cooked=cooked.resolve();stage=stage.resolve();rows=[]
 for name in PACKAGES:
  package=PREFIX/name
  assert (cooked/package.with_suffix('.uasset')).is_file(),str(package)
  for ext in ['.uasset','.uexp','.ubulk','.uptnl']:
   rel=package.with_suffix(ext);source=cooked/rel;dest=stage/rel
   if not source.is_file():continue
   assert source.resolve().is_relative_to(cooked) and dest.resolve().is_relative_to(stage)
   assert not dest.exists(),'Never overwrite a verified base cooked package: '+str(rel)
   dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,dest);os.utime(dest,None)
   digest=hashlib.sha256(source.read_bytes()).hexdigest();assert hashlib.sha256(dest.read_bytes()).hexdigest()==digest
   rows.append({'relative':rel.as_posix(),'sha256':digest,'bytes':dest.stat().st_size})
 # Keep UAT's original manifest intact; identify this bounded addition separately.
 (manifest.parent.parent/'SupplementalCookManifest.json').write_text(json.dumps(rows,indent=2))
 return rows
