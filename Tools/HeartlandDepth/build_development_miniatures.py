"""Use the existing native render-LOD compiler for matched Heartland state pieces."""
import unreal
from pathlib import Path
r=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
compiler=r/'Tools/SettlementEnvironments/build_renderlod_miniature.py'
for name in ['Base','ArcaneHall','Barracks','Market']:
 recipe=r/'Evidence/HumanHeartlandDepth-20261010/Local/Miniatures'/(name+'-recipe.json')
 exec(compile(compiler.read_text(),str(compiler),'exec'),{'RECIPE_PATH':str(recipe)})
print('SOUL_DEPTH_MINIATURES_COMPLETE')
