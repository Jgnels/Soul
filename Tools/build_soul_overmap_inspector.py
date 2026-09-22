"""Build an interactive local browser inspector for Soul's overmap."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORLD = json.loads((ROOT/"Data"/"soul_world_overmap_v1_20260922.json").read_text(encoding="utf-8"))
IMPORT = json.loads((ROOT/"Data"/"soul_overmap_runtime_import_v1_20260922.json").read_text(encoding="utf-8"))
SIM = json.loads((ROOT/"Evidence"/"WorldOvermap"/"founder_slice_simulation.json").read_text(encoding="utf-8"))
OUT = ROOT/"Evidence"/"WorldOvermap"/"soul_world_overmap_inspector.html"

payload = json.dumps({"world":WORLD,"runtime":IMPORT,"simulation":SIM}, separators=(",",":"))
payload = payload.replace("</", "<\\/")

head = """<!doctype html><html><head><meta charset="utf-8"><title>Soul Overmap Inspector</title>
<style>
body{margin:0;font-family:Segoe UI,Arial;background:#14181b;color:#e8e3d7}
header{padding:14px 20px;background:#20262b;border-bottom:1px solid #394149}
h1{margin:0 0 5px;font-size:23px}.sub{color:#b9c2c7}
main{display:grid;grid-template-columns:minmax(0,1fr) 330px;gap:12px;padding:12px}
.panel{background:#20262b;border:1px solid #394149;border-radius:10px;padding:12px}
.controls{display:flex;gap:14px;flex-wrap:wrap;margin-top:10px;align-items:center}
label{font-size:13px;color:#d1d7da}select{background:#151a1e;color:#eee;border:1px solid #4b555d;padding:5px}
svg{width:100%;height:auto;background:#ddd4bd;border-radius:8px}
"""
head += """
.route{fill:none;stroke:#7b7060;opacity:.82}.road{stroke:#806c51}.trail{stroke-dasharray:5 4}
.choke{stroke:#c54f3f!important}.dim{opacity:.12!important}.founderEdge{stroke:#e1b84b!important;stroke-width:4!important}
.node{cursor:pointer;stroke:#262626;stroke-width:1.5}.capital{stroke-width:3}
.founderNode{stroke:#e1b84b!important;stroke-width:4!important}.resource{stroke:#e3c35b;stroke-width:4}
.label{font-size:12px;fill:#1e2327;pointer-events:none;font-weight:600}
.small{font-size:12px;color:#b9c2c7}.kv{display:grid;grid-template-columns:110px 1fr;gap:5px;font-size:13px}
.k{color:#9eaaaf}.v{color:#f0ece2;word-break:break-word}a{color:#9bc9ef}
@media(max-width:1000px){main{grid-template-columns:1fr}}
</style></head><body><header><h1>Soul World Overmap Inspector</h1>
<div class="sub">Continuous-world campaign structure — local non-UE review</div>
<div class="controls">
<label>Faction <select id="owner"><option value="all">All</option><option>humans</option><option>vikings</option>
<option>dwarves</option><option>orcs</option><option>nature</option><option>dark</option><option value="neutral">neutral</option></select></label>
<label><input type="checkbox" id="founder"> Founder slice only</label>
<label><input type="checkbox" id="roads"> Roads only</label>
<label><input type="checkbox" id="resources"> Resources only</label>
<label><input type="checkbox" id="chokes"> Highlight chokepoints</label>
</div></header><main><section class="panel"><svg id="map" viewBox="0 0 1020 950"></svg></section>
<aside class="panel"><h2 style="margin-top:0">Selection</h2><div id="detail" class="small">Click a region.</div>
<hr><h3>Founder gate</h3><div id="sim" class="small"></div>
<hr><div class="small"><a href="founder_slice_simulation.md">Traversal report</a> ·
<a href="route_analysis.md">Route analysis</a> · <a href="runtime_import_validation.json">Runtime validation</a></div>
</aside></main><script>const D="""
tail = """;
const world=D.world,runtime=D.runtime,sim=D.simulation,svg=document.getElementById('map'),NS='http://www.w3.org/2000/svg';
const founder=new Set(world.founder_slice.region_ids);
const ownerColors={humans:'#5d88c7',vikings:'#4e7189',dwarves:'#9a6a3f',orcs:'#8a4a37',dark:'#66516f',nature:'#5f7f55',neutral:'#777b79'};
const macroColors={northern_fjords:'#dbe9ef',crownspine:'#ded9d2',heartland:'#e8e0c7',eastern_badlands:'#dbc3a8',greenwood:'#c9dec1',ashen_south:'#d5c8cd'};
function el(tag,attrs){const x=document.createElementNS(NS,tag);Object.entries(attrs||{}).forEach(function(kv){x.setAttribute(kv[0],kv[1]);});return x;}
function mapFromUE(p){return [p[0]/1000+500,475-p[1]/1000];}
function addTerrain(){
 world.macro_regions.forEach(function(m){svg.appendChild(el('polygon',{points:m.polygon.map(function(p){return p.join(',');}).join(' '),fill:macroColors[m.id]||'#ddd',stroke:'#b8ad98','stroke-width':1}));});
 (world.terrain_features.rivers||[]).forEach(function(r){svg.appendChild(el('polyline',{points:r.points.map(function(p){return p.join(',');}).join(' '),fill:'none',stroke:'#6e9fbd','stroke-width':8,opacity:.7}));});
 (world.terrain_features.forest_belts||[]).forEach(function(f){svg.appendChild(el('polygon',{points:f.polygon.map(function(p){return p.join(',');}).join(' '),fill:'#789a6d',opacity:.18}));});
 (world.terrain_features.mountain_belts||[]).forEach(function(m){svg.appendChild(el('polyline',{points:m.points.map(function(p){return p.join(',');}).join(' '),fill:'none',stroke:'#77736f','stroke-width':20,opacity:.22}));});
}
function drawRoutes(){
 Object.entries(runtime.routes).forEach(function(pair){
  const id=pair[0],r=pair[1],pts=r.ue_spline_points_cm.map(mapFromUE);
  const d='M '+pts[0][0]+' '+pts[0][1]+' Q '+pts[1][0]+' '+pts[1][1]+' '+pts[2][0]+' '+pts[2][1];
  const path=el('path',{d:d,class:'route '+(r.road?'road':'trail'),'stroke-width':r.road?3:2,'data-id':id,'data-a':r.a,'data-b':r.b,'data-road':r.road?'1':'0','data-choke':r.chokepoint?'1':'0'});
  if(founder.has(r.a)&&founder.has(r.b))path.dataset.founder='1';svg.appendChild(path);
 });
}
"""
tail += """
function drawNodes(){
 Object.entries(runtime.regions).forEach(function(pair){
  const id=pair[0],n=pair[1],xy=mapFromUE(n.ue_position_cm),owner=n.owner||'neutral';
  const g=el('g',{'data-node':id,'data-owner':owner,'data-resource':n.resource?'1':'0'});
  if(founder.has(id))g.dataset.founder='1';
  const circle=el('circle',{cx:xy[0],cy:xy[1],r:n.anchor_type==='settlement_proxy'?11:7,fill:ownerColors[owner]||'#777',class:'node '+(n.anchor_type==='settlement_proxy'?'capital ':'')+(n.resource?'resource':'')});
  circle.addEventListener('click',function(){showDetail(id);});g.appendChild(circle);
  if(n.anchor_type==='settlement_proxy'||founder.has(id)){const t=el('text',{x:xy[0]+12,y:xy[1]-10,class:'label'});t.textContent=n.display_name;g.appendChild(t);}
  svg.appendChild(g);
 });
}
function showDetail(id){
 const n=runtime.regions[id];
 const pairs=[['Region',n.display_name],['ID',id],['Faction',n.owner||'neutral'],['Macro',n.macro_region],['Biome',n.biome],['Landform',n.landform],['Feature',n.feature],['Resource',n.resource||'—'],['Settlement',n.settlement_id||'—'],['Battle recipe',n.battle_recipe_hint],['Neighbors',n.neighbors.join(', ')],['UE cm',n.ue_position_cm.join(', ')]];
 let h='<div class="kv">';pairs.forEach(function(p){h+='<div class="k">'+p[0]+'</div><div class="v">'+p[1]+'</div>';});h+='</div>';
 document.getElementById('detail').innerHTML=h;
}
function apply(){
 const owner=document.getElementById('owner').value,fonly=document.getElementById('founder').checked,roads=document.getElementById('roads').checked,res=document.getElementById('resources').checked,ch=document.getElementById('chokes').checked;
 document.querySelectorAll('.route').forEach(function(x){
  let dim=(fonly&&x.dataset.founder!=='1')||(roads&&x.dataset.road!=='1');
  if(owner!=='all'){const hit=[x.dataset.a,x.dataset.b].some(function(id){return (runtime.regions[id].owner||'neutral')===owner;});if(!hit)dim=true;}
  x.classList.toggle('dim',dim);x.classList.toggle('choke',ch&&x.dataset.choke==='1');x.classList.toggle('founderEdge',fonly&&x.dataset.founder==='1');
 });
"""
tail += """
 document.querySelectorAll('[data-node]').forEach(function(g){
  let dim=(fonly&&g.dataset.founder!=='1')||(res&&g.dataset.resource!=='1');
  if(owner!=='all'&&g.dataset.owner!==owner)dim=true;
  g.classList.toggle('dim',dim);
  g.querySelector('circle').classList.toggle('founderNode',fonly&&g.dataset.founder==='1');
 });
}
['owner','founder','roads','resources','chokes'].forEach(function(id){document.getElementById(id).addEventListener('change',apply);});
addTerrain();drawRoutes();drawNodes();apply();
document.getElementById('sim').innerHTML=
 '<b>'+sim.shortest_approach_count+' equal-action approaches</b><br>'+
 'Minimum battle commitment: '+sim.minimum_actions_including_battle+' actions<br>'+
 'Final approaches: '+sim.final_approach_regions.map(function(x){return runtime.regions[x].display_name;}).join(' / ')+'<br>'+
 'Quarry detour: '+sim.resource_detours.quarry_resource_loop.days_at_3_ap+' days at 3 AP/day';
</script></body></html>
"""
OUT.write_text(head + payload + tail, encoding="utf-8")
print("WROTE", OUT)
