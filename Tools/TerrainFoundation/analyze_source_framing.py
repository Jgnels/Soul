"""Read-only source-window comparison. Never export a heightfield or import UE data.

Test whether a different framing of the same conventional owned height samples
improves capital hinterlands and original-shoreline topology. All output remains
analytical. No cropped source is presumed to be a better production foundation.
"""
from pathlib import Path
import json,math
import numpy as np
from PIL import Image
import analyze_fit as fit
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'Evidence/TerrainFoundation-20261007/Local/source-framing';OUT.mkdir(parents=True,exist_ok=True)
original=fit.native.copy();summaries=[]
windows=[('northwest',0,0),('northeast',510,0),('southwest',0,510),('southeast',510,510),('centre',255,255)]
for name,x0,y0 in windows:
 # A 1531-vertex crop covers exactly 6120 native metres at 4 m spacing.
 # Resampling here is analysis-only; compensate the old analyzer's 8160 divisor
 # so uniform physical XYZ scaling uses the actual source-window width.
 crop=original[y0:y0+1531,x0:x0+1531]
 resized=np.array(Image.fromarray(crop).resize((2041,2041),Image.Resampling.BILINEAR))
 fit.native=resized*(8160/6120);fit.OUT=OUT/name;fit.OUT.mkdir(exist_ok=True)
 for turn in range(4):
  result=fit.analyze(3500,turn,0)
  result['source']=f'Mountain05 {name} 6120 m source window; analysis-only bilinear resampling, no exported heightfield'
  result['native_window_origin_m']=[x0*4,y0*4];result['native_window_side_m']=6120
  (fit.OUT/f'mountain05-3500-rot{turn}-sea0-fit-r2.json').write_text(json.dumps(result,indent=2))
  z=np.rot90(resized,turn)*3500/6120;dy,dx=np.gradient(z,3500/2040);slope=np.degrees(np.arctan(np.hypot(dx,dy)));yy,xx=np.indices(z.shape);X=xx*3500/2040;Y=yy*3500/2040
  capital_metrics={}
  for key in ['human_capital','dwarf_hold','viking_harbour']:
   a=next(a for a in result['anchors'] if a['id']==key);circle=(X-a['xy_m'][0])**2+(Y-a['xy_m'][1])**2<=200**2
   capital_metrics[key]={'xy_m':a['xy_m'],'height_above_native_water_m':a['height_m'],'hinterland_dry_below15_fraction':float(np.mean((z[circle]>0)&(slope[circle]<15))),'height_p05_p95_m':np.percentile(z[circle],[5,95]).tolist(),'radial_footprint_relief_m':a['radial_relief_m']}
  summary={'window':name,'native_window_origin_m':[x0*4,y0*4],'native_window_side_m':6120,'rotation_quarters':turn,'physical_side_m':3500,'water_datum':'unchanged original native','dry_paths_found':result['dry_success_count'],'dry_failures':result['dry_failures'],'capital_metrics':capital_metrics,'analysis_only':True,'heightfield_exported':False,'limitations':['Coarse 13.7 m search does not establish dense grade, bridge, drainage or rendered quality.','Crop removes source geography; no crop is accepted merely for a higher route count.']}
  (fit.OUT/f'framing-rot{turn}.json').write_text(json.dumps(summary,indent=2));summaries.append(summary)
  (OUT/'summary.json').write_text(json.dumps(summaries,indent=2))
  print('FRAMING',name,turn,result['dry_success_count'],capital_metrics['human_capital']['hinterland_dry_below15_fraction'],flush=True)
print('FRAMING_COMPLETE',len(summaries))
