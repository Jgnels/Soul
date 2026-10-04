import unreal
from pathlib import Path
out=Path(r"D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929/Evidence/Spellbook-20261004/icons")
out.mkdir(parents=True,exist_ok=True)
for i in range(1,41):
    name=f"T_spells_mix_frame_{i:02d}"
    asset=unreal.load_asset('/Game/Spell_Mix/frame/Textures/'+name)
    if not asset:
        unreal.log_error('Missing '+name)
        continue
    task=unreal.AssetExportTask()
    task.object=asset
    task.filename=str(out/(name+'.png'))
    task.automated=True
    task.prompt=False
    task.replace_identical=True
    task.exporter=unreal.TextureExporterPNG()
    unreal.Exporter.run_asset_export_task(task)
unreal.log('SOUL_ICON_EXPORT_DONE')
