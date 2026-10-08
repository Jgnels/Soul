import sys,json
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw
R=Path.cwd();E=R/'Evidence/MapFinalPolish-20261007';D=json.loads((E/'Local/routes-art-r1.json').read_text());a={v['id']:v['xy_m'] for v in D['anchors']}
size=1800;im=Image.new('RGB',(size,size),'#141b17');dr=ImageDraw.Draw(im);box=(200,1150,1350,2450)
def xy(p):return ((p[0]-box[0])/(box[2]-box[0])*size,(p[1]-box[1])/(box[3]-box[1])*size)
colors=['#e57373','#90caf9','#fff176','#ce93d8','#80cbc4','#ffb74d','#dce775','#f48fb1']
rows=[]
for i,r in enumerate(D['routes']):
 hit=False
 for s in r['segments']:
  p=np.array(s['points_m']);mask=(p[:,0]>box[0])&(p[:,0]<box[2])&(p[:,1]>box[1])&(p[:,1]<box[3])
  if mask.any():
   dr.line([xy(q) for q in p],fill=colors[i%len(colors)],width=3);v=p[np.flatnonzero(mask)[len(np.flatnonzero(mask))//2]];dr.text(xy(v),str(i),fill='white');hit=True
 if hit:rows.append([i,r['a'],r['b']])
for name,p in a.items():
 q=xy(p)
 if 0<q[0]<size and 0<q[1]<size:dr.ellipse((q[0]-5,q[1]-5,q[0]+5,q[1]+5),fill='white');dr.text(q,name,fill='white')
im.save(E/'Local/human-road-diagnostic.png');print(json.dumps(rows))
