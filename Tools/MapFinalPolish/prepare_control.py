"""Reuse established road-only rasterizer with this run's isolated output paths."""
import sys,runpy
from pathlib import Path
R=Path(__file__).resolve().parents[2];sys.path.insert(0,str(R/'Tools/MapPolish'));import route_surface
route_surface.OUT=R/'Evidence/MapFinalPolish-20261007'
sys.argv=['prepare_road_control.py','--input','routes-art-r2.json','--revision','final1']
runpy.run_path(str(R/'Tools/MapPolish/prepare_road_control.py'),run_name='__main__')
