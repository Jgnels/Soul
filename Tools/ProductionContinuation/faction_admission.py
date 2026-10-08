"""Read-only admission worksheet: existing canonical routes, authored roles and cooked content.
Never creates or activates a runtime scenario and never substitutes donor assignments.
"""
from pathlib import Path
import json,hashlib,collections
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionContinuation-20261008'
def read(p):return json.loads((R/p).read_text(encoding='utf-8-sig'))
paths=['Data/soul_world_overmap_v1_20260922.json','Data/soul_campaign_start_states_v1_20260922.json','Data/soul_overmap_battle_handoff_v1_20260922.json','Data/battlefield_recipes.json','Data/CampaignComposition/RuntimeProof.json','Data/CampaignComposition/HumanRuntimeProof.json','Evidence/SettlementEnvironmentPlan-20261005/canonical-environment-candidates.json']
g,starts,handoff,recipes,fixture,human,assignments=map(read,paths)
nodes={n['id']:n for n in g['nodes']};recipe_ids={v['id'] for v in recipes['recipes']};start=starts['scenarios']['six_faction_sandbox_candidate'];assigned={v['region_id']:v for v in assignments['settlement_candidates'] if v['slot_tier']=='major'}
handoffs={(v['source_region'],v['destination_region']):v for v in handoff['handoffs']}
stage=json.loads((E/'Local/stage-loose-r2/Diagnostics/receipt.json').read_text());assert stage['pass'];content=Path(stage['stage'])/'Windows/Soul/Content'
def cooked(package):
 if not package or not package.startswith('/Game/'):return None
 p=content/(package[len('/Game/'):]+'.umap')
 return p.is_file()
profiles=[]
for label,d in [('ordinary composition',fixture),('Human proof',human)]:
 enabled=[v for v in d.get('battlefield_candidates',[]) if v.get('enabled')]
 profiles.append({'profile':label,'default_battle_map':d['battle_map'],'default_cooked_map_present':cooked(d['battle_map']),'enabled_battlefield_candidates':[{'id':v['id'],'map':v['map'],'cooked_map_present':cooked(v['map'])} for v in enabled], 'runtime_identity_gate':'humans/human_knight versus dwarves/dwarf_warrior only'})
rows=[]
for edge in g['edges']:
 for a,b in [(edge['a'],edge['b']),(edge['b'],edge['a'])]:
  node=nodes[b];h=handoffs.get((a,b));recipe=node.get('battle_recipe_hint')
  rows.append({'source':a,'destination':b,'destination_macro_region':node['macro_region'],'destination_homeland_affinity':node.get('owner'),'proposed_sandbox_start_owner':start['region_owners'].get(b),'canonical_route_kind':edge['route'],'road':edge['road'],'existing_directed_approach':h['strategic_route']['entry_direction'] if h else None,'existing_recipe_hint':recipe,'recipe_metadata_present':recipe in recipe_ids,'runtime_enabled_as_six_faction':False})
assert len(rows)==102 and len({(x['source'],x['destination']) for x in rows})==102
assert set(handoffs)<=set((x['source'],x['destination']) for x in rows)
capitals=[]
for faction,v in start['factions'].items():
 rid=v['capital_region'];a=assigned[rid];wrapper={'humans':'/Game/Soul/Maps/Settlements/L_HumanCapital_Authored','dwarves':'/Game/Soul/Maps/Settlements/L_DwarfHold_Authored'}.get(faction)
 capitals.append({'faction':faction,'region_id':rid,'canonical_settlement_id':a['settlement_id'],'approved_candidate_source_codes':a['candidate_listing_codes'],'candidate_demo_package':a.get('candidate_demo_package'),'candidate_demo_cooked_presence':cooked(a.get('candidate_demo_package')),'qualified_authored_wrapper':wrapper,'wrapper_cooked_presence':cooked(wrapper),'qualification_scope':'Human: new cooked proof passes; Dwarf: previous authored proof retained, not rerun' if wrapper else 'No deep authored proof admitted; no assignment inferred from file presence','runtime_activated':False})
counts={'directed_connections':len(rows),'with_existing_directed_approach':sum(x['existing_directed_approach'] is not None for x in rows),'without_existing_directed_approach':sum(x['existing_directed_approach'] is None for x in rows),'with_recipe_metadata':sum(x['recipe_metadata_present'] for x in rows),'major_seat_candidates':len(capitals)}
out={'status':'DERIVED_ADMISSION_WORKSHEET_NOT_RUNTIME_DATA','source_sha256':{p:hashlib.sha256((R/p).read_bytes()).hexdigest() for p in paths},'counts':counts,'current_fixture_battlefields':profiles,'major_seats':capitals,'directed_connections':rows,'interpretation':['Existing runtime can choose its admitted fallback without an approach direction. Missing records are content/admission debt, not proof of movement failure.','Historical recipe hints and donor strings do not override the later approved environment-role matrix.','Cooked map presence proves bytes only, not city, battle, scale, navigation or six-faction acceptance.','No force counts, faction balance, diplomacy, new city assignments or physical anchor changes were invented.','36-region traversal and six independent gameplay factions are distinct qualification gates.']}
(E/'six-faction-admission.json').write_text(json.dumps(out,indent=2)+'\n',encoding='utf-8')
lines=['# Six-faction admission coverage','', 'Derived read-only worksheet. Existing canonical IDs and assignments are preserved; nothing is activated.','',f"The 51 legal connections produce **102 directed approaches**. The existing founder handoff provides **{counts['with_existing_directed_approach']}** explicit approach directions; **{counts['without_existing_directed_approach']}** currently remain unset. All 102 destinations have existing recipe metadata. The runtime still supports its qualified fallback when an approach is unset; this is not a new movement failure.",'','The ordinary composition fixture admits Dragon Graveyard variants; the Human proof uses its authored city. A 36-anchor world does not yet imply 36 geographically matched battle locations or six admitted combat rosters.','', '| Faction | Existing seat | Approved source codes | Qualified wrapper cooked | Candidate demo cooked |','|---|---|---|---|---|']
for a in capitals:lines.append('| '+ ' | '.join([a['faction'],a['canonical_settlement_id'],', '.join(a['approved_candidate_source_codes']),str(a['wrapper_cooked_presence']),str(a['candidate_demo_cooked_presence'])])+' |')
lines+=['','`null` means no verified package assignment, not a search/download verdict. Historical candidate availability is not re-audited here. Human and Dwarf use the two frozen authored proofs; no third integration is started.','', 'The next six-faction increment should admit exact existing faction/roster identities and prove one supported matchup through the current bridge, then bind explicit region-context battle environments. Do not fill the 82 missing directions with invented values or treat the historical recipe donor text as approved replacement art.','', 'Exact per-edge records, source hashes, current admitted maps and stage presence are in `six-faction-admission.json`.']
(E/'six-faction-admission.md').write_text('\n'.join(lines)+'\n',encoding='utf-8');print(json.dumps(counts))
