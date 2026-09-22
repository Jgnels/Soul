"""Build Soul's non-UE world overmap v1 and founder-slice SVGs."""
from __future__ import annotations
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATA_OUT = ROOT / "Data" / "soul_world_overmap_v1_20260922.json"
FOUNDER_RUNTIME_OUT = ROOT / "Data" / "soul_founder_slice_runtime_seed_20260922.json"
EVIDENCE_DIR = ROOT / "Evidence" / "WorldOvermap"
FULL_SVG = EVIDENCE_DIR / "soul_world_overmap_v1.svg"
SLICE_SVG = EVIDENCE_DIR / "soul_founder_slice_overmap_v1.svg"

FOUNDER_SLICE = [
    "human_capital", "crossroads", "old_quarry", "river_ford",
    "forest_edge", "ancient_shrine", "orc_watch", "north_pass", "orc_camp",
]

FOUNDER_UE_POSITIONS = {
    "human_capital": [-3000, 0, 50],
    "crossroads": [-1600, 0, 50],
    "old_quarry": [-700, -1500, 50],
    "river_ford": [-200, 1200, 50],
    "forest_edge": [200, -500, 50],
    "ancient_shrine": [700, -2100, 50],
    "orc_watch": [1600, 800, 50],
    "north_pass": [1700, -1300, 50],
    "orc_camp": [3100, 0, 50],
}

FOUNDER_GAMEPLAY = {
    "human_capital": ["settlement","recruitment","tavern","hero_services"],
    "crossroads": ["travel_hub"],
    "old_quarry": ["resource_site","ore"],
    "river_ford": ["crossing","road_gate","encounter"],
    "forest_edge": ["resource_site","wood","ambush_route"],
    "ancient_shrine": ["magic_landmark","hero_progression"],
    "orc_watch": ["enemy_outpost","encounter"],
    "north_pass": ["chokepoint","encounter"],
    "orc_camp": ["enemy_stronghold","siege"],
}

TERRAIN_FEATURES = {
    "seas": [
        {"id":"western_sea","kind":"sea","edge":"west","notes":["Viking and Nature coast access"]},
    ],
    "rivers": [
        {"id":"heart_river","kind":"river","points":[[505,235],[470,360],[455,500],[420,560],[365,720],[330,840]],
         "crossings":["river_ford","southern_crossing"]},
        {"id":"eastern_run","kind":"river","points":[[640,265],[680,400],[690,560],[720,660]],
         "crossings":["orc_broken_bridge"]},
    ],
    "mountain_belts": [
        {"id":"crownspine_range","kind":"mountains","points":[[440,85],[520,180],[610,250],[700,250],[850,150],[930,110]],
         "passes":["viking_snow_pass","mountain_shrine","dwarf_mountain_pass","north_pass"]},
    ],
    "forest_belts": [
        {"id":"greenwood_forest","kind":"forest","polygon":[[50,570],[450,585],[500,880],[60,900]],
         "gates":["coastal_ruins","nature_grassland_edge","southern_crossing"]},
    ],
}

MACRO_REGIONS = [
    {"id":"northern_fjords","name":"Northern Fjords","status":"TEMP_NAME",
     "polygon":[[45,45],[455,45],[520,245],[390,305],[80,285]]},
    {"id":"crownspine","name":"Crownspine","status":"TEMP_NAME",
     "polygon":[[455,45],[950,50],[955,340],[685,365],[520,245]]},
    {"id":"heartland","name":"Heartland","status":"TEMP_NAME",
     "polygon":[[55,280],[595,275],[610,610],[175,660],[45,555]]},
    {"id":"eastern_badlands","name":"Eastern Badlands","status":"TEMP_NAME",
     "polygon":[[575,320],[960,320],[970,665],[710,700],[585,560]]},
    {"id":"greenwood","name":"Greenwood","status":"TEMP_NAME",
     "polygon":[[45,560],[530,585],[555,900],[45,900]]},
    {"id":"ashen_south","name":"Ashen South","status":"TEMP_NAME",
     "polygon":[[505,590],[970,615],[980,900],[520,900]]},
]

