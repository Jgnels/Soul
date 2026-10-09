"""Require real guarded Viking combat, campaign return and separate-process restore."""
from pathlib import Path
import argparse,json,re,hashlib
R=Path(__file__).resolve().parents[2]
def digest(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--evidence-root',type=Path,required=True);p.add_argument('--run',required=True);p.add_argument('--load',required=True);p.add_argument('--output',required=True);a=p.parse_args();E=a.evidence_root.resolve();assert E.is_relative_to(R/'Evidence')
 rows=[];logs=[]
 for name,marker in [(a.run,'SOUL_CAMPAIGN_ROUNDTRIP_PASS'),(a.load,'SOUL_CAMPAIGN_COLD_LOAD_PASS')]:
  d=E/'Local'/name;s=json.loads((d/'runtime/summary.json').read_text());log=(d/'runtime/unreal.log').read_text(encoding='utf-8-sig',errors='replace');logs.append(log)
  assert s['clean_shutdown'] and s['completion_marker_observed'] and not s['crash_signatures'] and marker in log
  temps=[g['temperature_c'] for line in (d/'runtime/telemetry.jsonl').read_text().splitlines() for g in json.loads(line)['gpu']]
  rows.append({'run':name,'runtime_kind':s['runtime_kind'],'seconds':s['elapsed_seconds'],'peak_gpu_c':max(temps),'summary':str((d/'runtime/summary.json').relative_to(E))})
 d=E/'Local'/a.run/'User/Saved';cold=E/'Local'/a.load/'User/Saved';slot='RBSave/Domains/Soul.Composition3500.VikingProof.domain.rbsave';assert (d/slot).read_bytes()==(cold/slot).read_bytes()
 snapshot=json.loads((d/'CampaignInputExpectedSnapshot.json').read_text(encoding='utf-8-sig'));assert snapshot['enemy_faction']=='vikings' and snapshot['result_target']=='viking_snow_pass'
 result={k.lower():v for k,v in snapshot['result'].items()};assert len(result)==len(snapshot['result']);won=snapshot['result_won'];assert snapshot['player_region']==('viking_snow_pass' if won else 'mountain_shrine') and result['enemy']==snapshot['enemies']['viking_snow_pass'] and result['player']==snapshot['army']['human_knight']
 assert result['magic']==0,'Ordinary proof must not hide automatic spell assistance'
 log=logs[0];assert 'enemy=vikings:viking_axe_warrior:30' in log and 'RB_COMBAT_PRODUCED: accepted=1' in log
 bodies=[line for line in log.splitlines() if 'SOUL_VIKING_ROSTER_BODY' in line];indices=set()
 for line in bodies:
  assert 'mesh=/Game/Fantasy_Pack/Characters/Viking_Ulf/Mesh/SK_Ulf_Full.SK_Ulf_Full' in line and 'role=LINE' in line
  indices.add(int(re.search(r'index=(\d+)',line)[1]))
 assert len(indices)>=15
 if won:assert len(indices)==30 and result['enemy_waves']>0
 weapon_count=log.count('SOUL_VISUAL_WEAPON mesh=SM_Viking_Axe hand=hand_r');assert weapon_count==len(indices)
 captures=[]
 for folder in [d,cold]:
  for f in (folder/'Screenshots').glob('*.png'):captures.append({'path':str(f.relative_to(E)),'sha256':digest(f)})
 out={'pass':True,'faction':'vikings','unit':'viking_axe_warrior','runtime':rows,'ordinary_battle_no_spell_observer':True,'natural_result':'victory' if won else 'defeat','source_region':'mountain_shrine','target_region':'viking_snow_pass','return_region':snapshot['player_region'],'result':result,'exact_owned_body_count':len(indices),'exact_owned_axe_count':weapon_count,'rb_accepted_contact_log_count':log.count('RB_COMBAT_PRODUCED: accepted=1'),'slot':'Soul.Composition3500.VikingProof','save_sha256':digest(d/slot),'fresh_process_save_bytes_identical':True,'exact_restore':True,'captures':captures,'performance_qualification':False}
 (E/a.output).write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out,indent=2))
if __name__=='__main__':main()
