"""Read-only physical battle suitability probe of the existing Human river bridge."""
import unreal as u,json,hashlib
from pathlib import Path
r=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));s=u.get_editor_subsystem(u.EditorActorSubsystem)
p="/Game/SoulCampaignComposition/L_Composition_3500_r2"
source=r/"Content/SoulCampaignComposition/L_Composition_3500_r2.umap";before=hashlib.sha256(source.read_bytes()).hexdigest()
assert u.get_editor_subsystem(u.LevelEditorSubsystem).load_level(p)
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
actors=s.get_all_level_actors();ignored=[a for a in actors if isinstance(a,u.Volume)]
rows=[]
for a in actors:
 label=a.get_actor_label()
 if any(x in label.lower() for x in ("human_west_bridge","river_ford","water","river")):
  c,e=a.get_actor_bounds(False)
  rows.append({"name":a.get_name(),"label":label,"class":a.get_class().get_name(),"center":[c.x,c.y,c.z],"extent":[e.x,e.y,e.z],"rotation":str(a.get_actor_rotation())})
origin=u.Vector(-110853,4792,180)
samples=[]
for dx in range(-3000,3001,250):
 for dy in range(-1750,1751,250):
  h=u.SystemLibrary.line_trace_single_for_objects(w,origin+u.Vector(dx,dy,8000),origin+u.Vector(dx,dy,-8000),[u.ObjectTypeQuery.OBJECT_TYPE_QUERY1],False,ignored,u.DrawDebugTrace.NONE)
  d=h.to_dict() if h else {}
  if d.get("blocking_hit"):
   v=d["impact_point"];n=d["impact_normal"];a=d.get("hit_actor")
   samples.append({"xy":[dx,dy],"z":v.z,"normal":n.z,"actor":a.get_actor_label() if a else ""})
  else:samples.append({"xy":[dx,dy],"miss":True})
assert hashlib.sha256(source.read_bytes()).hexdigest()==before
(r/"Evidence/HumanHeartlandDepth-20261010/Local/campaign-crossing-inspection.json").write_text(json.dumps({"source_sha256":before,"donor_saved":False,"actors":rows,"sample_origin":[origin.x,origin.y,origin.z],"samples":samples},indent=2))
print("SOUL_CROSSING_INSPECTED",len(rows),len(samples))
