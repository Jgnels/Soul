"""Report ordered admission separately from observed battle qualification."""
from pathlib import Path
import json,re,argparse,itertools
R=Path(__file__).resolve().parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--evidence-root',type=Path,required=True);a=p.parse_args();E=a.evidence_root.resolve();assert E.is_relative_to(R/'Evidence')
 source=(R/'Source/SoulCore/Private/SoulCampaignBattleBridge.cpp').read_text()
 matches=re.findall(r'\{TEXT\("([^"]+)"\),TEXT\("([^"]+)"\),TEXT\("([^"]+)"\),TEXT\("([^"]+)"\)\}',source)
 assert matches,'Exact ordered admission table not found; refuse inferred generic support'
 pairs={(a,d):(au,du) for a,au,d,du in matches};assert len(pairs)==len(matches)
 campaign=(R/'Source/SoulCore/Private/SoulCampaign.cpp').read_text()
 units=dict(re.findall(r'if \(FactionId == TEXT\("([^"]+)"\)\) return TEXT\("([^"]+)"\);',campaign))
 ids=['humans','dwarves','orcs','vikings','nature','dark'];evidence={}
 prior=R/'Evidence/SixFactionFoundation-20261008/six-faction-readiness.json'
 for x in json.loads(prior.read_text())['ordered_matchups']:
  if x['admitted']:evidence[(x['attacker'],x['defender'])]={'path':str(prior.relative_to(R)),'scope':'Inherited qualified natural battle; unchanged forward visual branch, focused native regression required this mission.'}
 for f in sorted(E.glob('*-result.json')):
  d=json.loads(f.read_text())
  if d.get('pass') and d.get('attacker') and d.get('defender'):
   key=(d['attacker'],d['defender']);scope=[x.get('runtime_kind') for x in d.get('runtime',[])]
   current=evidence.get(key,{}).get('scope');current_score=current.count('packaged') if isinstance(current,list) else -1
   # Preserved editor receipts must not downgrade a completed cooked proof.
   if scope.count('packaged')>=current_score:evidence[key]={'path':str(f.relative_to(R)),'scope':scope}
 rows=[]
 for a,d in itertools.permutations(ids,2):
  admitted=(a,d) in pairs
  if admitted:
   assert pairs[a,d]==(units[a],units[d]),'Admission contains an aliased roster'
   status='PASS' if (a,d) in evidence else 'ADMITTED_NOT_RUN';reason='Exact ordered admission; encounter still requires a legal occupied hostile edge and a geographically matched existing recipe.'
  elif a not in units or d not in units:status='REJECT_UNSUPPORTED_ROSTER';reason='At least one faction lacks an admitted exact infantry roster.'
  else:status='REJECT_OTHER_EXPLICIT_REASON';reason='Exact rosters exist but this ordered pair is not on the explicit admission list.'
  rows.append({'attacker':a,'defender':d,'status':status,'admitted':admitted,'reason':reason,'evidence':evidence.get((a,d)) if admitted else None})
 out={'admitted':len(pairs),'total_ordered_pairs':30,'rows':rows,'autonomous_ai_enabled':False,'note':'Pair admission is necessary, not sufficient: no-approach or other action-specific rejection can still prevent a particular encounter.'}
 (E/'ordered-pair-matrix.json').write_text(json.dumps(out,indent=2),encoding='utf-8')
 lookup={(x['attacker'],x['defender']):x['status'] for x in rows}
 lines=['# Ordered encounter matrix','','Attacker rows; defender columns. PASS evidence scopes are explicit in the JSON receipt.','', '| Attacker | '+' | '.join(ids)+' |','|---|'+'---|'*6]
 for a in ids:lines.append('| '+a+' | '+' | '.join('-' if a==d else lookup[a,d] for d in ids)+' |')
 lines+=['',str(len(pairs))+' / 30 explicitly admitted. Full autonomous AI remains OFF.','',out['note']]
 (E/'ordered-pair-matrix.md').write_text('\n'.join(lines)+'\n',encoding='utf-8');print(json.dumps({'admitted':len(pairs),'passed':sum(x['status']=='PASS' for x in rows)}))
if __name__=='__main__':main()