def elevation_for(landform, feature):
    high = {"pass","mountain_pass","terrace","terraced_slope","ridge",
            "shrine_terrace","fortified_hold","industrial_approach"}
    low = {"valley","river_valley","cliff_harbour","coast","valley_hill"}
    if landform in high or "mountain" in landform:
        return "high"
    if landform in low or feature in {"river","river_crossing","shore_bridge"}:
        return "low"
    return "mid"

def node(id, name, macro, owner, x, y, biome, landform, feature,
         recipe, kind="region", settlement=None, resource=None, notes=None):
    return {
        "id":id,"name":name,"macro_region":macro,"owner":owner,"x":x,"y":y,
        "biome":biome,"landform":landform,"feature":feature,
        "elevation_band":elevation_for(landform, feature),"kind":kind,
        "settlement_id":settlement,"resource":resource,
        "battle_recipe_hint":recipe,"notes":notes or [],
    }

NODES = [
    node("viking_harbour","Viking Harbour","northern_fjords","vikings",120,140,
         "cold_coast","cliff_harbour","shore_bridge","viking.harbour_edge",
         "capital","city.viking_harbour",notes=["Viking faction seat","Kraken homeland access"]),
    node("viking_fjord_ridge","Fjord Ridge","northern_fjords","vikings",250,100,
         "snow_coast","ridge","water_edge","viking.fjord_ridge"),
    node("viking_forest_track","Forest Track","northern_fjords","vikings",260,220,
         "boreal","wooded_track","village_edge","viking.forest_track"),
    node("viking_snow_pass","Snow Pass","northern_fjords",None,400,170,
         "snow","mountain_pass","narrow_pass","viking.snow_pass",
         notes=["Northern approach into the central mountain spine"]),
    node("mountain_shrine","Mountain Shrine","crownspine",None,500,230,
         "mountain","terrace","shrine","neutral.mountain_shrine","landmark",
         notes=["Neutral north-south route landmark"]),

    node("northwest_march","Northwest March","heartland",None,215,315,
         "temperate","rolling_plain","trade_road","human.rolling_ridge",
         notes=["Neutral transition between the Viking north and Human Heartland"]),

    node("dwarf_high_quarry","High Quarry","crownspine","dwarves",640,130,
         "mountain","terraced_slope","quarry","dwarf.high_quarry",
         resource="ore"),
    node("dwarf_snow_basin","Snow Basin","crownspine","dwarves",760,90,
         "snow_mountain","basin","bowl","dwarf.snow_basin"),
    node("dwarf_forge_approach","Forge Approach","crownspine","dwarves",740,200,
         "mountain","industrial_approach","forge_gate","dwarf.forge_approach"),
    node("dwarf_hold","Dwarf Hold","crownspine","dwarves",850,150,
         "mountain","fortified_hold","great_forge","dwarf.forge_approach",
         "capital","city.dwarf_hold",notes=["Dwarf faction seat","Automaton Colossus homeland"]),
    node("dwarf_mountain_pass","Mountain Pass","crownspine",None,620,260,
         "mountain","pass","switchback","dwarf.mountain_pass"),
    node("human_capital","Human Capital","heartland","humans",180,450,
         "temperate","fortified","capital","human.fortress_outskirts",
         "capital","city.human_capital",notes=["Founder slice start","Human faction seat"]),
    node("crossroads","Crossroads","heartland","humans",320,450,
         "temperate","rolling_plain","crossroads","human.grassland_crossroads",
         notes=["Founder slice strategic hub"]),
    node("old_quarry","Old Quarry","heartland",None,390,330,
         "temperate","ridge","resource","human.rolling_ridge",
         resource="ore",notes=["Founder slice resource site"]),
    node("river_ford","River Ford","heartland",None,420,560,
         "temperate","valley","river_crossing","human.river_road",
         notes=["Founder slice crossing / road gate"]),
    node("forest_edge","Forest Edge","heartland",None,480,420,
         "forest","woodland","grove","nature.grassland_edge",
         resource="wood",notes=["Founder slice flanking corridor"]),
    node("ancient_shrine","Ancient Shrine","heartland",None,520,300,
         "mountain_forest","shrine_terrace","ancient_shrine",
         "neutral.mountain_shrine","landmark",
         notes=["Founder slice magic landmark"]),
    node("orc_watch","Orc Watch","eastern_badlands","orcs",620,540,
         "badlands","mesa","watch","orc.badlands",
         notes=["Founder slice forward Orc position"]),
    node("north_pass","North Pass","eastern_badlands",None,640,360,
         "mountain","pass","narrow_pass","dwarf.mountain_pass",
         notes=["Founder slice alternate stronghold approach"]),
    node("orc_camp","Orc Stronghold","eastern_badlands","orcs",780,460,
         "badlands","ruined_city_edge","stronghold","orc.war_camp",
         "capital","city.orc_ruinhold",notes=["Founder slice enemy objective","Orc faction seat"]),
    node("orc_badlands","Badlands","eastern_badlands","orcs",880,510,
         "badlands","mesa","dry_gully","orc.badlands"),
    node("orc_war_camp","War Camp","eastern_badlands","orcs",860,400,
         "ruined_lowland","open_ruin","camp","orc.war_camp"),
    node("orc_ruined_field","Ruined Field","eastern_badlands","orcs",740,600,
         "temperate_ruins","broken_city_edge","ruins","orc.ruined_field"),
    node("orc_broken_bridge","Broken Bridge","eastern_badlands",None,720,660,
         "ruined_valley","ravine","broken_bridge","orc.broken_bridge",
         notes=["Primary Orc-to-Dark choke"]),

    node("coastal_ruins","Coastal Ruins","greenwood",None,100,600,
         "coast","valley_hill","ruins","neutral.coastal_ruins","landmark"),
    node("nature_shrine","Forest Shrine","greenwood","nature",260,640,
         "mountain_forest","shrine_terrace","ancient_shrine","nature.ancient_shrine"),
    node("nature_forest_clearing","Forest Clearing","greenwood","nature",280,750,
         "forest","clearing","dense_treeline","nature.forest_clearing"),
    node("nature_treehold","Nature Treehold","greenwood","nature",150,800,
         "forest","treehold","great_tree","nature.forest_clearing",
         "capital","city.nature_treehold",notes=["Nature faction seat","Mountain Dragon homeland"]),
    node("nature_river_woodland","River Woodland","greenwood","nature",330,840,
         "forest","river_valley","river","nature.river_woodland"),
    node("nature_grassland_edge","Grassland Edge","greenwood",None,400,690,
         "meadow_edge","rolling_plain","forest_edge","nature.grassland_edge"),
    node("southern_crossing","Southern Crossing","greenwood",None,520,700,
         "temperate","river_valley","road_crossing","neutral.open_grassland",
         notes=["Neutral connector between western forest and southern corruption"]),
    node("dark_corrupted_valley","Corrupted Valley","ashen_south","dark",620,740,
         "corrupted_mountain","valley","mist","dark.corrupted_valley"),
    node("dark_ruined_causeway","Ruined Causeway","ashen_south","dark",730,720,
         "corrupted_ruins","causeway","ruined_bridge","dark.ruined_causeway"),
    node("dark_ash_plain","Ash Plain","ashen_south","dark",730,840,
         "ash_waste","open_plain","dead_ground","dark.ash_plain"),
    node("dark_castle_approach","Castle Approach","ashen_south","dark",840,760,
         "corrupted","fortress_approach","alien_castle","dark.castle_approach"),
    node("dark_fortress","Dark Fortress","ashen_south","dark",900,840,
         "corrupted","fortress","throne_sanctum","dark.castle_approach",
         "capital","city.dark_fortress",notes=["Dark faction seat","Fantasy Dragon homeland"]),
]

