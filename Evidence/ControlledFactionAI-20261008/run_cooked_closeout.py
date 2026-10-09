"""Run the remaining cooked regressions only after the six non-Human pair queue passes."""
from pathlib import Path
import subprocess,sys,json
R=Path.cwd();E=R/'Evidence/ControlledFactionAI-20261008';S=E/'Local/stage-controlled-factions-r1/Diagnostics/receipt.json'
for key in ['dwarves_orcs','orcs_dwarves','dwarves_vikings','vikings_dwarves','orcs_vikings','vikings_orcs']:
 d=json.loads((E/(key+'-reverse-result.json')).read_text());assert d['pass'] and all(x['runtime_kind']=='packaged' for x in d['runtime'])
steps=[
 [str(E/'run_cooked_pairs.py'),'--pairs','dwarves','orcs','vikings'],
 ['Tools/ProductionContinuation/qualify_runtime.py','sixstate','--run','cooked-controlled-turn-r1','--six-proof','--controlled-turn','--stage-receipt',str(S),'--visual-fps','10','--evidence-root',str(E)],
 ['Tools/ProductionContinuation/qualify_runtime.py','load','--run','cooked-controlled-turn-load-r1','--six-proof','--controlled-turn','--source',str(E/'Local/cooked-controlled-turn-r1'),'--stage-receipt',str(S),'--visual-fps','10','--evidence-root',str(E)],
 ['Tools/SixFactionFoundation/collect_state.py','--run','cooked-controlled-turn-r1','--load','cooked-controlled-turn-load-r1','--controlled-turn','--output','controlled-turn-cooked.json','--evidence-root',str(E)],
 ['Tools/SixFactionFoundation/verify_final_stage.py','--receipt',str(S),'--evidence-root',str(E),'--output','stage-integrity-after.json']]
for i,args in enumerate(steps):
 with (E/('final-cooked-step-'+str(i)+'.log')).open('w') as f:r=subprocess.run([sys.executable]+args,cwd=R,stdout=f,stderr=subprocess.STDOUT)
 print('FINAL_COOKED_STEP',i,r.returncode,flush=True)
 if r.returncode:raise SystemExit(r.returncode)
print('FINAL_COOKED_REGRESSIONS_PASS',flush=True)
