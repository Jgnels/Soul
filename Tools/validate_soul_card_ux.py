import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ui = json.loads((ROOT / 'Data' / 'soul_card_ui_schema.json').read_text(encoding='utf-8'))
render = json.loads((ROOT / 'Data' / 'soul_card_render_manifest.json').read_text(encoding='utf-8'))
errors = []
expected_factions = {'human','dwarf','viking','orc','dark'}
expected_ranks = ['recruit','seasoned','veteran','elite','legendary']
jobs = render.get('unit_jobs', [])
if len(jobs) != 35: errors.append(f'expected 35 jobs, found {len(jobs)}')
counts = {}
ids = set()
for job in jobs:
    uid = job.get('unit_id'); faction = job.get('faction')
    if not uid or uid in ids: errors.append(f'duplicate/missing unit id: {uid!r}')
    ids.add(uid); counts[faction] = counts.get(faction, 0) + 1
    if job.get('status') != 'BLOCKED_ROSTER_CAST': errors.append(f'{uid}: invalid pre-cast status')
    if job.get('source_asset_id') is not None or job.get('source_asset_path') is not None: errors.append(f'{uid}: invented source binding')
if set(counts) != expected_factions: errors.append(f'faction set mismatch: {sorted(counts)}')
for faction in sorted(expected_factions):
    if counts.get(faction) != 7: errors.append(f'{faction}: expected 7 jobs, found {counts.get(faction,0)}')
rank_ids = [x.get('rank') for x in render.get('rank_frames', [])]
if rank_ids != expected_ranks: errors.append(f'render rank order mismatch: {rank_ids}')
ui_rank_ids = [x.get('id') for x in ui.get('ranks', [])]
if ui_rank_ids != expected_ranks: errors.append(f'UI rank order mismatch: {ui_rank_ids}')
small = {x['id']: x for x in ui.get('battlefield_small_fields', [])}
for field in ('portrait','count','hp_state','initiative','veterancy_rank','role_range','retaliation','morale','luck','statuses'):
    if field not in small: errors.append(f'small card missing {field}')
if render.get('global_capture',{}).get('no_render_in_this_lane') is not True: errors.append('non-UE lane render guard missing')
if render.get('hero_jobs',{}).get('status') != 'BLOCKED_HERO_POOL_AUTHORITY': errors.append('hero pool must remain authority-blocked')
if errors:
    print('SOUL_CARD_UX_VALIDATION: FAIL')
    [print('- '+e) for e in errors]
    raise SystemExit(1)
print('SOUL_CARD_UX_VALIDATION: PASS')
print(f'- unit jobs: {len(jobs)}')
print(f'- faction distribution: {counts}')
print(f'- rank frames: {rank_ids}')
print('- hero job template present and authority-blocked')
print('- no UE/render work performed by this lane')
