"""Require actual directed RBCombat, correct six-army result and byte-identical cold restore."""
from pathlib import Path
import argparse,json,re,hashlib
R=Path(__file__).resolve().parents[2]
def digest(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--evidence-root',type=Path,required=True);p.add_argument('--run',required=True);p.add_argument('--load',required=True);p.add_argument('--faction',choices=['dwarves', 'orcs', 'vikings', 'nature', 'human_nature', 'dwarves_orcs', 'orcs_dwarves', 'dwarves_vikings', 'vikings_dwarves', 'orcs_vikings', 'vikings_orcs'],required=True);a=p.parse_args();E=a.evidence_root.resolve();assert E.is_relative_to(R/'Evidence')
 pairs={'dwarves': ('dwarves', 'humans'), 'orcs': ('orcs', 'humans'), 'vikings': ('vikings', 'humans'), 'nature': ('nature', 'humans'), 'human_nature': ('humans', 'nature'), 'dwarves_orcs': ('dwarves', 'orcs'), 'orcs_dwarves': ('orcs', 'dwarves'), 'dwarves_vikings': ('dwarves', 'vikings'), 'vikings_dwarves': ('vikings', 'dwarves'), 'orcs_vikings': ('orcs', 'vikings'), 'vikings_orcs': ('vikings', 'orcs')}
 attacker,defender=pairs[a.faction]
 rows=[];logs=[]
 for name,marker in [(a.run,'SOUL_CONTROLLED_BATTLE_PASS'),(a.load,'SOUL_CAMPAIGN_COLD_LOAD_PASS')]:
  d=E/'Local'/name;s=json.loads((d/'runtime/summary.json').read_text());log=(d/'runtime/unreal.log').read_text(encoding='utf-8-sig',errors='replace');logs.append(log)
  assert s['clean_shutdown'] and s['completion_marker_observed'] and not s['crash_signatures'] and marker in log
  temps=[g['temperature_c'] for line in (d/'runtime/telemetry.jsonl').read_text().splitlines() for g in json.loads(line)['gpu']]
  rows.append({'run':name,'runtime_kind':s['runtime_kind'],'seconds':s['elapsed_seconds'],'peak_gpu_c':max(temps),'summary':str((d/'runtime/summary.json').relative_to(E))})
 d=E/'Local'/a.run/'User/Saved';cold=E/'Local'/a.load/'User/Saved';slot='Soul.Composition3500.Controlled.'+a.faction;relative='RBSave/Domains/'+slot+'.domain.rbsave'
 assert (d/relative).read_bytes()==(cold/relative).read_bytes()
 snapshot=json.loads((d/'CampaignInputExpectedSnapshot.json').read_text(encoding='utf-8-sig'));assert snapshot['profile']==slot
 result={k.lower():v for k,v in snapshot['result'].items()};assert len(result)==len(snapshot['result']) and result['magic']==0
 armies={x['faction']:x for x in snapshot['other_faction_states']}
 armies['humans']={'troops':snapshot['army']['human_knight'],'region':snapshot['player_region']}
 army=armies[attacker];defending=armies[defender]
 assert army['troops']==result['player'] and defending['troops']==result['enemy']
 target=snapshot['result_target'];won=snapshot['result_won'];assert defending['region']==target
 assert snapshot['owners'][target]==(attacker if won else defender)
 assert not won or army['region']==target
 units={'humans':'human_knight','dwarves':'dwarf_warrior','orcs':'orc_hammer_warrior','vikings':'viking_axe_warrior','nature':'nature_bear_warrior'}
 meshes={'humans':'/Game/Knights_Pack/Meshes/Knight_02/Mesh_UE4/Full/SK_Knight_02_Full_01.SK_Knight_02_Full_01','dwarves':'/Game/Dwarf_Pack/Bedvar/Mesh/SK_Dwarf_Bedvar_Full.SK_Dwarf_Bedvar_Full','orcs':'/Game/Fantasy_Pack/Characters/Orc_Hummer/Mesh/SK_Orc_Hummer.SK_Orc_Hummer','vikings':'/Game/Fantasy_Pack/Characters/Viking_Ulf/Mesh/SK_Ulf_Full.SK_Ulf_Full','nature':'/Game/Animals_Warrior_Pack/Mesh/Bear/SK_Bear_Full.SK_Bear_Full'}
 log=logs[0];assert 'player='+attacker+':'+units[attacker]+':' in log and 'enemy='+defender+':'+units[defender]+':' in log
 assert 'RB_COMBAT_PRODUCED: accepted=1' in log
 bodies={0:set(),1:set()}
 for line in log.splitlines():
  if 'SOUL_EXACT_ROSTER_BODY' not in line:continue
  side=int(re.search(r'side=(\d+)',line)[1]);index=int(re.search(r'index=(\d+)',line)[1]);assert side in bodies
  expected=meshes[attacker if side==0 else defender]
  assert 'mesh='+expected+' ' in line and 'role=LINE' in line
  assert 'faction='+(attacker if side==0 else defender)+' ' in line
  bodies[side].add(index)
 assert all(len(x)>=15 for x in bodies.values())
 captures=[{'path':str(f.relative_to(E)),'sha256':digest(f)} for folder in [d,cold] for f in (folder/'Screenshots').glob('*.png')]
 resolved=re.search(r'SOUL_BATTLE_RESOLVED: won=(\d+) playerSurvivors=(\d+) enemySurvivors=(\d+) physical=(\d+)/(\d+) routed=(\d+)/(\d+) waves=(\d+)/(\d+)',log)
 assert resolved and int(resolved[2])==result['player'] and int(resolved[3])==result['enemy']
 physical={'attacker':int(resolved[4]),'defender':int(resolved[5]),'attacker_routed':bool(int(resolved[6])),'defender_routed':bool(int(resolved[7]))}
 out={'pass':True,'attacker':attacker,'defender':defender,'runtime':rows,'natural_result':'attacker_victory' if won else 'defender_victory','target':target,'attacker_return_region':army['region'],'owner':snapshot['owners'][target],'result':result,'physical_result':physical,'survivor_semantics':'Existing morale-defeated forces have zero effective campaign survivors; physical alive/reserve totals are reported separately.','exact_body_counts':{str(k):len(v) for k,v in bodies.items()},'rb_accepted_contacts':log.count('RB_COMBAT_PRODUCED: accepted=1'),'slot':slot,'save_sha256':digest(d/relative),'fresh_process_save_bytes_identical':True,'captures':captures,'performance_qualification':False}
 (E/(a.faction+'-reverse-result.json')).write_text(json.dumps(out,indent=2));print(json.dumps(out,indent=2))
if __name__=='__main__':main()
