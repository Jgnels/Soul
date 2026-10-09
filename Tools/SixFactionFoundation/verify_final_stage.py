"""After-run integrity: all base cooked hashes, fresh roster copies and executable."""
from pathlib import Path
import argparse,datetime,hashlib,json,sys
R=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(R/'Tools/ProductionContinuation'))
from stage_manifest import verify_manifest_presence

def digest(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

def main():
 p=argparse.ArgumentParser();p.add_argument('--receipt',type=Path,required=True);p.add_argument('--evidence-root',type=Path,required=True);p.add_argument('--output',default='stage-integrity-after-final.json');a=p.parse_args()
 e=a.evidence_root.resolve();receipt=a.receipt.resolve();assert e.is_relative_to(R/'Evidence') and receipt.is_relative_to(e/'Local')
 assert Path(a.output).name==a.output
 d=json.loads(receipt.read_text());assert d['pass'];stage=Path(d['stage'])/'Windows';n=verify_manifest_presence(receipt,stage)
 base=json.loads((receipt.parent/'prelinked-cooked-files.json').read_text());mismatches=[]
 for row in base:
  f=(stage/row['relative']).resolve();assert f.is_relative_to(stage.resolve())
  if not f.is_file() or digest(f)!=row['sha256']:mismatches.append(row['relative'])
 exe=stage/'Soul/Binaries/Win64/SoulComposition.exe';binary=digest(exe)==d['binary_sha256']
 out={'checked_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'stage_receipt':str(receipt.relative_to(R)),'manifest_entries_present':n,'base_cooked_files_hash_checked':len(base),'base_cooked_mismatches':mismatches,'supplemental_copied_files_hash_checked':len(d.get('supplemental_cooked_files',[])),'binary_matches_fresh_build':binary,'pass':not mismatches and binary,'external_remover_diagnosed':False}
 (e/a.output).write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out));assert out['pass']
if __name__=='__main__':main()
