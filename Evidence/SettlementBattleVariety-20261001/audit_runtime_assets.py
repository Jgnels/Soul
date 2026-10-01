import json, pathlib, unreal
assets = [
"/Game/AlienPlanet/Meshes/SM_BigTowerComplex", "/Game/AlienPlanet/Meshes/SM_BigBetweenTower",
"/Game/Fantasy_Pack/Characters/Orc_Hummer/Mesh/SK_Orc_Hummer", "/Game/Fantasy_Pack/Characters/Troll/Mesh/SK_Troll",
"/Game/Fantasy_Pack/Characters/Viking_Ulf/Mesh/SK_Ulf_Full", "/Game/Fantasy_Pack/Characters/Fantasy_Barbarian/Mesh/SK_Fantasy_Barbarian_Full",
"/Game/Fantasy_Pack/Characters/Barbarian/Mesh/SK_Barbarian_Full",
"/Game/Fantasy_Pack/Characters/Orc_Hummer/Animations/Anim_Orc_Hummer_Attack_1", "/Game/Fantasy_Pack/Characters/Orc_Hummer/Animations/Anim_Orc_Hummer_Dead",
"/Game/Fantasy_Pack/Characters/Orc_Hummer/Animations/Anim_Orc_Hummer_Idle", "/Game/Fantasy_Pack/Characters/Orc_Hummer/Animations/Anim_Orc_Hummer_Run",
"/Game/Fantasy_Pack/Characters/Troll/Animations/Anim_Troll_Attack_1", "/Game/Fantasy_Pack/Characters/Troll/Animations/Anim_Troll_Dead",
"/Game/Fantasy_Pack/Characters/Troll/Animations/Anim_Troll_Idle", "/Game/Fantasy_Pack/Characters/Troll/Animations/Anim_Troll_Run",
"/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_1", "/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Dead_1",
"/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Idle", "/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Run"]
rows=[]
for path in assets:
    obj=unreal.EditorAssetLibrary.load_asset(path)
    rows.append({"path":path,"loaded":obj is not None,"class":obj.get_class().get_name() if obj else None})
out=pathlib.Path(r"D:\RefinedBadger\Worktrees\Soul-bannerlord-campaign-map-20260929\Evidence\SettlementBattleVariety-20261001\asset-load-audit.json")
out.write_text(json.dumps({"all_loaded":all(x["loaded"] for x in rows),"assets":rows},indent=2),encoding="utf-8")
unreal.log("SOUL_ASSET_AUDIT all_loaded=%s count=%d" % (all(x["loaded"] for x in rows),len(rows)))
unreal.SystemLibrary.quit_editor()