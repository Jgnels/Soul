from PIL import Image,ImageDraw
from pathlib import Path
p=Path('Evidence/DwarfPassGate-20261007/Local/captures-final')
names=['whole_labels_minimized','ordinary_campaign','human_roads','dwarf_context','dwarf_close','dwarf_rejected_gate_approach','mountain_pass','orc_routes','nature_paths']
for page in range(3):
 canvas=Image.new('RGB',(1280,3*385),'#172127');draw=ImageDraw.Draw(canvas)
 for row,name in enumerate(names[page*3:page*3+3]):
  canvas.paste(Image.open(p/(name+'.png')).resize((640,360)),(0,row*385+25))
  draw.text((10,row*385+5),name,fill='white')
  old=Path('Evidence/MapFinalPolish-20261007/Local/captures-final6')/(name+'.png')
  if name=='dwarf_context':old=old.with_name('dwarf_approach.png')
  if old.exists():canvas.paste(Image.open(old).resize((640,360)),(640,row*385+25))
 canvas.save(p/('review-sheet-'+str(page)+'.jpg'))