def edge(a, b, route="trail", cost=8, road=False, choke=False, notes=None):
    return {
        "a":a,"b":b,"route":route,
        "action_cost":1,
        "logistics_movement_cost":cost,
        "movement_cost":cost,
        "road":road,"chokepoint":choke,"notes":notes or [],
    }

EDGES = [
    # Founder slice topology is copied exactly from the current playtest prototype.
    edge("human_capital","crossroads","road",6,True),
    edge("crossroads","old_quarry","road",6,True),
    edge("crossroads","river_ford","road",6,True),
    edge("crossroads","forest_edge","trail",8),
    edge("old_quarry","ancient_shrine","trail",8),
    edge("river_ford","orc_watch","road",6,True,True),
    edge("forest_edge","orc_watch","trail",8),
    edge("forest_edge","north_pass","trail",8,False,True),
    edge("north_pass","orc_camp","pass",10,False,True),
    edge("orc_watch","orc_camp","road",6,True),
    # Northern/Viking routes.
    edge("viking_harbour","viking_forest_track","road",6,True),
    edge("viking_harbour","viking_fjord_ridge","coast_road",7,True),
    edge("viking_forest_track","viking_snow_pass","trail",8),
    edge("viking_fjord_ridge","viking_snow_pass","ridge_trail",9),
    edge("viking_forest_track","northwest_march","trade_road",7,True),
    edge("northwest_march","human_capital","trade_road",7,True),
    edge("viking_snow_pass","mountain_shrine","pass",10,False,True),

    # Crownspine/Dwarf routes.
    edge("mountain_shrine","ancient_shrine","mountain_trail",9),
    edge("mountain_shrine","dwarf_mountain_pass","pass",10,False,True),
    edge("dwarf_mountain_pass","dwarf_high_quarry","road",6,True),
    edge("dwarf_high_quarry","dwarf_snow_basin","trail",8),
    edge("dwarf_high_quarry","dwarf_forge_approach","road",6,True),
    edge("dwarf_forge_approach","dwarf_hold","road",6,True),
    edge("dwarf_snow_basin","dwarf_hold","high_pass",10,False,True),
    edge("dwarf_mountain_pass","north_pass","pass",10,False,True),

    edge("dwarf_forge_approach","north_pass","old_dwarf_road",9,False,True,
         notes=["Secondary eastern mountain route; keeps Dwarf access from depending on one quarry pass"]),

    # Eastern/Orc depth.
    edge("north_pass","orc_war_camp","ridge_road",8,True),
    edge("orc_camp","orc_badlands","road",6,True),
    edge("orc_camp","orc_war_camp","road",6,True),
    edge("orc_watch","orc_ruined_field","trail",8),
    edge("orc_badlands","orc_ruined_field","gully_trail",8),
    edge("orc_ruined_field","orc_broken_bridge","road",6,True,True),
    # Greenwood/Nature routes.
    edge("human_capital","coastal_ruins","coast_road",7,True),
    edge("coastal_ruins","nature_shrine","forest_road",7,True),
    edge("nature_shrine","nature_forest_clearing","trail",8),
    edge("nature_shrine","nature_grassland_edge","trail",8),
    edge("nature_forest_clearing","nature_treehold","trail",8),
    edge("nature_forest_clearing","nature_river_woodland","trail",8),
    edge("nature_river_woodland","nature_treehold","river_trail",8),
    edge("nature_grassland_edge","nature_river_woodland","meadow_trail",8,
         notes=["Secondary Nature loop; avoids a single forest-gate dependency"]),
    edge("nature_grassland_edge","river_ford","road",6,True),
    edge("nature_grassland_edge","southern_crossing","road",6,True),
    edge("river_ford","southern_crossing","road",6,True),

    # Southern/Dark routes.
    edge("southern_crossing","orc_broken_bridge","trail",8),
    edge("southern_crossing","dark_corrupted_valley","road",7,True),
    edge("orc_broken_bridge","dark_ruined_causeway","causeway",8,True,True),
    edge("dark_corrupted_valley","dark_ruined_causeway","trail",8),
    edge("dark_corrupted_valley","dark_ash_plain","trail",8),
    edge("dark_ruined_causeway","dark_castle_approach","road",7,True),
    edge("dark_ash_plain","dark_fortress","road",7,True),
    edge("dark_castle_approach","dark_fortress","road",6,True,True),
]

