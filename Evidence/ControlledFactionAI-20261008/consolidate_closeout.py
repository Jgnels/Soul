"""Consolidate only completed verified gates; never turn pending evidence into PASS."""
from pathlib import Path
import json,datetime,subprocess,sys,hashlib
R=Path.cwd();E=R/'Evidence/ControlledFactionAI-20261008'
def read(name):return json.loads((E/name).read_text())
def write(name,d):(E/name).write_text(json.dumps(d,indent=2)+'\n')
keys=['dwarves','orcs','vikings','human_nature','nature','dwarves_orcs','orcs_dwarves','dwarves_vikings','vikings_dwarves','orcs_vikings','vikings_orcs']
results=[]
for key in keys:
 d=read(key+'-reverse-result.json');assert d['pass'] and d['fresh_process_save_bytes_identical'] and len(d['runtime'])==2 and all(x['runtime_kind']=='packaged' for x in d['runtime']);d['receipt']=key+'-reverse-result.json';results.append(d)
turn=read('controlled-turn-cooked.json');assert turn['pass'] and turn['exact_fresh_restore'] and turn['neutral_captures']==6 and turn['replayed_proposals_rejected']==8 and all(x['runtime_kind']=='packaged' for x in turn['runtime'])
for name in ['stage-integrity-after.json','preservation-final.json','final-native-results.json','source-admission-replay.json']:
 assert read(name)['pass'],name
assert read('final-native-results.json')['tests']==43
assert all(x['pass'] for x in read('qualified-builds.json')['builds'])
inspected={x['path'].replace('\\','/') for x in read('cooked-visual-inspection.json')['captures']}
for d in results:
 battle=next(x['path'].replace('\\','/') for x in d['captures'] if x['path'].endswith('Vertical_Battle.png'));assert battle in inspected,battle
subprocess.run([sys.executable,'Tools/ControlledFactionAI/pair_matrix.py','--evidence-root',str(E)],check=True)
matrix=read('ordered-pair-matrix.json');assert matrix['admitted']==14 and sum(x['status']=='PASS' for x in matrix['rows'])==14
now=datetime.datetime.now(datetime.timezone.utc);head=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip();branch=subprocess.check_output(['git','branch','--show-current'],text=True).strip();assert branch=='codex/soul-bannerlord-campaign-map-20260929'
readiness=read('six-faction-readiness.json');readiness['status']='CONTROLLED_FOUNDATION_PASS_NOT_FULL_AUTONOMOUS_AI';readiness['ordered_matchups']=matrix['rows'];readiness['ready_for_first_bounded_autonomous_military_experiment']=True;readiness['experiment_scope']='Opt-in capped observer experiment through prepared/executed actions. Interactive Human defensive control is not qualified.'
for f in readiness['factions']:
 f['save_restore']='PASS current cooked controlled-turn F5/F9 and separate-process restoration; controlled-turn-cooked.json'
 f['controlled_action_api']='PASS native adversarial admission and actual cooked execution; no autonomous scheduler'
 f['controlled_turn']='PASS cooked eight legal moves, six neutral captures, eight replay rejections, F5/F9 and cold restore'
 f['ordered_attacker_pairs']=[{'defender':x['defender'],'status':x['status'],'evidence':x['evidence']} for x in matrix['rows'] if x['attacker']==f['faction'] and x['admitted']]
 if f['faction']=='nature':f['battlefield_coverage']='Human <-> Nature at canonical Orc Watch with existing geographically matched dragon_watch field fallback. Both editor/cooked natural outcomes and save/cold restore PASS. No Nature forest/city integration claimed.'
write('six-faction-readiness.json',readiness)
plans=read('additional-pair-plan.json')
for row in plans:
 d=next(x for x in results if x['receipt']==row['proof']+'-reverse-result.json');row.update(status='PASS_COOKED_NATURAL_RESULT_AND_COLD_RESTORE',natural_receipt=d['receipt'],natural_result=d['natural_result'],owner=d['owner'],attacker_return_region=d['attacker_return_region'])
write('additional-pair-plan.json',plans)
peak=max(x['peak_gpu_c'] for d in results+[turn] for x in d['runtime'])
out={'status':'COMPLETE','started_utc':'2026-10-09T05:52:57Z','qualified_closeout_utc':now.isoformat(),'elapsed_hours':round((now-datetime.datetime(2026,10,9,5,52,57,tzinfo=datetime.timezone.utc)).total_seconds()/3600,3),'starting_head':'18ba64b0ded98253237af74f6c8903af9e056a93','source_commit':'43060da2e40d415afe2dd24387e4b2f9d29bb2d3','head_before_evidence_commit':head,'ending_head_receipt':'final-head.txt (written after the scoped evidence commit to avoid a self-referential commit hash)','branch':branch,'controlled_ai_admission_policy':'PASS','non_human_attacker_battle_support':'PASS','next_nature_dark_matchup':'PASS: Nature in both directions against Humans','ordered_pairs_admitted':14,'ordered_pairs_natural_proven':14,'fresh_cooked_pairs':11,'inherited_human_forward_pairs':3,'ready_for_first_bounded_autonomous_military_ai_experiment':True,'full_six_faction_ai':'STILL OFF','world_foundation':'KEEP','native_tests_passed':43,'source_tool_tests_passed':17,'fresh_targets':['SoulEditor','Soul','SoulComposition'],'controlled_six_faction_turn':'PASS','explicit_directed_approaches':20,'unset_directed_approaches':82,'stage_integrity':'PASS','preservation':'PASS','cooked_functional_peak_gpu_c':peak,'performance_qualification':'NOT RUN; prior sustained85C cutoff unresolved; these runs capped10FPS','interactive_human_defender_control_qualified':False,'temp_file_aging_removal_diagnosed':False,'balance_or_physical_combat_determinism_qualified':False,'push':False,'merge':False,'default_promoted':False,'next_action':'A separately scoped opt-in deterministic military action selector with strict turn/action caps, using this admission API; retain observer scope until Human defensive control is separately qualified.'}
write('final-closeout.json',out)
lines=['# Six-faction readiness','','Controlled action foundation PASS; full autonomous strategy OFF.','', '| Faction | Exact core infantry | Admitted outgoing opponents |','|---|---|---|']
for f in readiness['factions']:lines.append('| '+f['faction']+' | '+str(f['exact_unit'] or 'UNSUPPORTED')+' | '+(', '.join(x['defender'] for x in f['ordered_attacker_pairs']) or 'None; legal neutral/owned moves only')+' |')
lines+=['','All six participated in the cooked controlled turn: eight normal moves, six neutral captures, eight replay rejections, F5/F9 and separate-process exact restoration.','', '14/30 explicit ordered pairs have natural runtime evidence:11 freshly cooked this mission,3 inherited Human forward proofs with current focused native regression. See ordered-pair-matrix.json for per-pair scopes.','', 'Approaches:20 explicit,82 unset. No new records fabricated; exercised pass/watch encounters use existing geographic recipe fallback.','', 'Ready for a FIRST bounded observer military-AI experiment: YES. This is not a playable six-faction AI release. Human defensive control/HUD needs separate qualification; Dark has no exact battle roster and Nature does not fight Dwarf/Orc/Viking yet.','', 'Terrain, population, authored capitals and performance remain outside this mission.']
(E/'six-faction-readiness.md').write_text('\n'.join(lines)+'\n')
subprocess.run([sys.executable,'Tools/ControlledFactionAI/make_review.py','--evidence-root',str(E),'--final'],check=True)
print(json.dumps(out))
