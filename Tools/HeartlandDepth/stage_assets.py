"""Admit a dependency-closed additive cook into a NEW private stage only.
Never modifies the prior stage or its linked immutable files. Changed packages
are installed atomically from private copies, with old base hashes retained.
"""
from pathlib import Path
import argparse,hashlib,json,os,shutil,sys
R=Path(__file__).resolve().parents[2];sys.path.insert(0,str(R/'Tools/ProductionContinuation'));sys.path.insert(0,str(R/'Tools'))
from stage_manifest import verify_manifest_presence
from qualify_soul_vertical import conflicting_processes
p=argparse.ArgumentParser();p.add_argument('--stage-receipt',type=Path,required=True);p.add_argument('--cook-receipt',type=Path,required=True);a=p.parse_args()
E=(R/'Evidence/HumanHeartlandDepth-20261010').resolve();sp=a.stage_receipt.resolve();cp=a.cook_receipt.resolve()
assert sp.is_relative_to(E/'Local') and cp.is_relative_to(E/'Local') and not conflicting_processes()
def digest(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
s=json.loads(sp.read_text());c=json.loads(cp.read_text());assert s['pass'] and c['pass'] and not c['standalone_cook'] and c['inline_material_shaders']
boundary=E/'Local/depth-cook-boundary.json';b=json.loads(boundary.read_text());assert digest(boundary)==c['boundary_sha256']
prior=Path(b['base_receipt']);old=json.loads(prior.read_text());root=(Path(s['stage'])/'Windows').resolve();oldroot=(Path(old['stage'])/'Windows').resolve()
assert root!=oldroot and Path(s['prior_stage_receipt']).resolve()==prior.resolve()
assert root.is_relative_to(Path(os.environ['LOCALAPPDATA'])/'Soul/CampaignAlphas')
assert verify_manifest_presence(sp,root)>0 and verify_manifest_presence(prior,oldroot)>0
allowed_replace='/Game/Soul/Maps/Settlements/SL_HumanCapital_Houses'
packages={x['package']:x for x in b['cook_packages']};rows=c['files'];assert rows and len({x['relative'] for x in rows})==len(rows)
required=sum(x['bytes'] for x in rows)+16*2**20;assert shutil.disk_usage(root).free>=8*2**30+required
before={};admitted=[]
for row in rows:
 assert row['package'] in packages
 relative=Path(row['relative']);dest=(root/relative).resolve();src=Path(row['source']).resolve()
 assert dest.is_relative_to(root) and src.is_relative_to(cp.parent.parent/'Cooked')
 assert relative.suffix in ('.uasset','.umap','.uexp','.ubulk','.uptnl') and digest(src)==row['sha256']
 assert relative.as_posix().startswith('Soul/Content/'+row['package'][6:]+'.')
 if dest.exists():
  assert row['package']==allowed_replace,'Unreviewed existing package replacement: '+row['package']
  before[row['relative']]=digest(dest)
 if (oldroot/relative).exists():assert row['package']==allowed_replace
# Mark incomplete before mutation; a partial stage can never pass the launcher.
s['pass']=False;sp.write_text(json.dumps(s,indent=2))
for row in rows:
 dest=root/row['relative'];dest.parent.mkdir(parents=True,exist_ok=True);private=dest.with_name(dest.name+'.depth-incoming')
 assert not private.exists() and private.resolve().is_relative_to(root)
 shutil.copy2(row['source'],private);assert digest(private)==row['sha256'];os.replace(private,dest)
 assert digest(dest)==row['sha256']
 assert shutil.disk_usage(root).free>=8*2**30
 admitted.append({k:row[k] for k in ('relative','sha256','bytes')})
for relative,sha in before.items():assert digest(oldroot/relative)==sha,'Prior base was modified'
base_list=sp.parent/'prelinked-cooked-files.json';base=json.loads(base_list.read_text());newhash={x['relative']:x['sha256'] for x in admitted}
for row in base:
 if row['relative'] in newhash:row['sha256']=newhash[row['relative']];row['bytes']=(root/row['relative']).stat().st_size;row['private_depth_replacement']=True
base_list.write_text(json.dumps(base,indent=2))
supplement={x['relative']:x for x in s.get('supplemental_cooked_files',[])};supplement.update({x['relative']:x for x in admitted})
s['supplemental_cooked_files']=list(supplement.values());s['depth_cook_receipt']=str(cp);s['depth_cook_receipt_sha256']=digest(cp);s['depth_owned_replacements_before']=before;s['depth_new_asset_count']=len(admitted);s['fresh_asset_cook']=True;s['source_cooked_bytes_preserved']=True;s['prior_base_unchanged']=True;s['packaged_runtime_qualified']=False
s['asset_registry_note']='Base registry/shared shaders retained. Explicitly loaded dependency-closed additions carry inline shaders. Local loose stage; not standalone/distribution packaging.'
sp.write_text(json.dumps(s,indent=2));verify_manifest_presence(sp,root)
s['pass']=True;sp.write_text(json.dumps(s,indent=2));print('DEPTH_STAGE_ASSETS_PASS',len(admitted),'files; prior base preserved')
