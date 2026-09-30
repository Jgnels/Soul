import unreal as u,json
from pathlib import Path
r=Path('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview');w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert w.get_path_name().startswith('/Game/SoulCampaignMountain/L_mountain_campaign.')
rows=[]
for p in json.loads((r/'TerrainWork/collision-probes.json').read_text()):
    start=u.Vector(p['x'],p['y'],100000);end=u.Vector(p['x'],p['y'],-100000)
    h=u.SystemLibrary.sphere_trace_single(w,start,end,.25,u.TraceTypeQuery.ECC_VISIBILITY,False,[],u.DrawDebugTrace.NONE,True)
    d=h.to_dict() if h else {};ok=d.get('blocking_hit',False) and isinstance(d.get('hit_actor'),u.LandscapeProxy)
    rows.append({'hit':bool(ok),'error_cm':abs(d['impact_point'].z-p['expected_z']) if ok else None})
report={'map':w.get_path_name(),'probe_type':'0.25cm radius sweep at exact heightmap vertices','probes':len(rows),'hits':sum(x['hit'] for x in rows),'max_error_cm':max(x['error_cm'] for x in rows if x['hit']),'note':'Zero-width ray at 40 exact outer-component vertices misses; neighboring sub-centimeter samples hit. Retain this precision caveat for integration.','details':rows}
(r/'Evidence/mountain-collision-surface.json').write_text(json.dumps(report,indent=2));print({k:v for k,v in report.items() if k!='details'})
assert report['hits']==len(rows) and report['max_error_cm']<1
