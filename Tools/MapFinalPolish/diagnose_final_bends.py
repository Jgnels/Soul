import sys,json
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw
R=Path.cwd();E=R/'Evidence/MapFinalPolish-20261007';D=json.loads((E/'Local/routes-art-r5.json').read_text())
for name,box in [('nature',(200,2200,1100,3200)),('pass',(1800,900,2400,1700))]:
 size=1500;im=Image.new('RGB',(size,size),'#17251c');d=ImageDraw.Draw(im)
 def xy(p):return ((p[0]-box[0])/(box[2]-box[0])*size,(p[1]-box[1])/(box[3]-box[1])*size)
 colors=['#e57373','#90caf9','#fff176','#ce93d8','#80cbc4','#ffb74d','#dce775','#f48fb1'];rows=[]
 for i,r in enumerate(D['routes']):
  hit=False
  for s in r['segments']:
   p=np.array(s['points_m']);mask=(p[:,0]>box[0])&(p[:,0]<box[2])&(p[:,1]>box[1])&(p[:,1]<box[3])
   if mask.any():
    d.line([xy(q) for q in p],fill=colors[i%8],width=3);ix=np.flatnonzero(mask);v=p[ix[len(ix)//2]];d.text(xy(v),str(i),fill='white');hit=True
  if hit:rows.append([i,r['a'],r['b']])
 for a in D['anchors']:
  p=xy(a['xy_m'])
  if 0<p[0]<size and 0<p[1]<size:d.ellipse((p[0]-5,p[1]-5,p[0]+5,p[1]+5),fill='white');d.text(p,a['id'],fill='white')
 im.save(E/f'Local/{name}-r5-diagnostic.png');print(name,rows)