REFERENCE_PRINCIPLES = [
    "Heroes III: fog/exploration, roads and resource/landmark objects make strategic routes readable.",
    "Total War: Warhammer: terrain constrains movement; settlements anchor territorial control and chokepoints matter.",
    "Bannerlord: continuous geography should visually communicate settlement identity, production, terrain and route logic.",
]

def compass_direction(dx, dy):
    # Map-space y grows southward, so negative dy is north.
    if dx == 0 and dy == 0:
        return "center"
    horiz = "east" if dx > 0 else "west"
    vert = "south" if dy > 0 else "north"
    ax, ay = abs(dx), abs(dy)
    if ax > ay * 1.8:
        return horiz
    if ay > ax * 1.8:
        return vert
    return vert + "_" + horiz

def derived_runtime_regions():
    by_id={n["id"]:n for n in NODES}
    neighbors={k:[] for k in by_id}
    roads={k:[] for k in by_id}
    approaches={k:{} for k in by_id}
    routes={}
    for e in EDGES:
        a,b=e["a"],e["b"]
        neighbors[a].append(b); neighbors[b].append(a)
        if e["road"]:
            roads[a].append(b); roads[b].append(a)
        na,nb=by_id[a],by_id[b]
        if a in FOUNDER_UE_POSITIONS and b in FOUNDER_UE_POSITIONS:
            ax,ay,_=FOUNDER_UE_POSITIONS[a]
            bx,by,_=FOUNDER_UE_POSITIONS[b]
        else:
            ax,ay=na["x"],na["y"]
            bx,by=nb["x"],nb["y"]
        approaches[b][a]=compass_direction(ax-bx, ay-by)
        approaches[a][b]=compass_direction(bx-ax, by-ay)
        rid="route."+"_".join(sorted((a,b)))
        routes[rid]={**e,"id":rid,
                     "approach_into_a":approaches[a][b],
                     "approach_into_b":approaches[b][a]}
    regions={}
    for rid,n in by_id.items():
        regions[rid]={
            **n,
            "neighbors":sorted(neighbors[rid]),
            "road_neighbors":sorted(roads[rid]),
            "approach_from_neighbor":approaches[rid],
        }
        if rid in FOUNDER_UE_POSITIONS:
            regions[rid]["founder_ue_position"]=FOUNDER_UE_POSITIONS[rid]
    return regions,routes

