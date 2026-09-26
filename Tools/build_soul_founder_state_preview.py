"""Build a self-contained interactive HTML preview of founder fog/movement state vectors."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "Data"
OUT = ROOT / "Evidence" / "WorldOvermap"
WORLD = json.loads((DATA / "soul_world_overmap_v1_20260922.json").read_text(encoding="utf-8"))
STATES = json.loads((DATA / "soul_founder_presentation_state_vectors_v1_20260922.json").read_text(encoding="utf-8"))
ANCHORS = json.loads((DATA / "soul_overmap_visual_anchors_v1_20260922.json").read_text(encoding="utf-8"))

founder = set(WORLD["founder_slice"]["region_ids"])
nodes = {n["id"]: n for n in WORLD["nodes"] if n["id"] in founder}
anchors = {a["region_id"]: a for a in ANCHORS["anchors"] if a["region_id"] in founder}
routes = {
    rid: r for rid, r in WORLD["runtime_routes"].items()
    if r["a"] in founder and r["b"] in founder
}

payload = {
    "nodes": nodes,
    "anchors": anchors,
    "routes": routes,
    "corridors": STATES["corridors"],
    "start": WORLD["founder_slice"]["start"],
    "objective": WORLD["founder_slice"]["enemy_objective"],
}
template = r'''<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<title>Soul Founder Overmap State Preview</title>
<style>
body{margin:0;background:#17191c;color:#ece8df;font-family:Segoe UI,Arial,sans-serif}
main{display:grid;grid-template-columns:minmax(720px,1fr) 320px;min-height:100vh}
#map{width:100%;height:100vh;background:#292d2d}
aside{padding:18px;background:#202327;border-left:1px solid #444;overflow:auto}
h1{font-size:20px;margin:0 0 14px} label{display:block;margin:12px 0 5px;color:#bbb}
select,input,button{width:100%;box-sizing:border-box;margin:3px 0;padding:8px;background:#30353a;color:#eee;border:1px solid #555}
button{cursor:pointer}.row{display:grid;grid-template-columns:1fr 1fr;gap:8px}.stat{margin:12px 0;padding:10px;background:#292d31}
.route{stroke:#806d51;stroke-width:5;opacity:.22}.route.known{opacity:.58}.route.selectable{stroke:#e6bd56;stroke-width:8;opacity:.95}
.node{stroke-width:4}.visible{fill:#f6f0df;opacity:1}.memory{fill:#a49f92;opacity:.68}.unexplored{fill:#25282c;opacity:.45;stroke:#555!important}
.active{stroke:#ffe36e!important;stroke-width:8!important}.target{stroke:#e78450!important;stroke-width:7!important}
.nodeLabel{font-size:15px;font-weight:650;paint-order:stroke;stroke:#1b1e20;stroke-width:4;fill:#fff}.hiddenLabel{opacity:.18}
.small{font-size:12px;fill:#c8c3b8}.legend{font-size:12px;fill:#ddd}
</style></head><body><main><svg id="map" viewBox="120 220 760 430"></svg><aside>
<h1>Soul Founder Slice</h1>
<label for="corridor">Corridor</label><select id="corridor"></select>
<label for="step">State step</label><input id="step" type="range" min="0" value="0">
<div class="row"><button id="prev">Previous</button><button id="next">Next</button></div>
<div id="summary" class="stat"></div><div id="details"></div>
</aside></main><script>
const DATA = __DATA__;
const NS='http://www.w3.org/2000/svg', svg=document.getElementById('map');
const corridorSel=document.getElementById('corridor'), stepInput=document.getElementById('step');
'''
template += r'''
function E(tag,attrs={}){const e=document.createElementNS(NS,tag);for(const[k,v]of Object.entries(attrs))e.setAttribute(k,v);return e}
function clear(){while(svg.firstChild)svg.removeChild(svg.firstChild)}
function routeClass(id,state){if(state.selectable_route_ids.includes(id))return 'route selectable';if(state.known_route_ids.includes(id))return 'route known';return 'route'}
function nodeClass(id,state){let c='node ';if(state.visible_regions.includes(id))c+='visible';else if(state.explored_not_visible_regions.includes(id))c+=' memory';else c+=' unexplored';if(id===state.active_region)c+=' active';if(id===state.planned_target_region)c+=' target';return c}
function ownerStroke(owner){return({humans:'#73a6d8',orcs:'#a7ad5e'}[owner]||'#9a9a9a')}
function render(){
 const corridor=DATA.corridors[+corridorSel.value], state=corridor.states[+stepInput.value]; clear();
 const bg=E('rect',{x:120,y:220,width:760,height:430,fill:'#343936'});svg.appendChild(bg);
 for(const [id,r] of Object.entries(DATA.routes)){
  const a=DATA.nodes[r.a], b=DATA.nodes[r.b]; const line=E('line',{id,x1:a.x,y1:a.y,x2:b.x,y2:b.y,class:routeClass(id,state)});svg.appendChild(line);
 }
 for(const [id,n] of Object.entries(DATA.nodes)){
  const anchor=DATA.anchors[id], size=anchor.scale_class==='major'?14:(anchor.scale_class==='medium'?11:8);
  const shape=anchor.anchor_type==='major_settlement_silhouette'?E('rect',{x:n.x-size,y:n.y-size,width:size*2,height:size*2,rx:3}):E('circle',{cx:n.x,cy:n.y,r:size});
  shape.setAttribute('class',nodeClass(id,state));shape.setAttribute('stroke',ownerStroke(n.owner));svg.appendChild(shape);
  const label=E('text',{x:n.x+13,y:n.y-10,class:'nodeLabel '+(state.unexplored_regions.includes(id)?'hiddenLabel':'')});label.textContent=n.name;svg.appendChild(label);
 }
'''
template += r'''
 const active=DATA.nodes[state.active_region], focus=E('circle',{cx:active.x,cy:active.y,r:28,fill:'none',stroke:'#ffe36e','stroke-width':2,'stroke-dasharray':'5 4'});svg.appendChild(focus);
 document.getElementById('summary').innerHTML=`<b>${corridor.corridor_id}</b><br>Step ${state.step} · ${state.cumulative_travel_actions} travel actions<br>Active: <b>${active.name}</b><br>Plan: ${state.planned_action} → <b>${DATA.nodes[state.planned_target_region]?.name||state.planned_target_region}</b>`;
 document.getElementById('details').innerHTML=`<b>Visible</b><br>${state.visible_regions.map(x=>DATA.nodes[x].name).join(', ')}<br><br><b>Memory only</b><br>${state.explored_not_visible_regions.map(x=>DATA.nodes[x].name).join(', ')||'—'}<br><br><b>Selectable destinations</b><br>${state.selectable_destinations.map(x=>DATA.nodes[x].name).join(', ')}`;
}
DATA.corridors.forEach((c,i)=>{const o=document.createElement('option');o.value=i;o.textContent=`${c.corridor_id}: ${c.visual_signature}`;corridorSel.appendChild(o)});
function sync(){stepInput.max=DATA.corridors[+corridorSel.value].states.length-1;if(+stepInput.value>+stepInput.max)stepInput.value=stepInput.max;render()}
corridorSel.addEventListener('change',()=>{stepInput.value=0;sync()});stepInput.addEventListener('input',render);
document.getElementById('prev').onclick=()=>{stepInput.value=Math.max(0,+stepInput.value-1);render()};
document.getElementById('next').onclick=()=>{stepInput.value=Math.min(+stepInput.max,+stepInput.value+1);render()};
sync();
</script></body></html>'''

html_text = template.replace('__DATA__', json.dumps(payload, separators=(',', ':')))
out_path = OUT / "soul_founder_state_preview.html"
out_path.write_text(html_text, encoding="utf-8")
manifest = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "BUILT",
    "corridors": len(STATES["corridors"]),
    "states": sum(len(c["states"]) for c in STATES["corridors"]),
    "regions": len(nodes),
    "routes": len(routes),
    "output": str(out_path.relative_to(ROOT)),
}
(OUT / "founder_state_preview_build_manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
print(json.dumps(manifest, indent=2))
