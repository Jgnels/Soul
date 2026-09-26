"""Build interactive browser review for Soul's candidate six-faction campaign start."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORLD = json.loads((ROOT/"Data"/"soul_world_overmap_v1_20260922.json").read_text(encoding="utf-8"))
RUNTIME = json.loads((ROOT/"Data"/"soul_overmap_runtime_import_v1_20260922.json").read_text(encoding="utf-8"))
START = json.loads((ROOT/"Data"/"soul_campaign_start_states_v1_20260922.json").read_text(encoding="utf-8"))
SLOTS = json.loads((ROOT/"Data"/"soul_overmap_settlement_slots_v1_20260922.json").read_text(encoding="utf-8"))
OPENING = json.loads((ROOT/"Evidence"/"WorldOvermap"/"campaign_opening_analysis.json").read_text(encoding="utf-8"))
OUT = ROOT/"Evidence"/"WorldOvermap"/"soul_campaign_start_inspector.html"

data = json.dumps({
    "world": WORLD,
    "runtime": RUNTIME,
    "start": START,
    "slots": SLOTS,
    "opening": OPENING,
}, separators=(",",":")).replace("</","<\\/")
head = """<!doctype html><html><head><meta charset="utf-8"><title>Soul Campaign Start Inspector</title>
<style>
body{margin:0;font-family:Segoe UI,Arial;background:#13171a;color:#e8e3d8}
header{padding:14px 20px;background:#20262b;border-bottom:1px solid #3b444a}
h1{margin:0 0 4px;font-size:23px}.sub{color:#b7c0c5;font-size:13px}
main{display:grid;grid-template-columns:minmax(0,1fr) 350px;gap:12px;padding:12px}
.panel{background:#20262b;border:1px solid #394149;border-radius:10px;padding:12px}
svg{width:100%;height:auto;background:#ddd4bd;border-radius:8px}
.controls{display:flex;gap:12px;flex-wrap:wrap;margin-top:10px}
select{background:#151a1e;color:#eee;border:1px solid #4b555d;padding:5px}
.route{fill:none;stroke:#81725e;opacity:.65}.road{stroke-width:3}.trail{stroke-dasharray:5 4;stroke-width:2}
.node{stroke:#222;stroke-width:1.5;cursor:pointer}.owned{stroke-width:4}.minor{stroke:#f0df9c;stroke-width:2;fill:none}
.value{fill:#e4c75f;stroke:#5a4b20;stroke-width:1.5}.army{fill:#fff;stroke:#161616;stroke-width:1.5}
.label{font-size:11px;fill:#222;font-weight:600;pointer-events:none}.dim{opacity:.12!important}
.kv{display:grid;grid-template-columns:115px 1fr;gap:5px;font-size:13px}.k{color:#9eaaaf}.v{color:#f0ece2}
.small{font-size:12px;color:#b9c2c7}a{color:#9bc9ef}.warn{color:#e3bd6d}
@media(max-width:1000px){main{grid-template-columns:1fr}}
</style></head><body>
<header><h1>Soul Campaign Start Inspector</h1>
<div class="sub">Candidate six-faction sandbox ownership overlay. Geography is stable; this ownership state is not canon.</div>
<div class="controls"><label>Focus faction <select id="focus"><option value="all">All</option>
<option>humans</option><option>vikings</option><option>dwarves</option><option>orcs</option><option>nature</option><option>dark</option></select></label>
<label><input id="neutral" type="checkbox" checked> Neutral regions</label>
<label><input id="affinity" type="checkbox"> Homeland affinity outlines</label>
<label><input id="value" type="checkbox" checked> Value sites</label></div></header>
<main><section class="panel"><svg id="map" viewBox="0 0 1020 950"></svg></section>
<aside class="panel"><h2 style="margin-top:0">Opening state</h2><div id="summary" class="small"></div>
<hr><h3>Selection</h3><div id="detail" class="small">Click a region.</div>
<hr><div class="small"><a href="campaign_opening_analysis.md">Opening audit</a> | <a href="start_state_validation.json">Validation</a></div>
</aside></main><script>const D="""
tail = """;
const world=D.world,runtime=D.runtime,start=D.start,slots=D.slots,opening=D.opening;
const sandbox=start.scenarios.six_faction_sandbox_candidate, owners=sandbox.region_owners;
const svg=document.getElementById('map'),NS='http://www.w3.org/2000/svg';
const colors={humans:'#5d88c7',vikings:'#4e7189',dwarves:'#9a6a3f',orcs:'#8a4a37',nature:'#5f7f55',dark:'#66516f',neutral:'#777b79'};
const macro={northern_fjords:'#dbe9ef',crownspine:'#ded9d2',heartland:'#e8e0c7',eastern_badlands:'#dbc3a8',greenwood:'#c9dec1',ashen_south:'#d5c8cd'};
const slotBy={};slots.slots.forEach(function(s){slotBy[s.region_id]=s;});
const valueSites=sandbox.value_site_candidates;
function el(t,a){const x=document.createElementNS(NS,t);Object.entries(a||{}).forEach(function(p){x.setAttribute(p[0],p[1]);});return x;}
function xy(p){return [p[0]/1000+500,475-p[1]/1000];}
function terrain(){world.macro_regions.forEach(function(m){svg.appendChild(el('polygon',{points:m.polygon.map(function(p){return p.join(',');}).join(' '),fill:macro[m.id]||'#ddd',stroke:'#b4a993','stroke-width':1}));});
(world.terrain_features.rivers||[]).forEach(function(r){svg.appendChild(el('polyline',{points:r.points.map(function(p){return p.join(',');}).join(' '),fill:'none',stroke:'#6e9fbd','stroke-width':8,opacity:.7}));});
(world.terrain_features.forest_belts||[]).forEach(function(f){svg.appendChild(el('polygon',{points:f.polygon.map(function(p){return p.join(',');}).join(' '),fill:'#719565',opacity:.17}));});}
function routes(){Object.values(runtime.routes).forEach(function(r){const p=r.ue_spline_points_cm.map(xy);const d='M '+p[0][0]+' '+p[0][1]+' Q '+p[1][0]+' '+p[1][1]+' '+p[2][0]+' '+p[2][1];svg.appendChild(el('path',{d:d,class:'route '+(r.road?'road':'trail')}));});}
function nodes(){
 Object.entries(runtime.regions).forEach(function(pair){
  const id=pair[0],n=pair[1],p=xy(n.ue_position_cm),owner=owners[id]||'neutral',slot=slotBy[id];
  const g=el('g',{'data-node':id,'data-startowner':owner,'data-affinity':n.owner||'neutral'});
  const c=el('circle',{cx:p[0],cy:p[1],r:slot&&slot.tier==='major'?11:7,fill:owner==='neutral'?'#777b79':colors[owner],class:'node '+(owner!=='neutral'?'owned':'')});
  c.addEventListener('click',function(){detail(id);});g.appendChild(c);
  if(slot&&slot.tier==='minor')g.appendChild(el('rect',{x:p[0]-10,y:p[1]-10,width:20,height:20,rx:3,class:'minor'}));
  if(valueSites[id]){const pts=[[p[0],p[1]-13],[p[0]+7,p[1]],[p[0],p[1]+13],[p[0]-7,p[1]]].map(function(v){return v.join(',');}).join(' ');g.appendChild(el('polygon',{points:pts,class:'value'}));g.dataset.value='1';}
  const fs=Object.entries(sandbox.factions).find(function(x){return x[1].starting_army.region===id;});
  if(fs){const pts=(p[0]-7)+','+(p[1]-16)+' '+(p[0]+7)+','+(p[1]-16)+' '+p[0]+','+(p[1]-29);g.appendChild(el('polygon',{points:pts,class:'army'}));g.dataset.army=fs[0];}
  if(slot||owner!=='neutral'){const t=el('text',{x:p[0]+12,y:p[1]-10,class:'label'});t.textContent=n.display_name;g.appendChild(t);}
  svg.appendChild(g);
 });
}
function detail(id){
 const n=runtime.regions[id],slot=slotBy[id],owner=owners[id]||'neutral',open=Object.values(opening.factions).find(function(x){return x.capital===id||x.minor===id;});
 const vals=[['Region',n.display_name],['Start owner',owner],['Homeland affinity',n.owner||'neutral'],['Settlement',slot?slot.tier+' / '+(slot.minor_kind||'major'):'-'],['Value site',valueSites[id]?valueSites[id].theme:'-'],['Biome',n.biome],['Feature',n.feature],['Battle recipe',n.battle_recipe_hint],['Opening role',open?('capital '+open.capital+' / minor '+open.minor):'-']];
 let h='<div class="kv">';vals.forEach(function(v){h+='<div class="k">'+v[0]+'</div><div class="v">'+v[1]+'</div>';});h+='</div>';document.getElementById('detail').innerHTML=h;
}
function apply(){
 const f=document.getElementById('focus').value,showNeutral=document.getElementById('neutral').checked,showAffinity=document.getElementById('affinity').checked,showValue=document.getElementById('value').checked;
 document.querySelectorAll('[data-node]').forEach(function(g){
  const start=g.dataset.startowner,aff=g.dataset.affinity;let dim=false;
  if(f!=='all'&&start!==f&&aff!==f)dim=true;
  if(!showNeutral&&start==='neutral')dim=true;
  g.classList.toggle('dim',dim);
  const c=g.querySelector('circle');c.setAttribute('stroke',showAffinity&&aff!=='neutral'?colors[aff]:'#222');
  const v=g.querySelector('.value');if(v)v.style.display=showValue?'':'none';
 });
}
['focus','neutral','affinity','value'].forEach(function(id){document.getElementById(id).addEventListener('change',apply);});
terrain();routes();nodes();apply();
let s='<b>12 owned regions / 24 neutral</b><br>Each faction: capital + aligned minor settlement.<br><br>';
Object.entries(opening.factions).forEach(function(p){const a=p[1];s+=p[0]+': '+a.neutral_frontier_count+' neutral choices; nearest enemy capital '+a.nearest_enemy_capital[0]+' ('+a.nearest_enemy_capital[1]+' moves)<br>';});
s+='<br><span class="warn">Army/garrison numbers and economy IDs remain balance-owned.</span>';
document.getElementById('summary').innerHTML=s;
</script></body></html>
"""
OUT.write_text(head + data + tail,encoding="utf-8")
print("WROTE",OUT)