def payload():
    runtime_regions,runtime_routes=derived_runtime_regions()
    return {
        "schema": 1,
        "generated": "2026-09-22",
        "status": "STRUCTURAL_V1_NONCANONICAL_PLACE_NAMES",
        "authority": {
            "founder_slice_topology": "current Soul founder-playtest prototype",
            "settlement_seats": "current city/siege plans",
            "battle_recipe_hints": "Data/battlefield_recipes.json",
            "balance_center": {"action_points_per_day": 3, "normal_travel_cost": 8},
            "travel_semantics": {
                "region_move_action_cost": 1,
                "edge_movement_cost_field": "logistics/readiness/supply movement cost, not action points",
                "road_cost_band": [6, 8],
                "trail_pass_cost_band": [8, 10],
            },
        },
        "design_references": REFERENCE_PRINCIPLES,
        "presentation_contract": [
            "Continuous-looking 3D geography; region graph is logical, not a visible board.",
            "Settlement miniatures/icons should resemble their actual visitable scene silhouettes.",
            "Roads, passes, rivers, forests and badlands must communicate route cost before UI text.",
            "Fog hides unvisited geography; explored but not visible geography remains readable.",
            "Campaign geography directly supplies biome/landform/feature/approach context to battle selection.",
        ],
        "founder_slice": {
            "region_ids": FOUNDER_SLICE,
            "start": "human_capital",
            "enemy_objective": "orc_camp",
            "purpose": "30-60 minute Human-vs-Orc founder micro-campaign",
        },
        "terrain_features": TERRAIN_FEATURES,
        "macro_regions": MACRO_REGIONS,
        "nodes": NODES,
        "edges": EDGES,
        "runtime_regions": runtime_regions,
        "runtime_routes": runtime_routes,
    }

