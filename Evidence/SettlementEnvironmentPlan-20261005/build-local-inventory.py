"""Read-only local metadata inventory; writes receipts only beside this script."""
from pathlib import Path
import json, hashlib, datetime

HERE = Path(__file__).resolve().parent
AOE = Path('D:/Unreal Projects/AoEAssetRenderLab/Content')
approved = json.loads((HERE/'approved-environments.json').read_text())['entries']
manifests = json.loads((HERE/'local-manifest-index.json').read_text())
catalog = json.loads(Path('D:/RefinedBadger/Vendor/Copperlight-Asset-Catalog/catalog/products.json').read_text(encoding='utf-8-sig'))
old = json.loads((HERE.parent/'CampaignWorldTerrain-20261005/Pivot/asset-inventory-files.json').read_text())

# Full manifest-relative file-existence sweep performed 2026-10-06 in this audit.
# This is presence qualification, not content-hash verification or UE closure.
presence = {'b8d49812': (199, 0), '9ee42cda': (95,95), '454baa93': (533,0),
            'd8d7b289': (822,822), 'b91e0942': (333,333), '3bfee149': (22002,22002),
            '33d8621e': (567,567), '3cc08b13': (258,258)}
codes = ['VK','TS','DO','FV','LV','FN','DC','DG','AC','MK','FF','LF','VT','AV','GT','RH','CD','WZ','MT','WC','ST','IF']
reuse = {
 'VK':'Preserve authored capital first; use the same longhouse, quay and palisade families for Viking secondary settlements after payload recovery.',
 'TS':'Use authored harbor/town demo; smaller waterfront subsets can supply Human major towns and port approaches.',
 'DO':'Second authored harbor identity; docks and quays can supply maritime chokepoints without copying a whole city.',
 'FV':'Existing coherent town demo can supply a Human minor town; select authored street/building subsets for villages.',
 'LV':'Use authored shore village and waterfront subsets after local payload identification.',
 'FN':'Pass/approach environment and regional battlefield source, not a generic replacement for Viking capital.',
 'DC':'Citadel plus authored entrance/exterior sections are candidates for Dwarven city, fortress and pass gate; inspect complete sections before reducing.',
 'DG':'Regional land/landmark source: retain large skeletal landmarks, rock composition and coherent local setting.',
 'AC':'Preserve alien castle showcase for Evil capital; entrance and rock-formation level instances offer bounded reuse.',
 'MK':'Preserve full CastleTown authored city/castle, derive strategic silhouette from that same scene; castle, courtyard, houses, bridges and roads are authored levels.',
 'FF':'Nature town, with smaller authored stilt/platform dwelling groups for villages; keep support structures with buildings.',
 'LF':'Dwarven forge city/industrial landmark; use complete forge composition before selecting furnace/chimney modules.',
 'VT':'Secondary Viking town and modular harbor/longhouse vocabulary once local payload identified.',
 'AV':'Jeff-assigned Orc village source; inspect coherent village and retain family proportions/materials before faction adaptation.',
 'GT':'Jeff-assigned Evil town source; free sample scope may be a modular subset, not a complete town.',
 'RH':'Orc/Evil fortress or ruins; actual gatehouse and curtain-wall level assemblies can preserve authored military silhouette.',
 'CD':'Second Orc/Evil castle/ruin family; local payload and authored layout unverified.',
 'WZ':'Approved shared war machines/siege camps; authored showcase and outpost blueprints provide reusable military clusters.',
 'MT':'Jeff assigned siege/camp support; exact listing payload must be identified before claiming it contains a particular siege asset.',
 'WC':'Approved additional camp: overview and native tent/prop family support small camp/outpost groups.',
 'ST':'Nature unique landmark environment; preserve authored temple/tree relationship after payload recovery.',
 'IF':'Dwarven industry/mining/forge family; no local payload found in bounded search, so no fabricated demo path.'}
