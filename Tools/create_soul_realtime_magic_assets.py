import json
import os
import traceback
import unreal

OUT = r"D:\RefinedBadger\Worktrees\Soul-realtime-battle-20260921\Evidence\soul_realtime_magic_assets.json"
SPELL_PATH = "/Game/Soul/Magic/Spells"
PRES_PATH = "/Game/Soul/Magic/Presentation"

spell_cls = unreal.RBMagicSpellDefinition
pres_cls = unreal.RBMagicPresentationProfile
target_enum = unreal.RBMagicTargetMode
delivery_enum = unreal.RBMagicDeliveryMode
effect_cls = unreal.RBMagicEffectSpec
cost_cls = unreal.RBMagicResourceCost

result = {"created": [], "updated": [], "errors": []}

def make_tag(name):
    tag = unreal.RBMagicLibrary.request_gameplay_tag(name, False)
    if tag.get_editor_property("tag_name") is None:
        raise RuntimeError("missing gameplay tag: " + name)
    return tag

def enum_value(cls, name):
    return getattr(cls, name.upper())

def object_path(path, name):
    return path + "/" + name + "." + name

def get_or_create(name, path, cls):
    existing = unreal.load_object(None, object_path(path, name))
    if existing:
        result["updated"].append(path + "/" + name)
        return existing
    factory = unreal.DataAssetFactory()
    try:
        factory.set_editor_property("data_asset_class", cls)
    except Exception:
        pass
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, path, cls, factory)
    if not asset:
        raise RuntimeError("create_asset returned None for " + name)
    result["created"].append(path + "/" + name)
    return asset

spells = [
    dict(name="DA_Soul_Firebolt", display="Firebolt",
         spell="Magic.Spell.Fire.Firebolt", school="Magic.School.Fire",
         target="UNIT", delivery="PROJECTILE", cost=8.0, range=2400.0,
         effects=[("Magic.Effect.Damage",35.0,0.0,0.0,1),
                  ("Magic.Effect.Burning",4.0,4.0,0.0,1)],
         system="/Game/MagicSpells/Fire/FX/NS_Fireball",
         slot="projectile_system"),

    dict(name="DA_Soul_ChainLightning", display="Chain Lightning",
         spell="Magic.Spell.Storm.ChainLightning", school="Magic.School.Storm",
         target="UNIT", delivery="CHAIN", cost=14.0, range=2200.0,
         effects=[("Magic.Effect.Damage",28.0,0.0,900.0,4)],
         system="/Game/MagicSpells/Electric/FX/NS_ChainLightning",
         slot="cast_system"),
    dict(name="DA_Soul_Blizzard", display="Blizzard",
         spell="Magic.Spell.Ice.Blizzard", school="Magic.School.Ice",
         target="GROUND", delivery="PERSISTENT_AREA", cost=18.0, range=2600.0,
         effects=[("Magic.Effect.DamagePeriodic",8.0,8.0,650.0,32),
                  ("Magic.Effect.Slow",0.35,8.0,650.0,32)],
         system="/Game/MagicSpells/Ice/FX/NS_Ice_Hailstorm",
         slot="persistent_system"),
    dict(name="DA_Soul_TidalWard", display="Tidal Ward",
         spell="Magic.Spell.Water.TidalWard", school="Magic.School.Water",
         target="UNIT", delivery="INSTANT", cost=12.0, range=1800.0,
         effects=[("Magic.Effect.Shield",40.0,10.0,0.0,1)],
         system="/Game/MagicSpells/Water/FX/NS_WaterPulse",
         slot="cast_system"),

    dict(name="DA_Soul_StoneSentinel", display="Stone Sentinel",
         spell="Magic.Spell.Earth.StoneSentinel", school="Magic.School.Earth",
         target="GROUND", delivery="SUMMON", cost=20.0, range=1800.0,
         effects=[("Magic.Effect.Summon",1.0,20.0,0.0,1)],
         system="/Game/MagicSpells/Earth/FX/NS_GiantSpike",
         slot="cast_system"),
    dict(name="DA_Soul_Tailwind", display="Tailwind",
         spell="Magic.Spell.Air.Tailwind", school="Magic.School.Air",
         target="GLOBAL", delivery="STRATEGIC", cost=10.0, range=0.0,
         effects=[("Magic.Effect.StrategicMovement",0.20,1.0,0.0,1)],
         system="/Game/MagicSpells/Air/FX/NS_AirAscend",
         slot="cast_system"),
]

for spec in spells:
    try:
        spell = get_or_create(spec["name"], SPELL_PATH, spell_cls)
        spell.set_editor_property("display_name", spec["display"])
        spell.set_editor_property(
            "description", "Soul real-time battle spell: " + spec["display"])
        spell.set_editor_property("spell_tag", make_tag(spec["spell"]))
        spell.set_editor_property("school_tag", make_tag(spec["school"]))
        spell.set_editor_property(
            "target_mode", enum_value(target_enum, spec["target"]))
        spell.set_editor_property(
            "delivery_mode", enum_value(delivery_enum, spec["delivery"]))
        spell.set_editor_property("range", spec["range"])
        spell.set_editor_property("cooldown_seconds", 6.0)

        cost = cost_cls()
        cost.set_editor_property(
            "resource_tag", make_tag("Magic.Resource.Mana"))
        cost.set_editor_property("amount", spec["cost"])
        spell.set_editor_property("costs", [cost])

        effects = []
        for tag_name, mag, dur, radius, max_targets in spec["effects"]:
            eff = effect_cls()
            eff.set_editor_property("effect_tag", make_tag(tag_name))
            eff.set_editor_property("magnitude", mag)
            eff.set_editor_property("duration_seconds", dur)
            eff.set_editor_property("radius", radius)
            eff.set_editor_property("max_targets", max_targets)
            effects.append(eff)
        spell.set_editor_property("effects", effects)

        profile_name = spec["name"].replace(
            "DA_Soul_", "DA_SoulPresentation_")
        profile = get_or_create(profile_name, PRES_PATH, pres_cls)
        profile.set_editor_property("spell_tag", make_tag(spec["spell"]))
        system = unreal.load_object(None, spec["system"] + "." +
            spec["system"].rsplit("/", 1)[-1])
        if not system:
            raise RuntimeError(
                "provider system failed to load: " + spec["system"])
        profile.set_editor_property(spec["slot"], system)
        profile.set_editor_property("icon_capture_time_seconds", 0.65)

        unreal.EditorAssetLibrary.save_loaded_asset(
            spell, only_if_is_dirty=False)
        unreal.EditorAssetLibrary.save_loaded_asset(
            profile, only_if_is_dirty=False)
    except Exception as exc:
        result["errors"].append(
            spec["name"] + ": " + repr(exc) + "\n" + traceback.format_exc())

result["pass"] = (
    len(result["errors"]) == 0 and
    len(result["created"]) + len(result["updated"]) == 12)
os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "w", encoding="utf-8") as f:
    json.dump(result, f, indent=2)
print("SOUL_REALTIME_MAGIC_ASSETS pass=%s created=%d updated=%d errors=%d" %
      (result["pass"], len(result["created"]),
       len(result["updated"]), len(result["errors"])))
if not result["pass"]:
    raise RuntimeError("Soul real-time magic asset creation failed")
