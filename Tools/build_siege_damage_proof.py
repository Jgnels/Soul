import unreal, os, json, traceback

OUT=r"D:\RefinedBadger\Games\Soul\Evidence"
MAP="/Game/Soul/Maps/Tests/LV_Soul_SiegeDamageProof"
DATA="/Game/Soul/Data/Settlements/DA_Soul_SiegeDamageProof"
SETTLEMENT="siege_damage_proof"
CLEAN="/Game/Ravenhold/Art/1_Assets/1_Architecture/SM_FCK_CurtainWall_10m"
RUIN="/Game/Ravenhold/Art/1_Assets/1_Architecture/SM_FCK_CurtainWall_10m_Ruined"

levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
tools=unreal.AssetToolsHelpers.get_asset_tools()
def n(x):return unreal.Name(x)

def scenario():
    a=unreal.load_asset(DATA)
    if a:return a
    fac=unreal.DataAssetFactory();fac.set_editor_property("data_asset_class",unreal.SoulSettlementScenarioData)
    a=tools.create_asset("DA_Soul_SiegeDamageProof","/Game/Soul/Data/Settlements",unreal.SoulSettlementScenarioData,fac)
    a.set_editor_property("settlement_id",n(SETTLEMENT));a.set_editor_property("faction_id",n("humans"));a.set_editor_property("region_id",n("proof_region"));a.set_editor_property("fortification_level",1);a.set_editor_property("wall_integrity_permille",1000)
    unreal.EditorAssetLibrary.save_asset(DATA,False);return a

def sm_actor(mesh_path,loc,label):
    m=unreal.load_asset(mesh_path)
    if not m:raise RuntimeError("Missing Ravenhold mesh "+mesh_path)
    a=actors.spawn_actor_from_class(unreal.StaticMeshActor,loc,unreal.Rotator(0,0,0));a.set_actor_label(label,False);a.static_mesh_component.set_static_mesh(m);return a

def main():
    if not levels.new_level(MAP):raise RuntimeError("Could not create damage proof level")
    clean=sm_actor(CLEAN,unreal.Vector(0,0,0),"Ravenhold_Wall_Intact")
    ruin=sm_actor(RUIN,unreal.Vector(0,0,0),"Ravenhold_Wall_Breached")
    seg=actors.spawn_actor_from_class(unreal.SoulFortificationSegmentActor,unreal.Vector(0,0,0),unreal.Rotator(0,0,0));seg.set_actor_label("Soul_WestWall_State",False);seg.set_editor_property("settlement_id",n(SETTLEMENT));seg.set_editor_property("segment_id",n("west_wall"));seg.set_editor_property("breach_scar_id",n("west_wall_breach"));seg.set_editor_property("intact_actors",[clean]);seg.set_editor_property("breached_actors",[ruin]);seg.apply_wall_state(1000,False)

    sc=scenario()
    boot=actors.spawn_actor_from_class(unreal.SoulSettlementBootstrapActor,unreal.Vector(0,0,0),unreal.Rotator(0,0,0));boot.set_actor_label("Soul_DamageProof_Bootstrap",False);boot.set_editor_property("scenario",sc)
    ctrl=actors.spawn_actor_from_class(unreal.SoulSettlementPresentationController,unreal.Vector(0,0,0),unreal.Rotator(0,0,0));ctrl.set_actor_label("Soul_DamageProof_Presentation",False);ctrl.set_editor_property("settlement_id",n(SETTLEMENT))
    obj=actors.spawn_actor_from_class(unreal.SoulSiegeObjectiveActor,unreal.Vector(-300,0,0),unreal.Rotator(0,0,0));obj.set_actor_label("Soul_Objective_Gatehouse",False);obj.set_editor_property("settlement_id",n(SETTLEMENT));obj.set_editor_property("objective_id",n("gatehouse"));obj.set_editor_property("linked_breach_scar_id",n("west_wall_breach"));obj.set_editor_property("effect_tag",n("OpenGate"))

    # Simple ground/light are only proof staging, not in-house shipping art.
    cube=unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    ground=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-80),unreal.Rotator(0,0,0));ground.set_actor_label("ProofGround",False);ground.static_mesh_component.set_static_mesh(cube);ground.set_actor_scale3d(unreal.Vector(30,30,.5))
    light=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,1000),unreal.Rotator(-45,-30,0));light.set_actor_label("ProofKeyLight",False)
    try:light.get_component_by_class(unreal.DirectionalLightComponent).set_editor_property("intensity",80000.0)
    except:pass
    cam=actors.spawn_actor_from_class(unreal.SoulTownViewAnchor,unreal.Vector(1800,-2600,1300),unreal.Rotator(0,0,0));cam.set_actor_label("Soul_DamageProof_View",False);cam.set_editor_property("settlement_id",n(SETTLEMENT));cam.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(cam.get_actor_location(),unreal.Vector(0,0,250)),False)
    unreal.EditorLevelLibrary.save_current_level()
    with open(os.path.join(OUT,"siege_damage_proof_manifest.json"),"w",encoding="utf-8") as f:json.dump({"map":MAP,"clean":CLEAN,"ruined":RUIN,"scar":"west_wall_breach","note":"Engine cube/light are proof staging only and must not be treated as shipping art."},f,indent=2)
    unreal.log("SOUL_SIEGE_PROOF_DONE "+MAP)

try:main()
except Exception:
    e=traceback.format_exc();unreal.log_error("SOUL_SIEGE_PROOF_ERROR\n"+e);open(os.path.join(OUT,"siege_damage_proof_error.txt"),"w",encoding="utf-8").write(e)
