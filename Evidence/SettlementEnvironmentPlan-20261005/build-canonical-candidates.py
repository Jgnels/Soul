"""Generate evidence-only candidate bindings from existing canonical IDs."""
from pathlib import Path
import json, hashlib, datetime
P=Path(__file__).resolve().parent
W=P.parent.parent
world_path=W/'Data/soul_world_overmap_v1_20260922.json'
slot_path=W/'Data/soul_overmap_settlement_slots_v1_20260922.json'
structure_path=W/'Data/settlement_blueprints.json'
battle_path=W/'Data/battlefield_recipes.json'
world=json.loads(world_path.read_text());slots=json.loads(slot_path.read_text());structures=json.loads(structure_path.read_text());battle=json.loads(battle_path.read_text())
inventory=json.loads((P/'local-environment-inventory.json').read_text())
packs={r['code']:r for r in inventory['entries']};nodes={n['id']:n for n in world['nodes']};recipes={r['id']:r for r in battle['recipes']}
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
selection={
 'human_capital':(['MK'],'/Game/CastleTown/Levels/Persistant/PL_CastleTown','Jeff-selected exact capital; blocked on full external actor closure'),
 'dwarf_hold':(['DC','LF'],'/Game/DwarvenCitadel/Maps/DwarvenCitadel','Candidate city; exact listing linkage and loaded layout require qualification. LF remains actual forge-role candidate, not a second city pasted into this one.'),
 'viking_harbour':(['VK'],None,'Jeff-selected capital payload missing; historical MainVillage path is not currently a live map'),
 'nature_treehold':(['FF'], '/Game/Forest_village/Level/L_showcase_level','Town source available; promotion to full Nature capital is a candidate, not explicit Jeff capital assignment. Great-tree requirement unresolved.'),
 'orc_camp':(['RH','CD'],'/Game/Ravenhold/Scenes/HM-FortCastle_Kit_Demo','Candidate occupied fortress/ruin; preserve authored layout and existing city.orc_ruinhold identity; style/layout not yet accepted'),
 'dark_fortress':(['AC'],'/Game/AlienPlanet/Levels/L_Showcase','Jeff-selected capital; local complete package paths available; actual fortress/approach qualification pending'),
 'crossroads':(['FV'],'/Game/Medieval_Fantasy_Village/Levels/L_Medieval_Town','Existing noncanonical market-town slot; use a coherent authored subset only after inspection'),
 'coastal_ruins':([],None,'Retain existing approved Coastal Ruins setting; no new listing substituted'),
 'dark_castle_approach':(['AC'],'/Game/AlienPlanet/BluePrints/LI_Entrance','Existing ward-bastion candidate; entrance scene needs actual bounds and route inspection'),
 'dwarf_forge_approach':(['LF'],'/Game/Legendary_Forge/Maps/L_showcase_forge','Existing forge-outpost candidate; bounded authored outer-works subset rather than whole duplicate city'),
 'nature_forest_clearing':(['FF'],'/Game/Forest_village/Level/L_showcase_level','Existing grove-village candidate; retain coherent authored clearing/subset'),
 'northwest_march':(['FV','VT'],None,'Existing neutral trade-post candidate; neither harbor nor new city ID is implied'),
 'orc_war_camp':(['WC','WZ'],'/Game/WarCamp_Collection/Maps/Overview/Camp_Garrison_War_Collection','Existing camp candidate; actual native tent/prop group to be selected'),
 'viking_forest_track':(['VT','VK'],None,'Existing village candidate; exact selected Viking local payloads missing')}
factions={f['id']:f for f in structures['factions']}
bound=[]
for slot in slots['slots']:
 n=nodes[slot['region_id']];codes,mapname,note=selection[n['id']]
 actual=[]
 for c in codes:
  actual += [m for m in packs[c]['demo_maps'] if m['package']==mapname]
 fkey='nature_candidate' if slot['faction_affinity']=='nature' else slot['faction_affinity'];f=factions.get(fkey,{})
 bound.append({'region_id':n['id'],'settlement_id':slot['settlement_id'],'slot_tier':slot['tier'],'existing_slot_authority':slot['status'],'faction_affinity':slot['faction_affinity'],'candidate_listing_codes':codes,'candidate_demo_package':mapname,'live_demo_files':actual,
 'candidate_note':note,'scene_binding_status':'EVIDENCE_ONLY_NOT_RUNTIME_BINDING','canonical_xy':[n['x'],n['y']],'battle_recipe_id':n['battle_recipe_hint'],
 'required_at_start_physical_groups':None,'buildable_physical_groups':None,'physical_group_status':'UNKNOWN until complete authored-scene inspection; do not infer group/actor bindings from logical names',
 'existing_logical_building_ids':[x['id'] for k in ['recruit_structures','civic_structures'] for x in f.get(k,[])] if slot['tier']=='major' else [],
 'legacy_donor_mapping_status':'Historical Data donor strings require later explicit reconciliation; this receipt does not mutate them'})
