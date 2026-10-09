"""Append only explicitly admitted fresh Viking/Nature packages to a new local loose stage.
The base registry/shader libraries remain untouched; this is not a distribution cook.
"""
from pathlib import Path
import hashlib,shutil,os,json
PACKAGES=['Materials/M_Ulf_Axe','Mesh/SM_Viking_Axe','Textures/Axe/T_Axe_Ulf_Albedo','Textures/Axe/T_Axe_Ulf_Ao','Textures/Axe/T_Axe_Ulf_Metallic','Textures/Axe/T_Axe_Ulf_Normals']
PREFIX=Path('Soul/Content/Fantasy_Pack/Characters/Viking_Ulf')
NATURE_ROOTS = ['/Game/Animals_Warrior_Pack/Mesh/Bear/SK_Bear_Full', '/Game/Animals_Warrior_Pack/Mesh/Warrior_02/SM_Warrior_02_Axe', '/Game/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Idle', '/Game/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Idle_Sit', '/Game/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Run', '/Game/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Run_Back', '/Game/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Run_Left', '/Game/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Run_Right', '/Game/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Get_Hit_1', '/Game/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Dead_1', '/Game/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_1', '/Game/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_2', '/Game/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_3', '/Game/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_4']
NATURE_PACKAGES = ['Soul/Content/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_1', 'Soul/Content/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_2', 'Soul/Content/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_3', 'Soul/Content/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_4', 'Soul/Content/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Dead_1', 'Soul/Content/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Get_Hit_1', 'Soul/Content/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Idle', 'Soul/Content/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Idle_Sit', 'Soul/Content/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Run', 'Soul/Content/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Run_Back', 'Soul/Content/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Run_Left', 'Soul/Content/Animals_Warrior_Pack/Animations/1With_Weapon/Anim_Warrior_Run_Right', 'Soul/Content/Animals_Warrior_Pack/Materials/Bear/M_Bear_Armors', 'Soul/Content/Animals_Warrior_Pack/Materials/Bear/M_Bear_Body', 'Soul/Content/Animals_Warrior_Pack/Materials/Bear/M_Bear_Fur', 'Soul/Content/Animals_Warrior_Pack/Materials/Warrior_02/M_Warrior_2_Sword', 'Soul/Content/Animals_Warrior_Pack/Mesh/Bear/SK_Bear_Full', 'Soul/Content/Animals_Warrior_Pack/Mesh/Bear/SK_Bear_Full_PhysicsAsset', 'Soul/Content/Animals_Warrior_Pack/Mesh/Mannequin/SK_Mannequin_Skeleton', 'Soul/Content/Animals_Warrior_Pack/Mesh/Warrior_02/SM_Warrior_02_Axe', 'Soul/Content/Animals_Warrior_Pack/Textures/Bear/Armors/T_Bear_Armors_Albedo', 'Soul/Content/Animals_Warrior_Pack/Textures/Bear/Armors/T_Bear_Armors_Ao', 'Soul/Content/Animals_Warrior_Pack/Textures/Bear/Armors/T_Bear_Armors_Metallic', 'Soul/Content/Animals_Warrior_Pack/Textures/Bear/Armors/T_Bear_Armors_Normals', 'Soul/Content/Animals_Warrior_Pack/Textures/Bear/Body/T_Bear_Body_Albedo', 'Soul/Content/Animals_Warrior_Pack/Textures/Bear/Body/T_Bear_Body_Ao', 'Soul/Content/Animals_Warrior_Pack/Textures/Bear/Body/T_Bear_Body_Metallic', 'Soul/Content/Animals_Warrior_Pack/Textures/Bear/Body/T_Bear_Body_Normals', 'Soul/Content/Animals_Warrior_Pack/Textures/Bear/Fur/T_Bear_Fur_Albedo', 'Soul/Content/Animals_Warrior_Pack/Textures/Bear/Fur/T_Bear_Fur_Ao', 'Soul/Content/Animals_Warrior_Pack/Textures/Bear/Fur/T_Bear_Fur_Metallic', 'Soul/Content/Animals_Warrior_Pack/Textures/Bear/Fur/T_Bear_Fur_Normals', 'Soul/Content/Animals_Warrior_Pack/Textures/Warrior_02/Sword/T_Sword_Albedo', 'Soul/Content/Animals_Warrior_Pack/Textures/Warrior_02/Sword/T_Sword_Ao', 'Soul/Content/Animals_Warrior_Pack/Textures/Warrior_02/Sword/T_Sword_Metallic', 'Soul/Content/Animals_Warrior_Pack/Textures/Warrior_02/Sword/T_Sword_Normals']
FACTION_ROOTS = ['/Game/Fantasy_Pack/Characters/Viking_Ulf/Mesh/SM_Viking_Axe'] + NATURE_ROOTS

def append_viking_cook(cooked,stage,manifest):
 return _append_packages(cooked,stage,manifest,[PREFIX/name for name in PACKAGES])

def append_faction_cook(cooked,stage,manifest):
 return _append_packages(cooked,stage,manifest,[PREFIX/name for name in PACKAGES]+[Path(n) for n in NATURE_PACKAGES])

def _append_packages(cooked,stage,manifest,packages):
 cooked=cooked.resolve();stage=stage.resolve();rows=[]
 for package in packages:
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