rows=[]
for entry,code in zip(approved,codes):
 row={**entry,'code':code,'ownership_status':'Owned per Jeff; no purchase required or authorized',
      'identity_confidence':'unresolved_local_mapping','local_completeness':'not_located_in_bounded_search',
      'visual_acceptance':'not claimed by this file-only audit','proposed_modular_reuse':reuse[code],
      'local_sources':[],'demo_maps':[],'evidence':[]}
 ms=[m for m in manifests if entry['url'] in m.get('listing_urls',[])]
 if ms:
  row['identity_confidence']='exact_listing_URL_in_local_manifest'
  caches=[m for m in ms if 'FabLibrary' not in Path(m['path']).parts]
  ref=caches[0] if caches else ms[0]
  expected,primary=presence[entry['id'][:8]]
  row['manifest_package_count']=expected
  row['manifest_presence_qualification']={'expected':expected,'cache_present':expected,'primary_AoE_present':primary,'method':'Checked every manifest-relative .uasset/.umap path with is_file; no content hash or UE dependency claim.'}
  row['local_completeness']='all_manifest_package_paths_present; UE dependency closure and runtime unqualified'
  row['evidence']=[{'manifest':m['path'],'sha256':m['sha256'],'listing_urls':m['listing_urls']} for m in ms]
  row['families']=[k for k in ref['roots'] if not k.startswith('__')]
  if primary:
   row['local_sources'].append({'kind':'primary_imported_content','content_root':str(AOE),'present':primary})
  for m in caches:
   root=Path(m['path']).parent/'data'/'Content'
   row['local_sources'].append({'kind':'owned_vault_payload','content_root':str(root),'present':expected})
  content=AOE if primary else Path(ref['path']).parent/'data'/'Content'
  for f in ref['map_files']:
   rel=Path(f).relative_to('Content');p=content/rel
   row['demo_maps'].append({'package':'/Game/'+str(rel.with_suffix('')).replace('\\','/'),'file':str(p),'exists':p.is_file(),'bytes':p.stat().st_size if p.is_file() else None})
 if code=='MK':
  proof=json.loads((HERE/'human-capital-local-identity.json').read_text())
  row['identity_confidence']='exact_catalog_ID_in_existing_Macbeth_plan_and_hall_audit'
  row['local_completeness']='PARTIAL:43 maps+1753 uassets; only661 KingsHall external actors; full capital cannot be reconstructed from this closure'
  row['local_sources']=[{'kind':'Macbeth_imported_partial_content','content_root':'D:/RefinedBadger/Games/Macbeth/Content'}]
  row['families']=['CastleTown'];row['demo_maps']=proof['maps'];row['evidence']=['human-capital-local-identity.json','human-capital-recovery-availability.json']
  row['full_capital_blocker']='Recover full CastleTown external actor/object closure. Historical whole-family transfer is incomplete; two ordinary packages also remain absent.'
  row['footprint_evidence']={'full_capital':'not measured; cannot infer from empty/incomplete levels','KingsHall_only_actor_location_extent_cm':[3000,3400,2200],'hall_actor_count':661,'warning':'Actor-location envelope is not aggregate architectural bounds and is not capital footprint.'}
 if code=='DC':
  f=old['families']['DwarvenCitadel'];root=AOE/'DwarvenCitadel'
  row['identity_confidence']='strong_named_local_family; exact836bcc0d manifest linkage not recovered'
  row['local_completeness']='1509 family packages +1060 external actors observed; no exact manifest coverage or runtime qualification'
  row['local_sources']=[{'kind':'primary_imported_content','content_root':str(AOE)}];row['families']=['DwarvenCitadel']
  row['demo_maps']=[{'package':s,'file':str(AOE/(s.removeprefix('/Game/')+'.umap')),'exists':(AOE/(s.removeprefix('/Game/')+'.umap')).is_file()} for s in f['maps']]
  row['evidence']=['../CampaignWorldTerrain-20261005/Pivot/dwarven-assembly-offline-receipt.json','../CampaignWorldTerrain-20261005/Pivot/asset-inventory-files.json']
 if code in ['VK','TS']:
  cr=next(r for r in catalog if entry['url']==r.get('SourceURL'))
  row['identity_confidence']='exact_listing_in_catalog; recorded_cache_path_missing'
  row['evidence']=[{'catalog_id':cr['AssetCatalogId'],'recorded_path':cr['LocalPath'],'recorded_path_exists':Path(cr['LocalPath']).exists(),'historical_file_summary':cr['FileSummary']}]
  if code=='VK':row['local_completeness']='Recorded cache absent; Soul/Content/Viking_Village is a dangling junction to absent AoE Viking_Village'
 if code in ['AV','GT','VT']:
  names={'AV':'African Village','GT':'Free Sample Dark Fantasy Gothic Environment Kitbash','VT':'Viking Village Modular kit'}
  cr=next(r for r in catalog if r.get('ProductName')==names[code])
  row['evidence']=[{'catalog_id':cr['AssetCatalogId'],'title':cr['ProductName'],'publisher':cr['Publisher'],'owned':cr['Owned'],'local_paths':cr['LocalPaths'],'limit':'Title/publisher ownership record only; exact listing URL not recorded in this catalog row.'}]
 rows.append(row)
