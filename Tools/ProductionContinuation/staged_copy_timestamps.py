"""Keep new temporary stage copies fresh without touching linked cooked payloads."""
import os
from pathlib import Path
import time

def refresh_stage_copies(stage: Path):
    stage=stage.resolve();now=time.time();copies=0;linked=0
    for path in stage.rglob('*'):
        if not path.is_file():continue
        if not path.resolve().is_relative_to(stage):
            raise ValueError('Stage resource resolves outside the owned package root')
        if path.stat().st_nlink>1:
            linked+=1
            continue
        os.utime(path,(now,now));copies+=1
    return {'copied_files_refreshed':copies,'linked_files_skipped':linked,
            'file_bytes_changed':False,'reason':'A new temp stage must not inherit aged source timestamps; linked cooked payload metadata is preserved.'}
