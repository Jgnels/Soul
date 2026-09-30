from pathlib import Path
import subprocess,re,json
r=Path(__file__).resolve().parents[1];html=(r/'settlement-plan.html').read_text(encoding='utf-8')
test="""<script>
try {
 let checks=0;const check=(ok,msg)=>{if(!ok)throw Error(msg);checks++};
 check(document.querySelectorAll('#marks .mapmark').length===18,'18 markers');
 for(const f of Object.keys(colors)){Array.from(document.querySelectorAll('#filters button')).find(b=>b.textContent===f).click();check(document.querySelectorAll('#sites button').length===3,'three sites '+f);check(document.querySelectorAll('#routes polyline').length===2,'two routes '+f)}
 for(const s of DATA.sites){choose(s.id);check(document.querySelector('#details h2').textContent===s.name,'detail '+s.id)}
 document.querySelector('#showroutes').checked=false;document.querySelector('#showroutes').onchange();check(document.querySelector('#routes').children.length===0,'hide routes');
 let m=document.createElement('meta');m.id='test-result';m.content='PASS '+checks;document.head.append(m);
} catch(e) {let m=document.createElement('meta');m.id='test-result';m.content='FAIL '+e.message;document.head.append(m)}
</script>"""
(r/'settlement-test.html').write_text(html.replace('</html>',test+'</html>'),encoding='utf-8')
args=[r'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe','--headless','--disable-gpu','--no-first-run','--user-data-dir='+str(r/'.local/MapTest'),'--dump-dom','--virtual-time-budget=3000','http://127.0.0.1:8766/settlement-test.html']
p=subprocess.run(args,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=45)
dom=p.stdout.decode('utf-8',errors='replace');m=re.search(r'<meta id="test-result" content="([^"]+)"',dom)
assert m, p.stderr.decode(errors='replace')[-1000:]
print(m.group(1));assert m.group(1).startswith('PASS')
(r/'Evidence/settlement-ui-test.json').write_text(json.dumps({'result':m.group(1),'browser':'Headless Edge','tested':'18 markers, six filters and route sets, all18 details, route toggle'},indent=2))
