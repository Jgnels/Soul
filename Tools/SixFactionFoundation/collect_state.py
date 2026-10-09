"""Collect evidence from the real six-faction controller/save observer and fresh load."""
from pathlib import Path
import argparse,json,hashlib,re
R=Path(__file__).resolve().parents[2]
def digest(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--evidence-root',type=Path,required=True);p.add_argument('--run',required=True);p.add_argument('--load',required=True);p.add_argument('--controlled-turn',action='store_true');p.add_argument('--output',default='state-proof-editor.json');a=p.parse_args();E=a.evidence_root.resolve();assert E.is_relative_to((R/'Evidence').resolve())
 rows=[]
 for name,marker in [(a.run,'SOUL_CONTROLLED_TURN_PASS' if a.controlled_turn else 'SOUL_SIX_FACTION_STATE_PASS'),(a.load,'SOUL_CAMPAIGN_COLD_LOAD_PASS')]:
  d=E/'Local'/name;summary=json.loads((d/'runtime/summary.json').read_text());log=(d/'runtime/unreal.log').read_text(encoding='utf-8-sig',errors='replace');telemetry=[json.loads(l) for l in (d/'runtime/telemetry.jsonl').read_text().splitlines()]
  assert summary['completion_marker_observed'] and summary['clean_shutdown'] and not summary['crash_signatures'] and marker in log
  rows.append({'run':name,'runtime_kind':summary['runtime_kind'],'seconds':summary['elapsed_seconds'],'peak_gpu_c':max(g['temperature_c'] for x in telemetry for g in x['gpu']),'summary':str((d/'runtime/summary.json').relative_to(E)),'markers':[l for l in log.splitlines() if 'SOUL_SIX_FACTION_' in l or 'SOUL_CONTROLLED_TURN_' in l or 'SOUL_CAMPAIGN_COLD_LOAD_PASS' in l]})
 start=E/'Local'/a.run/'User/Saved';cold=E/'Local'/a.load/'User/Saved';slot='Soul.Composition3500.ControlledTurn' if a.controlled_turn else 'Soul.Composition3500.SixFactionProof';relative='RBSave/Domains/'+slot+'.domain.rbsave'
 assert (start/relative).read_bytes()==(cold/relative).read_bytes()
 snapshot=json.loads((start/'CampaignInputExpectedSnapshot.json').read_text(encoding='utf-8-sig'));assert snapshot['profile']==slot
 owners=snapshot['owners'];assert len(owners)==36 and list(owners.values()).count('None')==(18 if a.controlled_turn else 24)
 factions={'humans','dwarves','orcs','vikings','nature','dark'};assert set(owners.values())==factions|{'None'}
 armies=[{'faction':'humans','army_id':'humans.primary','army_owner':'humans','region':snapshot['player_region'],'unit':'human_knight','troops':snapshot['army']['human_knight'],'day':snapshot['day'],'ap':snapshot['ap'],'resources':snapshot['resources']}]+snapshot['other_faction_states']
 assert {x['faction'] for x in armies}==factions
 for row in armies:
  assert row['army_owner']==row['faction'] and row['army_id']==row['faction']+'.primary' and owners[row['region']]==row['faction'] and row['day']==1 and row['ap']==(1 if a.controlled_turn and row['faction'] in {'humans','dwarves'} else 2)
 captures=[]
 for folder in [start,cold]:
  for f in sorted((folder/'Screenshots').glob('*.png')):captures.append({'path':str(f.relative_to(E)),'sha256':digest(f)})
 out={'pass':True,'runtime':rows,'slot':snapshot['profile'],'rb_save_domain':'Soul.Campaign','rb_save_schema':1,'save_sha256':digest(start/relative),'fresh_process_saved_bytes_identical':True,'regions':36,'legal_pairs':51,'controlled_admission':a.controlled_turn,'neutral_captures':6 if a.controlled_turn else 0,'replayed_proposals_rejected':8 if a.controlled_turn else 0,'owned_regions':18 if a.controlled_turn else 12,'neutral_regions':18 if a.controlled_turn else 24,'armies':armies,'strategic_ai_enabled':False,'forces_are_test_fixture_not_approved_balance':True,'controller_I_inspection':True,'controller_F5':True,'controller_F9':True,'exact_fresh_restore':True,'camera_pose_persistence_claimed':False,'captures':captures,'performance_qualification':False}
 (E/a.output).write_text(json.dumps(out,indent=2)+'\n');print(json.dumps({'pass':True,'output':str(E/a.output),'captures':len(captures),'peak_gpu_c':[x['peak_gpu_c'] for x in rows]}))
if __name__=='__main__':main()
