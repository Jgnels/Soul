from pathlib import Path
import json
s=Path(__file__).parent;p=(s/'dress_mesa.py').read_text().replace('M_MesaCampaignGround','M_WaterfrontGround').replace('M_MesaCampaignWater','M_WaterfrontWater')
poly=json.loads((s.parent/'TerrainWork/evil-waterfront.json').read_text())['polygon_uv']
code='float2 q=(P.xy+75000)/150000; float dist=10; bool inside=false;'
for a,b in zip(poly,poly[1:]+poly[:1]):
 code+=f'{{float2 a=float2({a[0]},{a[1]}),b=float2({b[0]},{b[1]}); float2 d=b-a; float t=saturate(dot(q-a,d)/dot(d,d)); dist=min(dist,length(q-a-t*d)); if((a.y>q.y)!=(b.y>q.y)){{if(q.x<(b.x-a.x)*(q.y-a.y)/(b.y-a.y)+a.x)inside=!inside;}}}}'
code+='float w=smoothstep(0,25.0/1500.0,dist)*(inside?1:0); return float3(max(B.r,w),B.g*(1-w),0);'
p=p.replace("mesa=tex(m,","biome=custom(m,"+repr(code)+",{'P':pos,'B':biome})\nmesa=tex(m,")
(s/'dress_waterfront.py').write_text(p)
p=(s/'capture_mesa.py').read_text().replace("views=[","views=[('evil_harbor',(-45500,-29000,600),-48,-90,52000),",1)
(s/'capture_waterfront.py').write_text(p)
(s/'start_waterfront_capture.py').write_text((s/'start_mesa_capture.py').read_text().replace('capture_mesa.py','capture_waterfront.py').replace('range(26)','range(30)'))
p=(s/'validate_mesa_v2.py').read_text().replace("b=np.load(p/'grass_coast_v2.npy')","b=np.load(p/'mesa_coast_v2.npy')").replace('mesa_coast_v2.png','evil_waterfront.png').replace("z=np.load(p/'mesa_coast_v2.npy')","z=np.load(p/'evil_waterfront.npy')").replace('mesa-transplant-v2.json','evil-waterfront.json').replace('mesa-validation.json','waterfront-validation.json').replace("(.25,.30)","(.19,.312)").replace('mesa-travel-study.png','waterfront-travel-study.png')
(s/'validate_waterfront.py').write_text(p)
