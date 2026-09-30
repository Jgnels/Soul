from pathlib import Path
import numpy as np,json
r=Path('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview');p=r/'TerrainWork';z=np.load(p/'evil_waterfront.npy');q=z[::4,::4];step=1500/2040*4;ny,nx=q.shape
dy,dx=np.gradient(q,step);slope=np.degrees(np.arctan(np.hypot(dx,dy)));yy,xx=np.mgrid[:ny,:nx]
water=q<=0;dist=np.where(water,0.,200.)
for _ in range(70):
 a=np.pad(dist,1,constant_values=200);dist=np.minimum(dist,np.minimum.reduce([a[2:,1:-1],a[:-2,1:-1],a[1:-1,2:],a[1:-1,:-2]])+step)
# Assess nine samples over an approximately 24m footprint, not just a single flat pixel.
a=np.pad(q,4,mode='edge');heights=np.stack([a[4+oy:4+oy+ny,4+ox:4+ox+nx] for oy in [-4,0,4] for ox in [-4,0,4]])
relief=heights.max(axis=0)-heights.min(axis=0);dry=(heights>0).mean(axis=0)
rows=[
('H1','Humans','Crownstead','capital',(.59,.63),'Central market and royal seat on the broad peninsula; space for farms and several approaches.','The center should remain open enough that one fort cannot lock the whole human realm.'),
('H2','Humans','Westhaven','port',(.39,.65),'Western-bank harbor linking the human heartland to the vikings and the main waterway.','Naval travel is a proposal; land supply to the capital must work without it.'),
('H3','Humans','Stonegate','city',(.72,.80),'Frontier market at the human/dwarf foothills; trade and recruitment staging point.','Keep a broad approach and a separate pass fort, rather than making the city an impassable wall.'),
('D1','Dwarves','Highforge','capital',(.84,.65),'Mountain-seat terrace overlooking the human frontier; a defensible forge capital.','Requires a carved terrace and tested pass access; avoid a summit-only placement.'),
('D2','Dwarves','Southdelve','city',(.66,.92),'Southern mining city makes the dwarven wraparound economically and militarily useful.','This should link along the southern foothills, with alternatives to a single bottleneck.'),
('D3','Dwarves','Ironharbor','port',(.88,.86),'Southeastern inlet port gives the mountain realm an outlet beyond its passes.','Verify navigable channel width before committing docks or naval rules.'),
('N1','Nature','Heartgrove','capital',(.40,.22),'Sheltered interior grove between ridges, away from an exposed frontier coast.','Forest dressing belongs around approach clearings; it must not obscure routes.'),
('N2','Nature','Greenreach','port',(.47,.31),'Southern woodland landing facing the central waterway and neighboring realms.','Small river landing rather than a giant stone trading port.'),
('N3','Nature','Lakewatch','city',(.56,.14),'Northern-lake settlement secures the western route around the preserved lake.','Retain room for armies to go around the lake without trespassing through the city.'),
('O1','Orcs','Redmaw','capital',(.87,.085),'Northeastern basin for the main stronghold, backed by hills and multiple expansion directions.','A staging basin is more useful than the highest peak.'),
('O2','Orcs','Ravager Landing','port',(.92,.30),'Southern war-port creates pressure across the central waterway.','Avoid putting every orc route through the lake corridor.'),
('O3','Orcs','Skullpass','city',(.70,.08),'Western muster town guards the approach between orc country and the northern lake.','Keep a usable bypass or secondary front so the whole faction cannot be trapped here.'),
('V1','Vikings','Fjordhall','capital',(.27,.56),'Western fjord-side seat with a protected inland terrace and access to the long waterway.','Capital and harbor can form one district; this marker is the city center.'),
('V2','Vikings','Northwatch','city',(.24,.41),'Northern frontier town watches the crossing toward evil and nature territory.','Cross-water connection is proposed, not an existing bridge.'),
('V3','Vikings','Stormhaven','port',(.08,.84),'Southern harbor extends viking reach along the long coast.','Leave enough hinterland for supplies instead of relying on naval movement alone.'),
('E1','Evil','Cinder Crown','capital',(.08,.21),'Protected mesa interior above the approach to the new coast.','Keep the graded access route outside lethal lava if hazards are introduced.'),
('E2','Evil','Ashport','port',(.195,.312),'New northern-bank waterfront; direct outlet for the formerly enclosed evil region.','Docks would project from dry ground into the existing water; water navigation remains unimplemented.'),
('E3','Evil','Black Gate','city',(.15,.275),'Fortified approach town between the mesa capital and its coastal outlet.','Keep this smaller than the capital; it should guard the route without being the only possible expansion direction.')
]
sites=[]
for sid,faction,name,kind,uv,why,caution in rows:
 d=np.hypot(xx/(nx-1)-uv[0],yy/(ny-1)-uv[1])*1500
 radius=32 if sid=='E2' else 70
 eligible=(d<radius)&(q>1.5)&(dry>=.77)&(slope<22)
 if kind=='port':eligible&=(dist<=24)&(q<9)
 if sid=='V1':eligible&=(dist<=45)
 score=d/radius+relief/8+slope/25
 if kind=='port':score+=dist/8
 if kind=='capital':score+=relief/5
 assert eligible.any(),sid
 iy,ix=np.unravel_index(np.argmin(np.where(eligible,score,1e9)),q.shape);u,v=ix/(nx-1),iy/(ny-1)
 height=float(q[iy,ix]);screen_dist=324500-height*100;scale=960/(np.tan(np.radians(25))*screen_dist)
 site={'id':sid,'faction':faction,'name':name,'kind':kind,'uv':[u,v],'world_cm':[-75000+u*150000,-75000+v*150000,height*100],'screen':[960+(u-.5)*150000*scale,540+(v-.5)*150000*scale],'height_m':height,'slope_deg':float(slope[iy,ix]),'footprint_relief_m':float(relief[iy,ix]),'shore_distance_m_approx':float(dist[iy,ix]),'why':why,'caution':caution,'earthworks':bool(relief[iy,ix]>5)}
 sites.append(site)
print(json.dumps([{k:s[k] for k in ['id','uv','height_m','footprint_relief_m','shore_distance_m_approx']} for s in sites],indent=2))
data={'status':'Proposal only; names are placeholders; no faction simulation changed.','sites':sites,'sources':[{'title':'Total War Academy: Provinces and settlements','url':'https://academy.totalwar.com/campaign-provinces-and-settlements/'},{'title':'TaleWorlds: The Passage of Time','url':'https://www.taleworlds.com/en/Games/Bannerlord/Blog/14'},{'title':'TaleWorlds: Settlement placement and the economy','url':'https://www.taleworlds.com/en/Games/Bannerlord/Blog/99'}]}
(p/'settlement-plan.json').write_text(json.dumps(data,indent=2))
