"""Summarize observed campaign outcomes; never edits saves or supplies battle results."""
from pathlib import Path
import argparse,collections,json,re

p=argparse.ArgumentParser();p.add_argument('run',type=Path);a=p.parse_args()
root=a.run.resolve();saved=root/'User/Saved'
log=(root/'runtime/unreal.log').read_text(encoding='utf-8-sig',errors='replace')
runtime=json.loads((root/'runtime/summary.json').read_text())
final=json.loads((saved/'CampaignInputExpectedSnapshot.json').read_text(encoding='utf-8-sig'))
timeline=[json.loads(x) for x in (saved/'AlphaTimeline.jsonl').read_text(encoding='utf-8-sig').splitlines() if x]
# F9 intentionally revisits days. Last observed action for a day/faction describes the retained branch.
actions={};attempts=collections.Counter();branch=0;per_branch=collections.Counter()
for line in log.splitlines():
 if 'SOUL_ALPHA_MID_SAVE_PASS' in line:branch+=1
 m=re.search(r'SOUL_ALPHA_ACTION day=(\d+) faction=(\w+) action=(.*?) region=(\w+) troops=(\d+) gold=(\d+) ap=(\d+) cursor=(\d+)',line)
 if not m:continue
 day,faction,verb,region,troops,gold,ap,cursor=m.groups();day=int(day)
 row=dict(day=day,faction=faction,action=verb,region=region,troops=int(troops),gold=int(gold),ap=int(ap),cursor=int(cursor))
 actions[(day,faction)]=row;attempts[faction]+=1;per_branch[(branch,day,faction)]+=1
retained=[actions[k] for k in sorted(actions) if k[0]<=final['day']]
by_faction={}
for faction in ['dwarves','orcs','vikings']:
 rows=[r for r in retained if r['faction']==faction]
 by_faction[faction]={
  'turn_actions':len(rows),'captures':sum(r['action'].startswith('captured ') for r in rows),
  'moves':sum(r['action'].startswith('moved to ') for r in rows),
  'withdrawals':sum(r['action'].startswith('withdrew to ') for r in rows),
  'attacks':sum(r['action'].startswith('attacked ') for r in rows),
  'recruit_actions':sum(r['action'].startswith('recruited ') for r in rows),
  'recruited':sum(int(re.search(r'recruited (\d+)',r['action'])[1]) for r in rows if r['action'].startswith('recruited ')),
  'holds':sum(r['action'].startswith('held') for r in rows),
 }
results=[];encounter={}
for line in log.splitlines():
 start=re.search(r'SOUL_CONTROLLED_ENCOUNTER faction=(\w+) from=(\w+) to=(\w+) recipe=(\w+)',line)
 if start:encounter=dict(zip(['attacker','source','target','recipe'],start.groups()))
 m=re.search(r'SOUL_CONTROLLED_RESULT attacker=(\w+) defender=(\w+) region=(\w+) owner=(\w+) victory=(\d) survivors=(\d+)/(\d+)',line)
 if not m:continue
 attacker,defender,region,owner,won,attack_survive,defend_survive=m.groups()
 results.append(dict(attacker=attacker,defender=defender,target_region=encounter.get('target'),recipe=encounter.get('recipe'),attacker_return_region=region,owner=owner,attacker_won=won=='1',survivors=[int(attack_survive),int(defend_survive)]))
baseline=timeline[0];passive=[]
for faction in ['nature','dark']:
 before=next(x for x in baseline['other_faction_states'] if x['faction']==faction)
 after=next(x for x in final['other_faction_states'] if x['faction']==faction)
 passive.append(all(before[k]==after[k] for k in ['region','troops','resources','recruitment_pools']) and all(final['owners'][r]==faction for r,v in baseline['owners'].items() if v==faction))
receipt={
 'run':str(root),'profile':final['profile'],'seed':final['alpha_seed'],'final_day':final['day'],
 'completed_turns':final['day']-1,'actions':by_faction,'observed_action_attempts_including_F9_rewind':dict(attempts),
 'retained_turns_since_process_start':final['day']-timeline[0]['day'],
 'rejected_action_candidates':dict(collections.Counter(re.findall(r'SOUL_ALPHA_REJECT .*?reason=(\S+)',log))),
 'retained_branch_actions':retained,'battle_results_observed':results,'resolved_encounters':final['resolved'],
 'final_territory':dict(collections.Counter(final['owners'].values())),
 'final_player':{k:final[k] for k in ['player_region','army','resources','ap','hero']},
 'final_factions':final['other_faction_states'],
 'midcampaign_F5_F9_exact':'SOUL_ALPHA_MID_SAVE_PASS' in log,
 'cold_restore_exact':'SOUL_ALPHA_COLD_RESTORE_PASS' in log,
 'human_defense_control':'SOUL_HUMAN_DEFENSE_CONTROL: pass=1 side=1' in log,
 'human_attack_control':'SOUL_HUMAN_DEFENSE_CONTROL: pass=1 side=0' in log,
 'passive_factions_unchanged':all(passive),
 'at_most_one_AI_action_per_day_per_save_branch':max(per_branch.values(),default=0)<=1,
 'runtime_clean':runtime.get('clean_shutdown',False) and runtime.get('completion_marker_observed',False),
 'performance_qualification':False,
}
receipt['meaningful_20_turn_pass']=all([
 receipt['runtime_clean'],receipt['completed_turns']>=20,receipt['midcampaign_F5_F9_exact'],
 receipt['passive_factions_unchanged'],receipt['at_most_one_AI_action_per_day_per_save_branch'],
 sum(x['captures'] for x in by_faction.values())>=2,sum(x['moves'] for x in by_faction.values())>=2,
 any(x['attacker']!='humans' and x['defender']!='humans' for x in results),
 sum(x['recruited'] for x in by_faction.values())>0,
])
receipt['cold_continuation_pass']=all([receipt['runtime_clean'],receipt['cold_restore_exact'],
 receipt['passive_factions_unchanged'],receipt['at_most_one_AI_action_per_day_per_save_branch'],
 receipt['retained_turns_since_process_start']>0])
(root/'campaign-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps({k:v for k,v in receipt.items() if k not in ['retained_branch_actions','final_factions']},indent=2))
