from pathlib import Path
import sys,runpy,shutil
R=Path.cwd();E=R/'Evidence/DwarfPassGate-20261007'
sys.path.insert(0,str(R/'Tools/MapPolish'))
sys.path.insert(0,str(R/'Evidence/MapPolish-20261007/Local/python-libs'))
import route_surface
route_surface.OUT=E
shutil.copy2(R/'Evidence/MapFinalPolish-20261007/Local/routes-presentation-final5.json',E/'Local/routes-final.json')
for script in ['prepare_native_routes.py','route_atlas.py']:
 sys.argv=[script,'--input','routes-final.json']
 runpy.run_path(str(R/'Tools/MapPolish'/script),run_name='__main__')
