from pathlib import Path
import json, os
roots=[
 Path(r'D:\Unreal Projects\AoEAssetRenderLab\Content\VaultCache'),
 Path(r'C:\ProgramData\Epic\EpicGamesLauncher\VaultCache'),
]
for root in roots:
    print("\nROOT",root)
    if not root.exists(): continue
    for p in sorted([x for x in root.iterdir() if x.is_dir()], key=lambda x:x.stat().st_mtime, reverse=True)[:80]:
        data=p/'data'/'Content'
        content_roots=[]
        if data.exists():
            try: content_roots=sorted([x.name for x in data.iterdir() if x.is_dir()])[:12]
            except: pass
        maps=[]
        if data.exists():
            try: maps=[str(x.relative_to(data)).replace('\\','/') for x in data.rglob('*.umap')][:12]
            except: pass
        print(json.dumps({
          "dir":p.name,
          "mtime":p.stat().st_mtime,
          "roots":content_roots,
          "maps":maps[:8],
        }, ensure_ascii=False))