result={'captured_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'authority_input':'approved-environments.json','scope':'Read-only filesystem, existing receipts, catalog and Epic/Fab manifests; no UE, donor/source/asset edits, download or purchase.',
 'method_limits':['A manifest URL proves listing identity; path presence is a separate test.','All-manifest-paths-present does not prove byte integrity, resolved engine/plugin dependencies, complete loaded scene, battle fit or visual quality.','Every .umap is listed in demo_maps, including level instances and asset overview maps; the name alone does not prove playable demo status.','Unavailable local payload does not mean unowned.'],
 'bounded_roots':['D:/Unreal Projects/AoEAssetRenderLab/Content','C:/Users/Jeff/Desktop/VaultCache','D:/RefinedBadger/Games','D:/RefinedBadger/AssetLibraries','D:/RefinedBadger/BakeoffAssets','D:/RefinedBadger/Donors','D:/kitbash','D:/AssetCaches/EpicVaultCache','D:/EpicGamesLauncher/VaultCache','D:/ColdStorage/Relocated-C/Users/Jeff/Documents/Unreal Projects','C:/Users/Jeff/Documents/Unreal Projects','C:/RefinedBadger'],
 'entries':rows}
(HERE/'local-environment-inventory.json').write_text(json.dumps(result,indent=2)+'\n')
lines=['# Approved environment local inventory','','All 22 remain owned per Jeff. This audit records identity and file presence independently; it does not grant visual acceptance. The exact Human capital is **Medieval Kingdom / CastleTown**, not Kingdom_Capital. Its complete demo is blocked by missing external actor closure.','','| Code | Approved listing / assigned role | Local identity and availability | Authored map to inspect |','|---|---|---|---|']
demo={ 'FV':'/Game/Medieval_Fantasy_Village/Levels/L_Medieval_Town','DC':'/Game/DwarvenCitadel/Maps/DwarvenCitadel; /Maps/ModulesAssembly','DG':'/Game/Dragon_graveyard/Level/L_showcase_level','AC':'/Game/AlienPlanet/Levels/L_Showcase','MK':'/Game/CastleTown/Levels/Persistant/PL_CastleTown (incomplete closure)','FF':'/Game/Forest_village/Level/L_showcase_level','LF':'/Game/Legendary_Forge/Maps/L_showcase_forge','RH':'/Game/Ravenhold/Scenes/HM-FortCastle_Kit_Demo','WZ':'/Game/Medieval_Warzone/Levels/L_Showcase','WC':'/Game/WarCamp_Collection/Maps/Overview/Camp_Garrison_War_Collection'}
for r in rows:
 paths='; '.join(s['content_root'] for s in r['local_sources']) or 'No live payload identified'
 lines.append(f"| {r['code']} | [{r['verified_listing_title']}]({r['url']}) — {r['user_role']} | {r['identity_confidence']}. {r['local_completeness']}. | `{demo[r['code']]}` |" if r['code'] in demo else f"| {r['code']} | [{r['verified_listing_title']}]({r['url']}) — {r['user_role']} | {r['identity_confidence']}. {r['local_completeness']}. | Unverified; no invented path. |")
lines+=['','Exact physical file paths, counts, every map path, manifest hashes and modular reuse notes are in [local-environment-inventory.json](local-environment-inventory.json). The two cached-only complete families are AlienPlanet at `D:/Unreal Projects/AoEAssetRenderLab/Content/VaultCache/FantasyA4185fb9ca8f8V1/data/Content` (533 packages) and Medieval_Fantasy_Village at `C:/Users/Jeff/Desktop/VaultCache/Medieval9ff219ea5d87V1/data/Content` (199 packages).','','Primary AoE has all manifest package paths for Dragon Graveyard (95), Forest Village (822 including external actors), Legendary Forge (333), Ravenhold (22,002 including external actors/objects), Medieval Warzone (567) and War Camp (258). This is file-presence coverage, not UE verification. DwarvenCitadel has 1,509 family packages and 1,060 external actors but lacks recovered exact-listing manifest proof.','','Do not substitute local LAYA `Kingdom_Capital` (09a1119d), older Hivemind `Medieval_Megapack`/826f1b55, Emily Dickinson `Village`/Stylized Town, or JustBStudios Water City/fd908d45 for one of the specified listings. They may remain historical donors or future explicitly approved reuse; similar names do not establish identity.','','The copied maps are not evidence of complete city layout. For the selected Human capital, the current hall-only actor-location extent is 30 × 34 × 22 m; it is neither full-city footprint nor aggregate building bounds. Recover full CastleTown before choosing a reduced strategic silhouette.']
(HERE/'local-environment-inventory.md').write_text('\n'.join(lines)+'\n')
print(json.dumps({'entries':len(rows),'exact_manifest_payloads':len(presence),'partial_exact_capital':1,'named_family_pending_exact_link':1,'unlocated_payloads':12,'output':str(HERE/'local-environment-inventory.json')},indent=2))