FACTION_STYLE = {
    "humans":("#5d88c7","#dce9f8"), "vikings":("#4e7189","#dbe8ef"),
    "dwarves":("#9a6a3f","#ead7bf"), "orcs":("#8a4a37","#ead1c8"),
    "dark":("#66516f","#ded2e3"), "nature":("#5f7f55","#dce8d7"),
    None:("#6c706f","#e3e3df"),
}

MACRO_FILL = {
    "northern_fjords":"#dbe9ef", "crownspine":"#ded9d2",
    "heartland":"#e8e0c7", "eastern_badlands":"#dbc3a8",
    "greenwood":"#c9dec1", "ashen_south":"#d5c8cd",
}

def esc(text):
    return (str(text).replace("&","&amp;").replace("<","&lt;")
            .replace(">","&gt;").replace('"',"&quot;"))

def svg_map(nodes, edges, title, subtitle, slice_only=False):
    by_id = {n["id"]: n for n in nodes}
    selected = set(FOUNDER_SLICE) if slice_only else set(by_id)
    visible_nodes = [n for n in nodes if n["id"] in selected]
    visible_edges = [e for e in edges if e["a"] in selected and e["b"] in selected]
    def coord(n):
        if slice_only and n["id"] in FOUNDER_UE_POSITIONS:
            x,y,_=FOUNDER_UE_POSITIONS[n["id"]]
            return x,y
        return n["x"],n["y"]
    if slice_only:
        xy=[coord(n) for n in visible_nodes]
        xs=[v[0] for v in xy]; ys=[v[1] for v in xy]
        minx,maxx,miny,maxy=min(xs)-500,max(xs)+500,min(ys)-500,max(ys)+500
    else:
        minx,maxx,miny,maxy=0,1000,0,950
    width,height=1400,980
    sx=lambda x: 65+(x-minx)/(maxx-minx)*1260
    sy=lambda y: 110+(y-miny)/(maxy-miny)*790
    out=[f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">']
    out.append("""<defs>
      <pattern id="forestPattern" width="34" height="30" patternUnits="userSpaceOnUse">
        <path d="M17 4 L25 18 L21 18 L27 27 L7 27 L13 18 L9 18 Z" fill="#5f7f55" opacity="0.22"/>
      </pattern>
      <pattern id="mountainPattern" width="48" height="38" patternUnits="userSpaceOnUse">
        <path d="M2 34 L17 10 L27 25 L36 7 L47 34 Z" fill="none" stroke="#756b62" stroke-width="2" opacity="0.23"/>
      </pattern>
      <pattern id="badlandsPattern" width="50" height="32" patternUnits="userSpaceOnUse">
        <path d="M5 24 Q15 8 25 24 T45 24" fill="none" stroke="#9b6b47" stroke-width="3" opacity="0.20"/>
      </pattern>
      <pattern id="ashPattern" width="26" height="26" patternUnits="userSpaceOnUse">
        <circle cx="5" cy="8" r="2" fill="#66516f" opacity="0.15"/>
        <circle cx="18" cy="18" r="2.5" fill="#66516f" opacity="0.12"/>
      </pattern>
      <filter id="softShadow" x="-30%" y="-30%" width="160%" height="160%">
        <feDropShadow dx="0" dy="2" stdDeviation="2" flood-color="#332d25" flood-opacity="0.28"/>
      </filter>
    </defs>""")
    out.append('<rect width="1400" height="980" fill="#f2ead8"/>')
    out.append('<rect x="36" y="36" width="1328" height="908" rx="18" fill="#e9dfc8" stroke="#4b463b" stroke-width="3"/>')
    out.append(f'<text x="700" y="72" text-anchor="middle" font-family="Georgia" font-size="32" font-weight="700" fill="#2d2a25">{esc(title)}</text>')
    out.append(f'<text x="700" y="99" text-anchor="middle" font-family="Arial" font-size="15" fill="#5a544a">{esc(subtitle)}</text>')

    if not slice_only:
        for m in MACRO_REGIONS:
            pts=" ".join(f"{sx(x):.1f},{sy(y):.1f}" for x,y in m["polygon"])
            out.append(f'<polygon points="{pts}" fill="{MACRO_FILL[m["id"]]}" fill-opacity="0.78" stroke="#a89d87" stroke-width="2"/>')
            cx=sum(p[0] for p in m["polygon"])/len(m["polygon"])
            cy=sum(p[1] for p in m["polygon"])/len(m["polygon"])
            out.append(f'<text x="{sx(cx):.1f}" y="{sy(cy):.1f}" text-anchor="middle" font-family="Georgia" font-size="20" font-style="italic" fill="#6a6255" opacity="0.72">{esc(m["name"])}</text>')

        pattern_for={"crownspine":"mountainPattern","eastern_badlands":"badlandsPattern",
                     "greenwood":"forestPattern","ashen_south":"ashPattern"}
        for m in MACRO_REGIONS:
            pat=pattern_for.get(m["id"])
            if not pat:
                continue
            pts=" ".join(f"{sx(x):.1f},{sy(y):.1f}" for x,y in m["polygon"])
            out.append(f'<polygon points="{pts}" fill="url(#{pat})" stroke="none"/>')

        # Stylized western sea and the two major rivers are presentation hints, not topology authority.
        out.append('<path d="M36,115 C70,180 55,300 70,395 C85,505 46,620 65,735 C80,835 55,900 36,944 L36,115 Z" fill="#a8c7d5" opacity="0.95"/>')
        out.append(f'<path d="M {sx(505):.1f},{sy(235):.1f} C {sx(470):.1f},{sy(360):.1f} {sx(455):.1f},{sy(500):.1f} {sx(420):.1f},{sy(560):.1f} S {sx(360):.1f},{sy(760):.1f} {sx(330):.1f},{sy(840):.1f}" fill="none" stroke="#7eaac0" stroke-width="10" opacity="0.75"/>')
        out.append(f'<path d="M {sx(640):.1f},{sy(265):.1f} C {sx(680):.1f},{sy(400):.1f} {sx(690):.1f},{sy(560):.1f} {sx(720):.1f},{sy(660):.1f}" fill="none" stroke="#7eaac0" stroke-width="7" opacity="0.55"/>')

    for e in visible_edges:
        a,b=by_id[e["a"]],by_id[e["b"]]
        ax,ay=coord(a); bx,by=coord(b)
        x1,y1,x2,y2=sx(ax),sy(ay),sx(bx),sy(by)
        founder = e["a"] in FOUNDER_SLICE and e["b"] in FOUNDER_SLICE
        color="#6b5130" if e["road"] else "#786d5d"
        width_line=5 if e["road"] else 3
        dash="" if e["road"] else ' stroke-dasharray="9 7"'
        if founder and not slice_only:
            color="#8d622d"; width_line+=1
        out.append(f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" stroke="{color}" stroke-width="{width_line}" stroke-linecap="round"{dash} opacity="0.88"/>')
        if e["chokepoint"]:
            mx,my=(x1+x2)/2,(y1+y2)/2
            out.append(f'<circle cx="{mx:.1f}" cy="{my:.1f}" r="7" fill="#f2ead8" stroke="#6b5130" stroke-width="3"/>')

    for n in visible_nodes:
        nx,ny=coord(n)
        x,y=sx(nx),sy(ny)
        stroke,fill=FACTION_STYLE.get(n["owner"],FACTION_STYLE[None])
        if n["kind"]=="capital":
            r=14 if not slice_only else 17
            out.append(f'<rect x="{x-r:.1f}" y="{y-r:.1f}" width="{2*r}" height="{2*r}" rx="4" fill="{fill}" stroke="{stroke}" stroke-width="5"/>')
            out.append(f'<path d="M{x-r+2:.1f},{y-r+2:.1f} L{x:.1f},{y-r-9:.1f} L{x+r-2:.1f},{y-r+2:.1f}" fill="{stroke}" opacity="0.9"/>')
        elif n["kind"]=="landmark":
            out.append(f'<polygon points="{x:.1f},{y-11:.1f} {x+10:.1f},{y+8:.1f} {x-10:.1f},{y+8:.1f}" fill="{fill}" stroke="{stroke}" stroke-width="4"/>')
        else:
            rr=9 if not slice_only else 11
            out.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="{rr}" fill="{fill}" stroke="{stroke}" stroke-width="4"/>')
        if n.get("resource"):
            out.append(f'<circle cx="{x+12:.1f}" cy="{y-12:.1f}" r="5" fill="#d2a733" stroke="#6f5620" stroke-width="2"/>')
        dy=-22 if n["kind"]=="capital" else -16
        out.append(f'<text x="{x:.1f}" y="{y+dy:.1f}" text-anchor="middle" font-family="Arial" font-size="{16 if slice_only else 13}" font-weight="700" fill="#2e2a24" paint-order="stroke" stroke="#f2ead8" stroke-width="4">{esc(n["name"])}</text>')

    if not slice_only:
        out.append('<rect x="70" y="835" width="415" height="82" rx="9" fill="#f6efdf" stroke="#786d5d" stroke-width="2" opacity="0.96"/>')
        out.append('<text x="88" y="860" font-family="Arial" font-size="15" font-weight="700" fill="#2d2a25">Legend</text>')
        out.append('<line x1="90" y1="882" x2="145" y2="882" stroke="#6b5130" stroke-width="5"/><text x="155" y="887" font-family="Arial" font-size="14" fill="#3b3730">Road / cheaper travel</text>')
        out.append('<line x1="300" y1="882" x2="355" y2="882" stroke="#786d5d" stroke-width="3" stroke-dasharray="9 7"/><text x="365" y="887" font-family="Arial" font-size="14" fill="#3b3730">Trail / pass</text>')
        out.append('<text x="88" y="910" font-family="Arial" font-size="13" fill="#5a544a">Square = faction seat · gold dot = resource · ring = strategic choke · orange routes = founder slice</text>')
    out.append('</svg>')
    return "\n".join(out)

def main():
    DATA_OUT.parent.mkdir(parents=True, exist_ok=True)
    EVIDENCE_DIR.mkdir(parents=True, exist_ok=True)
    data=payload()
    DATA_OUT.write_text(json.dumps(data,indent=2)+"\n",encoding="utf-8")
    founder_ids=set(FOUNDER_SLICE)
    founder_regions={}
    for k,v in data["runtime_regions"].items():
        if k not in founder_ids:
            continue
        region=dict(v)
        region["neighbors"]=[x for x in v["neighbors"] if x in founder_ids]
        region["road_neighbors"]=[x for x in v["road_neighbors"] if x in founder_ids]
        region["approach_from_neighbor"]={
            x:direction for x,direction in v["approach_from_neighbor"].items()
            if x in founder_ids
        }
        region["site_roles"]=FOUNDER_GAMEPLAY[k]
        founder_regions[k]=region
    founder_routes={
        k:v for k,v in data["runtime_routes"].items()
        if v["a"] in founder_ids and v["b"] in founder_ids
    }
    founder_seed={
        "schema":1,
        "generated":"2026-09-22",
        "source":"Soul world overmap structural v1",
        "start_region":"human_capital",
        "enemy_region":"orc_camp",
        "regions":founder_regions,
        "routes":founder_routes,
    }
    FOUNDER_RUNTIME_OUT.write_text(json.dumps(founder_seed,indent=2)+"\n",encoding="utf-8")
    FULL_SVG.write_text(
        svg_map(NODES,EDGES,"Soul World Overmap — Structural V1",
                "Continuous strategic geography; temporary macro-region names; founder slice highlighted"),
        encoding="utf-8")
    SLICE_SVG.write_text(
        svg_map(NODES,EDGES,"Soul Founder Slice — Human–Orc Frontier",
                "9-region micro-campaign topology recovered from the current founder-playtest prototype",
                slice_only=True),
        encoding="utf-8")
    print("WROTE",DATA_OUT)
    print("WROTE",FOUNDER_RUNTIME_OUT)
    print("WROTE",FULL_SVG)
    print("WROTE",SLICE_SVG)
    print("nodes",len(NODES),"edges",len(EDGES),"founder",len(FOUNDER_SLICE))

if __name__=="__main__":
    main()
