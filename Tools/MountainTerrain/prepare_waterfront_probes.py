from pathlib import Path
import json,numpy as np
r=Path(__file__).resolve().parents[1];z=np.load(r/'TerrainWork/evil_waterfront.npy');points=[]
for y in range(100,2041,180):
    for x in range(100,2041,180):points.append({'x':-75000+x*150000/2040,'y':-75000+y*150000/2040,'expected_z':round(float(z[y,x])*200)/2})
(r/'TerrainWork/waterfront-collision-probes.json').write_text(json.dumps(points))