regional={
 'viking_fjord_ridge':['FN'],'viking_forest_track':['VT','VK'],'viking_snow_pass':['FN'],'mountain_shrine':[],
 'northwest_march':['FV','VT'],'dwarf_high_quarry':['IF'],'dwarf_snow_basin':[],'dwarf_forge_approach':['LF'],
 'dwarf_mountain_pass':['DC'],'crossroads':['FV'],'old_quarry':[],'river_ford':['TS','DO'],
 'forest_edge':['FF'],'ancient_shrine':['ST'],'orc_watch':['RH','WZ'],'north_pass':['RH'],
 'orc_badlands':[],'orc_war_camp':['WC','WZ','MT'],'orc_ruined_field':['RH','CD'],'orc_broken_bridge':['RH','CD'],
 'coastal_ruins':[],'nature_shrine':['ST'],'nature_forest_clearing':['FF'],'nature_river_woodland':['FF'],
 'nature_grassland_edge':[],'southern_crossing':[],'dark_corrupted_valley':['DG'],'dark_ruined_causeway':['RH','CD'],
 'dark_ash_plain':['DG'],'dark_castle_approach':['AC']}
contexts=[]
for n in world['nodes']:
 rid=n['battle_recipe_hint'];r=recipes.get(rid);codes=regional.get(n['id'],selection.get(n['id'],([],None,''))[0])
 contexts.append({'region_id':n['id'],'settlement_id':n['settlement_id'],'macro_region':n['macro_region'],'biome':n['biome'],'landform':n['landform'],'feature':n['feature'],'existing_battle_recipe_id':rid,'recipe_present':r is not None,'existing_recipe_donor_string':r.get('donor') if r else None,'candidate_environment_codes':codes,
 'proposal_scope':'Retain existing geography and recipe ID; qualify authored local approach/dressing or settlement battle scene. These candidates do not imply graph, route, terrain or campaign-start ownership changes.',
 'runtime_qualification':'UNKNOWN; file presence does not certify traversability, battle scale or spawn/approach fit'})
assert len(nodes)==36 and len(world['edges'])==51 and len(bound)==14
assert len([b for b in bound if b['settlement_id']])==6
assert all(c in packs for b in bound for c in b['candidate_listing_codes'])
assert all(c['recipe_present'] for c in contexts)
out={'captured_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'status':'CANDIDATE_EVIDENCE_ONLY','authority_note':'Copy existing IDs and graph context exactly. World source labels full-world ownership as candidate; eight minor slots remain noncanonical. No runtime/data/source/assets changed.',
 'source_hashes':{str(p.relative_to(W)):sha(p) for p in [world_path,slot_path,structure_path,battle_path]},'counts':{'regions':36,'edges':51,'major_city_ids':6,'minor_candidate_slots':8,'battle_context_rows':36},'settlement_candidates':bound,'battle_context_candidates':contexts,
 'unplaced_approved_town_families':{'codes':['TS','DO','LV','AV','GT'],'reason':'No additional canonical town/harbor settlement IDs in the current14slot layer. Keep as faction role coverage; do not invent new city IDs or attach them to an incompatible coastal/inland location.'}}
(P/'canonical-environment-candidates.json').write_text(json.dumps(out,indent=2)+'\n')
lines=['# Existing settlement candidate bindings','','Six major city IDs and eight existing noncanonical minor slots are preserved. These are proposed environment candidates, not runtime bindings. Every required-at-start/buildable physical actor group is **unknown until actual complete-scene inspection**. Existing logical building IDs are copied separately into the JSON.','','| Region ID | Existing settlement ID | Candidate source | Actual candidate map | Status |','|---|---|---|---|---|']
for b in bound:
 sid=f"`{b['settlement_id']}`" if b['settlement_id'] else 'None (existing candidate only)'
 demo=f"`{b['candidate_demo_package']}`" if b['candidate_demo_package'] else 'Unverified / not selected'
 lines.append(f"| `{b['region_id']}` | {sid} | {', '.join(b['candidate_listing_codes']) or 'Retained local base'} | {demo} | {b['candidate_note']} |")
lines += ['','No new harbor/city slot is created to accommodate a pack. TS, DO, LV, AV and GT remain available faction-family coverage until an existing suitable node/slot is explicitly chosen. This preserves the current strategic scope.','','[canonical-environment-candidates.json](canonical-environment-candidates.json) also records all 36 existing biome/landform/feature/battle-recipe contexts, exact source hashes and the 51-edge source authority. It does not rewrite any recipe. The old recipe donor strings are kept as historical inputs; a capital battle scene for the new MK proof must be explicitly qualified against the existing battle bridge.']
(P/'canonical-environment-candidates.md').write_text('\n'.join(lines)+'\n')
print(json.dumps({'status':'PASS','regions':len(nodes),'edges':len(world['edges']),'slots':len(bound),'major_ids':6,'all36_recipe_ids_resolve':True,'all_candidate_codes_resolve':True},indent=2))
