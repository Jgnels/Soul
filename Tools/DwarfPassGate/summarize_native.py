from pathlib import Path
import sys,runpy
R=Path.cwd();sys.path.insert(0,str(R/'Tools/MapPolish'))
import route_surface
route_surface.OUT=R/'Evidence/DwarfPassGate-20261007'
runpy.run_path(str(R/'Tools/MapPolish/summarize_native.py'),run_name='__main__')
