import sys,runpy
from pathlib import Path
R=Path.cwd();E=R/'Evidence/MapFinalPolish-20261007';sys.path.insert(0,str(R/'Tools/MapPolish'));import route_surface
route_surface.OUT=E
sys.argv=['route_atlas.py','--input','routes-presentation-final5.json'];runpy.run_path(str(R/'Tools/MapPolish/route_atlas.py'),run_name='__main__')
p=R/'Tools/MapPolish/summarize_native.py';s=p.read_text().replace('native-route-hits-final.json','native-route-hits-final2.json');exec(compile(s,str(p),'exec'))
