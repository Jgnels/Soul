import sys,runpy
from pathlib import Path
R=Path.cwd();sys.path.insert(0,str(R/'Tools/MapPolish'));import route_surface
route_surface.OUT=R/'Evidence/MapFinalPolish-20261007'
sys.argv=['prepare_road_control.py','--input','routes-art-r6.json','--revision','final5'];runpy.run_path(str(R/'Tools/MapPolish/prepare_road_control.py'),run_name='__main__')
sys.argv=['prepare_native_routes.py','--input','routes-presentation-final5.json'];runpy.run_path(str(R/'Tools/MapPolish/prepare_native_routes.py'),run_name='__main__')
