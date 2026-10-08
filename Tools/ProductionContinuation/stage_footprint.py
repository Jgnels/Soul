"""Read-only cooked footprint and staged configuration receipt (values never printed)."""
from pathlib import Path
import json,hashlib,collections,re
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionContinuation-20261008';S=E/'Local/stage-loose-r2/Diagnostics/receipt.json';s=json.loads(S.read_text());assert s['pass'];root=Path(s['stage'])/'Windows';groups=collections.defaultdict(lambda:{'bytes':0,'files':0});allfiles=[]
for p in root.rglob('*'):
 if not p.is_file():continue
 relative=p.relative_to(root).as_posix();parts=relative.split('/')
 group='/'.join(parts[:3]) if parts[:2]==['Soul','Content'] else '/'.join(parts[:2])
 size=p.stat().st_size;groups[group]['bytes']+=size;groups[group]['files']+=1;allfiles.append((relative,size))
findings=[];config_files=[]
for p in root.rglob('*.ini'):
 relative=p.relative_to(root).as_posix();config_files.append({'path':relative,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()});section=''
 for line in p.read_text(encoding='utf-8-sig',errors='replace').splitlines():
  if line.strip().startswith('['):section=line.strip()
  if '=' not in line or line.lstrip().startswith(';'):continue
  key,value=line.split('=',1);reasons=[]
  if re.search(r'(?<![A-Za-z0-9])[A-Za-z]:[/\\]',value):reasons.append('absolute_windows_path')
  if re.search(r'(?i)(token|password|secret|apikey|api_key)',key) and value.strip().strip(chr(34)):reasons.append('nonempty_sensitive_named_key')
  if 'Nwiro' in section:reasons.append('nwiro_section')
  if reasons:
   row={'file':relative,'section':section,'key':key,'findings':reasons,'value_redacted':True}
   if relative.startswith('Engine/'):
    source=Path('C:/Program Files/Epic Games/UE_5.8')/relative
    row['installed_engine_config_byte_identical']=source.is_file() and source.read_bytes()==p.read_bytes()
    current_section='';source_value=None
    for source_line in source.read_text(encoding='utf-8-sig',errors='replace').splitlines() if source.is_file() else []:
     if source_line.strip().startswith('['):current_section=source_line.strip()
     if current_section==section and '=' in source_line and source_line.split('=',1)[0]==key:source_value=source_line.split('=',1)[1]
    row['key_matches_installed_engine_source']=source_value==value
    row['classification']='matches installed-engine source; value redacted' if row['key_matches_installed_engine_source'] else 'requires review'
   findings.append(row)
rows=[{'family':k,**v,'gib':round(v['bytes']/2**30,5)} for k,v in sorted(groups.items(),key=lambda x:-x[1]['bytes'])]
out={'status':'LOCAL_ONLY_COOKED_STAGE_INVENTORY','stage':str(root),'file_count':len(allfiles),'apparent_file_bytes':sum(x[1] for x in allfiles),'physical_disk_bytes_not_inferred':'10,754 cooked files share hardlinks with this run cook and first stage; apparent totals must not be added as independent disk allocation.','groups':rows,'config_files':config_files,'redacted_config_findings':findings,'machine_specific_findings':[v for v in findings if not v.get('key_matches_installed_engine_source',False)],'config_scan_limit':'Key-name and absolute-Windows-path check only; not a universal security certification.','distributable_archive':False,'default_profile_promoted':False}
(E/'stage-footprint.json').write_text(json.dumps(out,indent=2)+'\n',encoding='utf-8')
lines=['# Actual local cooked-stage footprint','',f"{len(allfiles):,} files; apparent file sizes total {out['apparent_file_bytes']/2**30:.2f} GiB. Cooked assets share hardlinks with this run\'s cook and earlier stage, so these are not additional independently allocated bytes.",'','| Family | Files | Apparent GiB |','|---|---:|---:|']
for v in rows[:22]:lines.append(f"| {v['family']} | {v['files']} | {v['gib']:.3f} |")
lines+=['',f"All {len(config_files)} staged INI files were checked for nonempty sensitive-key names, Nwiro sections and absolute Windows paths without printing values. Raw sensitive-key findings: {len(findings)}; any matching installed-engine source values are classified separately in the JSON. UAT already removed the local Android editor token. This bounded check is not a general security audit.",'','The temporary loose stage is qualified locally and is not a distributable archive. It remains dependent on these recorded local paths; do not advertise it as an uploaded build. No donor or licensed package is committed.','', 'No content pruning is performed merely to lower this number. The authored Human city remains complete; a future compact distribution should use a reviewed profile and normal installer/archive packaging.']
(E/'stage-footprint.md').write_text('\n'.join(lines)+'\n',encoding='utf-8');print(json.dumps({'files':len(allfiles),'gib':out['apparent_file_bytes']/2**30,'config_files':len(config_files),'redacted_findings':findings},indent=2))
