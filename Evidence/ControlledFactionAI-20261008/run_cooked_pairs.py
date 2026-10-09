"""Sequential cooked exact-pair qualification; stop on any failed run or collector."""
from pathlib import Path
import argparse,json,subprocess,sys,shutil
R=Path.cwd();E=R/'Evidence/ControlledFactionAI-20261008';p=argparse.ArgumentParser();p.add_argument('--pairs',nargs='+',required=True,choices=['dwarves','orcs','vikings','human_nature','nature','dwarves_orcs','orcs_dwarves','dwarves_vikings','vikings_dwarves','orcs_vikings','vikings_orcs']);a=p.parse_args()
S=E/'Local/stage-controlled-factions-r1/Diagnostics/receipt.json';assert json.loads(S.read_text())['pass']
for key in a.pairs:
 run='cooked-'+key.replace('_','-')+'-r1';load=run+'-load';result=E/(key+'-reverse-result.json')
 if result.exists():
  prior=json.loads(result.read_text());backup=E/(key+'-editor-result.json');assert not backup.exists();assert all(x['runtime_kind']=='editor_game' for x in prior['runtime']);shutil.copy2(result,backup)
 steps=[['Tools/ProductionContinuation/qualify_runtime.py','controlled','--run',run,'--six-proof','--controlled-attacker',key,'--stage-receipt',str(S),'--visual-fps','10','--evidence-root',str(E)],['Tools/ProductionContinuation/qualify_runtime.py','load','--run',load,'--six-proof','--controlled-attacker',key,'--source',str(E/'Local'/run),'--stage-receipt',str(S),'--visual-fps','10','--evidence-root',str(E)],['Tools/ControlledFactionAI/collect_reverse.py','--run',run,'--load',load,'--faction',key,'--evidence-root',str(E)]]
 for index,args in enumerate(steps):
  with (E/(run+'-step-'+str(index)+'.log')).open('w') as f:r=subprocess.run([sys.executable]+args,cwd=R,stdout=f,stderr=subprocess.STDOUT)
  print(key,index,r.returncode,flush=True)
  if r.returncode:raise SystemExit(r.returncode)
 print('COOKED_PAIR_PASS',key,flush=True)
