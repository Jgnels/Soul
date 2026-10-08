"""Native complex collision through each crossing and 12 m of both approaches."""
import unreal as u,json,math
from pathlib import Path
R=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');E=R/'Evidence/MapPolish-20261007';B=R/'Evidence/ProductionWorldComposition-20261007'
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert w.get_path_name().startswith('/Game/SoulCampaignComposition/L_Composition_3500_r2')
a=u.get_editor_subsystem(u.EditorActorSubsystem);ignore=[x for x in a.get_all_level_actors() if x.get_actor_label().startswith('Polish_Inactive_')]
rows=[]
for c in json.loads((B/'selected-crossing-sites-r3.json').read_text())['crossings']:
 if c['gate_id']=='woodland_bridge':continue
 t=math.radians(c['yaw_deg']);half=c['span_m']/2;count=math.ceil((c['span_m']+24)*2);samples=[]
 for i in range(count+1):
  along=-half-12+(c['span_m']+24)*i/count;x=(c['center_xy_m'][0]+math.cos(t)*along)*100-175000;y=(c['center_xy_m'][1]+math.sin(t)*along)*100-175000
  hit=u.SystemLibrary.line_trace_single(w,u.Vector(x+.1,y+.1,100000),u.Vector(x+.1,y+.1,-100000),u.TraceTypeQuery.ECC_VISIBILITY,True,ignore,u.DrawDebugTrace.NONE).to_dict()
  samples.append(dict(along_m=along,xy_m=[(x+175000)/100,(y+175000)/100],z_m=hit['impact_point'].z/100 if hit.get('blocking_hit') else None,actor=hit['hit_actor'].get_actor_label() if hit.get('blocking_hit') else None))
 rows.append(dict(id=c['gate_id'],span_m=c['span_m'],samples=samples))
(E/('native-crossing-probes-'+globals().get('PROBE_REVISION','r1')+'.json')).write_text(json.dumps({'rows':rows,'note':'Axis probes diagnose decks and bank seams; approaches curve away outside the span, so off-road hillside slope is not route grade.'},indent=2))
print('NATIVE_CROSSING_PROBES',sum(len(r['samples']) for r in rows))
