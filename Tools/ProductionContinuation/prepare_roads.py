from pathlib import Path
import sys,runpy
R=Path.cwd();sys.path.insert(0,str(R/'Tools/MapPolish'));import route_surface
route_surface.OUT=R/'Evidence/ProductionContinuation-20261008'
sys.argv=['prepare_road_control.py','--input','routes-final.json','--revision','continuation2'];runpy.run_path(str(R/'Tools/MapPolish/prepare_road_control.py'),run_name='__main__')
sys.argv=['prepare_native_routes.py','--input','routes-final.json'];runpy.run_path(str(R/'Tools/MapPolish/prepare_native_routes.py'),run_name='__main__')
sys.argv=['route_atlas.py','--input','routes-final.json'];runpy.run_path(str(R/'Tools/MapPolish/route_atlas.py'),run_name='__main__')
