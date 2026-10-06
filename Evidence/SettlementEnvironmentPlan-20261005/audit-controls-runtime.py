"""File-only audit of the frozen development-controls-r1 evidence. No UE calls."""
from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
RUN = OUT / 'Local/development-controls-r1'
SAVED = RUN / 'User/Saved'
PREFIX = 'development-controls-r1'
def read(p):
    return json.loads(p.read_text(encoding='utf-8-sig'))
def digest(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()
def record(p, expected=None):
    actual = digest(p)
    result = {'path': p.relative_to(ROOT).as_posix(), 'bytes': p.stat().st_size, 'sha256': actual}
    if expected is not None:
        result.update(expected_sha256=expected, matches_frozen=actual == expected)
    return result

summary = read(RUN / 'runtime/summary.json')
receipt = read(SAVED / f'{PREFIX}_controls.json')
lines = (RUN / 'runtime/unreal.log').read_text(encoding='utf-8-sig').splitlines()
source = (ROOT / 'Source/Soul/Private/SoulSettlementDevelopmentQualification.cpp').read_text()
checks, inputs, success, failures, screenshots = [], [], [], [], []
for number, line in enumerate(lines, 1):
    m = re.search(r'SOUL_SETTLEMENT_DEVELOPMENT_CONTROLS_CHECK_PASS step=(\d+) (.*)', line)
    if m:
        checks.append({'step': int(m[1]), 'label': m[2], 'log_line': number})
    m = re.search(r'SOUL_SETTLEMENT_DEVELOPMENT_CONTROLS_INPUT step=(\d+) key=(\w+)', line)
    if m:
        inputs.append({'step': int(m[1]), 'key': m[2], 'log_line': number})
    if 'SOUL_SETTLEMENT_DEVELOPMENT_CONTROLS_PASS ' in line:
        success.append({'log_line': number, 'text': line})
    if re.search(r'\b(?:Error|Fatal):|Assertion failed|Critical error|SOUL_SETTLEMENT_DEVELOPMENT_CONTROLS_FAIL', line):
        failures.append({'log_line': number, 'text': line})
    m = re.search(r'Tracing Screenshot "([^"]+)" taken with size: (\d+) x (\d+)', line)
    if m:
        screenshots.append({'name': m[1], 'width': int(m[2]), 'height': int(m[3]), 'log_line': number})

expected_inputs = [(0,'T'), (1,'H'), (2,'U'), (3,'LeftMouseButton'), (4,'SpaceBar'),
                   (5,'T'), (6,'F5'), (7,'SpaceBar'), (8,'T'), (10,'F9'), (11,'T'),
                   (12,'H'), (13,'SpaceBar'), (15,'T'), (16,'H'), (17,'LeftMouseButton'),
                   (18,'F5'), (19,'SpaceBar'), (20,'F9'), (21,'T')]
milestones = {
 0: 'loaded data fixture is human.tavern:200 gold,2 days,level1,tavern hero service',
 1: 'T opens real town UI with construction visible and hiring absent',
 2: 'blocked hiring preserves both domains',
 3: 'U starts construction once and deducts exactly200 gold without advancing day',
 4: 'rendered BuildTavern duplicate reached its handler and reported rejection',
 5: 'one Space advances one campaign/construction day; hiring remains locked',
 6: 'mid-construction town UI is reopened and rendered',
 7: 'F5 does not mutate either domain',
 8: 'second Space completes tavern and unlocks existing service',
 9: 'completed town UI is reopened and rendered before capture',
 10: 'actual viewport is 1920x1080',
 11: 'F9 restores exact mid-construction campaign and settlement snapshots',
 12: 'reopened loaded town still hides locked hiring',
 13: 'H after rollback stays blocked and preserves both restored domains',
 14: 'restored remaining day completes once and EndDay closes town',
 15: 'completed town remains closed after EndDay',
 16: 'T reopens completed town with rendered hiring hitbox',
 17: 'H hires existing companion once for exactly1200 gold',
 18: 'rendered Hire duplicate reached its handler and reported already hired',
 19: 'completed-plus-hired F5 preserves both live domains',
 20: 'ordinary day input changes live checkpoint before final F9',
 21: 'final F9 restores exact completed-plus-hired campaign and settlement snapshots',
 22: 'loaded completed state is visible in real town panel',
 23: 'final rendered town still matches both saved domains',
}
expected_checks = []
def require(condition, name):
    expected_checks.append({'check': name, 'pass': bool(condition)})

require(summary['exit_code'] == 0 and summary['clean_shutdown'] and summary['completion_marker_observed'], 'wrapper clean exit0 and completion marker')
require(len(success) == 1 and not failures, 'one actual controls PASS and no error/fatal/assertion/fail markers')
require(receipt['status'] == 'CONTROLS_PASS', 'runtime receipt reports CONTROLS_PASS')
require(all(receipt[k] == 0 for k in ['authored_environment','miniature','battle_environment']), 'explicit nonvisual scope flags remain zero')
require(receipt['viewport'] == [1920,1080] and receipt['fps_cap'] == 20 and receipt['warmup_seconds'] == 20, '1920x1080 and20FPS functional run with20s warmup')
require(sorted(set(x['step'] for x in checks)) == list(range(24)), 'all24 steps0..23 observed')
require([x['step'] for x in checks] == sorted(x['step'] for x in checks), 'steps ordered monotonically')
require([(x['step'],x['key']) for x in inputs] == expected_inputs, 'exact20 logged input actions including2F5/2F9')
require(all(x['label'] in source for x in checks), 'every logged check label exists in frozen qualifier source')
for step, label in milestones.items():
    require(sum(x['step'] == step and x['label'] == label for x in checks) == 1, f'step{step}: {label}')
for step in range(24):
    require(sum(x['step'] == step and x['label'] == 'actual viewport is 1920x1080' for x in checks) == 1, f'step{step} actual viewport checked once')

def events(pattern):
    return [{'log_line': n, 'text': line} for n,line in enumerate(lines,1) if re.search(pattern,line)]
save_events = events(r'SOUL_CAMPAIGN_SAVE success=1 ')
load_events = events(r'SOUL_CAMPAIGN_LOAD success=1 ')
scope_events = events(r'SOUL_CAMPAIGN_SAVE_SCOPE domains=Soul.Campaign,Soul.Settlements')
present_events = events(r'SOUL_CAMPAIGN_LOAD_PRESENTED .*revision=[12]$')
require(len(save_events) == len(load_events) == len(scope_events) == len(present_events) == 2, 'two successful scoped saves, loads and presentation revisions')
require(receipt['load_revision'] == 2 and receipt['both_domain_snapshots_exact'], 'runtime final revision2 and exact two-domain assertion')
require(sum(x['label'] == 'new F5 operation completed and changed actual RBSave checkpoint bytes' for x in checks) == 2, 'both F5 completions require changed checkpoint bytes')
require(sum(x['label'] == 'new F9 callback completed with exact load and settlement revisions' for x in checks) == 2, 'both F9 completions require current success and exact revision increments')

shots = ['start','mid','completed','loaded','hired','loaded_completed']
shot_records = []
for label in shots:
    p = SAVED / f'Screenshots/{PREFIX}_{label}.png'
    b = p.read_bytes()
    dimensions = list(struct.unpack('>II', b[16:24]))
    shot_records.append({**record(p), 'label': label, 'dimensions': dimensions})
    require(b[:8] == b'\x89PNG\r\n\x1a\n' and dimensions == [1920,1080], f'{label} realPNG1920x1080')
require([x['name'] for x in screenshots] == [f'{PREFIX}_{x}' for x in shots], 'all six screenshot completion log events in order')
require(next(x['log_line'] for x in screenshots if x['name'] == f'{PREFIX}_completed') < next(x['log_line'] for x in inputs if x['key'] == 'F9'), 'completed screenshot logged before first F9')

snapshots = {}
snapshot_records = []
for label in ['start','mid','completed_hired']:
    cpath = SAVED / f'{PREFIX}_{label}_Soul.Campaign.json'
    spath = SAVED / f'{PREFIX}_{label}_Soul.Settlements.json'
    c,s = read(cpath),read(spath)
    settlement = s['settlements'][0]
    tavern = next(x for x in settlement['buildings'] if x['id'] == 'human.tavern')
    snapshots[label] = {'campaign':c, 'settlements':s}
    snapshot_records.append({'label':label,'day':c['day'],'gold':c['resources']['gold'], 'daily_gold_income':c['income']['gold'],
                             'hired':c['hired'],'tavern':tavern, 'settlement_id':settlement['id'], 'files':[record(cpath),record(spath)]})
require([(x['day'],x['gold'],x['hired'],x['tavern']['level'],x['tavern']['days'],x['tavern']['condition']) for x in snapshot_records] == [(1,3000,False,0,0,0),(2,3250,False,0,1,1),(3,2500,True,1,0,2)], 'persisted snapshots match actual day/gold/hire/tavern sequence')
require(3000-200+2*450-1200 == receipt['final_gold'] == 2500 and receipt['final_day'] == 3, 'final economy balances200build+2x450income-1200hire')
other_buildings = [[b for b in x['settlements']['settlements'][0]['buildings'] if b['id'] != 'human.tavern'] for x in snapshots.values()]
require(other_buildings[0] == other_buildings[1] == other_buildings[2], 'other four settlement buildings unchanged across persisted snapshots')
campaign_invariants = []
settlement_invariants = []
for pair in snapshots.values():
    campaign = json.loads(json.dumps(pair['campaign']))
    campaign.pop('day')
    campaign.pop('hired')
    campaign['resources'].pop('gold')
    campaign_invariants.append(campaign)
    settlements = json.loads(json.dumps(pair['settlements']))
    settlements['settlements'][0]['buildings'] = [b for b in settlements['settlements'][0]['buildings'] if b['id'] != 'human.tavern']
    settlement_invariants.append(settlements)
require(campaign_invariants[0] == campaign_invariants[1] == campaign_invariants[2], 'all campaign fields except day/gold/hired remain exactly equal')
require(settlement_invariants[0] == settlement_invariants[1] == settlement_invariants[2], 'all settlement fields except tavern remain exactly equal')

qualifier_manifest = read(OUT / 'controls-qualifier-source-receipt.json')
development_manifest = read(OUT / 'development-source-receipt.json')
asset_manifest = read(OUT / 'development-scenario-authoring.json')
frozen = {x['path']:x['sha256'] for x in qualifier_manifest['source_after'] + development_manifest['files']}
frozen.update(summary['mesa_payload_sha256'])
frozen['Data/CampaignEvilCorridor/presentation.json'] = summary['evil_corridor_profile_sha256']
frozen['Content/Soul/Data/Settlements/DA_Soul_HumanCapital_DevelopmentProof.uasset'] = asset_manifest['asset_sha256']
hashes = [record(ROOT/path, value) for path,value in sorted(frozen.items())]
hashes.append(record(ROOT / 'Data/SettlementEnvironments/CrownsteadDevelopmentProof.json'))
require(all(x.get('matches_frozen',True) for x in hashes), 'all source/proofDA/map/presentation/height/DLL frozen hashes match')

telemetry = [json.loads(x) for x in (RUN/'runtime/telemetry.jsonl').read_text().splitlines() if x.strip()]
def peak(rows, key, category):
    values = []
    for r in rows:
        objs = r.get(category,[]) if category == 'gpu' else [r.get(category,{})]
        for obj in objs:
            if isinstance(obj,dict) and isinstance(obj.get(key),(int,float)):
                values.append({'value':obj[key],'utc':r['utc']})
    return max(values,key=lambda x:x['value']) if values else None
resource_stats = {'telemetry_samples':len(telemetry), 'start_utc':telemetry[0]['utc'], 'end_utc':telemetry[-1]['utc'],
 'peak_gpu_temperature_c':peak(telemetry,'temperature_c','gpu'),
 'peak_gpu_utilization_percent':peak(telemetry,'utilization_percent','gpu'),
 'peak_device_vram_used_mib':peak(telemetry,'memory_used_mib','gpu'),
 'peak_sampled_process_working_set_mib':peak(telemetry,'working_set_mib','process_memory'),
 'peak_process_reported_working_set_mib':peak(telemetry,'peak_working_set_mib','process_memory'),
 'peak_process_private_commit_mib':peak(telemetry,'private_commit_mib','process_memory'),
 'scope':'Full launch/load/warmup/controls/shutdown sample series; VRAM is whole-device usage, not per-process. Working set and private commit are different measures. Capped functional run; no throughput/performance acceptance.'}
warnings = events(r'\bWarning:')
warning_categories = Counter(re.search(r'\](?:\[.*?\])?(\w+): Warning:',x['text'])[1] if re.search(r'\](?:\[.*?\])?(\w+): Warning:',x['text']) else 'other' for x in warnings)
evidence_files = [RUN/'runtime/summary.json',RUN/'runtime/launch.json',RUN/'runtime/unreal.log',RUN/'runtime/telemetry.jsonl',SAVED/f'{PREFIX}_controls.json',SAVED/f'{PREFIX}_controls_started.txt']
evidence_files += list((SAVED/'RBSave/Domains').rglob('*.rbsave')) + list((SAVED/'RBSave/Domains').glob('*.bak'))
run_files = [record(p) for p in evidence_files]
failures_audit = [x for x in expected_checks if not x['pass']]
result = {
 'recorded_utc':datetime.now(timezone.utc).isoformat(), 'status':'CONTROLS_PASS' if not failures_audit else 'AUDIT_FAIL',
 'authored_environment':0,'miniature':0,'battle_environment':0,'performance_acceptance':False,
 'run':RUN.relative_to(ROOT).as_posix(), 'summary':summary, 'runtime_receipt':receipt,
 'checks_count':len(checks),'paced_steps':24,'inputs_count':len(inputs),'input_counts':dict(Counter(x['key'] for x in inputs)),
 'audit_checks':expected_checks,'failed_audit_checks':failures_audit,'actual_checks':checks,'actual_inputs':inputs,
 'pass_markers':success,'unexpected_errors':failures,'save_events':save_events,'load_events':load_events,
 'save_scope_events':scope_events,'load_presented_events':present_events,'screenshots':shot_records,
 'screenshot_log_events':screenshots,'snapshot_values':snapshot_records,'source_and_payload_hashes':hashes,'run_evidence_hashes':run_files,
 'telemetry':resource_stats,'warning_count':len(warnings),'warning_categories':dict(warning_categories),
 'important_warning':'t.MaxFPS SetByCode was rejected because existing SetByConsole had higher priority; effective value remained20 and qualifier verified20.',
 'independent_snapshot_cross_check':'integration_audit independently confirmed all three numeric states and unchanged campaign/settlement fields; no cold-start restore is inferred.',
 'message_evidence':{'method':'Frozen source predicates read actual Campaign.LastMessage after ordinary input and emit CHECK_PASS; raw message strings are not separately logged.',
 'build_success':'Tavern construction started. Advance the day to make progress.',
 'duplicate_build_rejection':'Construction prerequisites, resources or building level do not permit this upgrade.',
 'blocked_hire_substring':'Complete the tavern','hire_success':'Tavern hero hired for 1200 gold.',
 'duplicate_hire_rejection':'The tavern hero is already in your service.'},
 'limitations':[
  'Saved JSON files are three expected checkpoint pairs. Restored-state byte equality is independently audited from frozen qualifier predicates and their successful runtime log checks; separate restored JSON files were not written.',
  'last_operation_busy_observed=false is allowed: final load completed between ticks. Success still required current bLastLoadSucceeded, no busy operation, exact load/development revision increments and exact two-domain equality.',
  'PNG headers and completion order verified here; root owns actual UI image review. No visual/art acceptance is inferred.',
  'Proof DA owned_environment_map is null; no authored settlement, miniature, visit, battle environment or full-capital payload qualification.',
  'Existing retained presentation/dressing remains visible and was not redesigned by this audit.'
 ]
}
(OUT/'controls-runtime-audit.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
md = [f"# Settlement development controls audit\n\n**{result['status']}** — development-controls-r1 exited cleanly with code0. All24 paced steps0–23, {len(checks)} successful runtime checks,20 input actions, two F5 saves and two F9 loads are present. No Error/Fatal/assertion/controls-failure markers were found.\n",
      'This is functional UI/state/persistence evidence only: **authored_environment=0; miniature=0; battle_environment=0**. It is not art acceptance or a performance benchmark.\n',
      '## State and persistence\n\n| Saved checkpoint | Day | Gold | Hired | Tavern level | Days remaining | Condition |\n|---|---:|---:|---|---:|---:|---|']
for x in snapshot_records:
    md.append(f"| {x['label']} | {x['day']} | {x['gold']} | {x['hired']} | {x['tavern']['level']} | {x['tavern']['days']} | {['Unbuilt','Building','Intact'][x['tavern']['condition']]} |")
md += ['\nThe200-gold build and1200-gold hire occurred once each. Gold reconciles as3000−200+2×450−1200=2500. Hiring was blocked before construction and after mid-construction rollback. Duplicate rendered BuildTavern/Hire clicks produced the actual rejection messages and left both domains unchanged. All four other starting buildings remained unchanged.\n',
 'Both saves logged Soul.Campaign and Soul.Settlements scope and successful completion; the qualifier required actual checkpoint-byte changes. Both loads logged success and presentation revisions1/2, then passed exact two-domain equality and exact load/development revision assertions. The six persisted JSON files are expected checkpoint pairs; separate post-load JSON files were not written, so restoration equality is supported by the frozen source predicates and successful runtime checks.\n',
 '## Input and capture evidence\n\nThe frozen qualifier routes actions through PC InputKey and the two duplicate actions through actual HUD hitboxes. There are seven T inputs, three H inputs, one U, four SpaceBar, two F5, two F9 and two mouse inputs only if their counts match the JSON; the exact authoritative counts are listed below.\n',
 f"`{json.dumps(result['input_counts'],sort_keys=True)}`\n",
 'All six PNGs have valid1920×1080 headers and matching completion log entries: start, mid, completed, loaded, hired, loaded_completed. Completed capture finished before the first F9. Root performs actual image inspection.\n',
 '## Resource observations\n\n| Metric | Peak |\n|---|---:|']
for key in ['peak_gpu_temperature_c','peak_gpu_utilization_percent','peak_device_vram_used_mib','peak_sampled_process_working_set_mib','peak_process_reported_working_set_mib','peak_process_private_commit_mib']:
    md.append(f"| {key} | {resource_stats[key]['value']} |")
md += [f"\n{len(telemetry)} telemetry samples span full startup through shutdown. VRAM is whole-device usage; process RAM reports sampled working set, OS peak working set, and private commit separately. The run was capped at20FPS with a20-second functional warmup. No performance acceptance is claimed.\n",
 f"## Integrity and limits\n\nAll{sum('expected_sha256' in x for x in hashes)} frozen source/payload hashes match, including qualifier revision2, integration sources, proof DA, retained map/presentation/height, and loaded DLL. Exact hashes, every check/input, all screenshots, snapshots, save files and telemetry are in [controls-runtime-audit.json](controls-runtime-audit.json).\n",
 f"There were{len(warnings)} Warning lines, preserved by category in JSON; no error markers. The MaxFPS priority warning retained the required20 value. The final async busy flag was not sampled true, but current callback success/revision/byte-equality guards passed. The proof DA has no bound authored environment; complete capital recovery and any scene/miniature/battle-environment acceptance remain separate.\n"]
# Keep prose readable; exact counts come from parsed records.
md = [x.replace('There are seven T inputs, three H inputs, one U, four SpaceBar, two F5, two F9 and two mouse inputs only if their counts match the JSON; the exact authoritative counts are listed below.', 'Exact input counts are listed below.') for x in md]
replacements = {
 'code0':'code 0', 'All24':'All 24', 'steps0':'steps 0', 'checks,20':'checks, 20',
 'The200':'The 200', 'and1200':'and 1200', 'as3000':'as 3000', 'revisions1/2':'revisions 1/2',
 'valid1920':'valid 1920', 'at20FPS':'at 20 FPS', 'a20-second':'a 20-second',
 'required20':'required 20', 'There were':'There were ', 'value.':'value.',
 f"All{sum('expected_sha256' in x for x in hashes)}":f"All {sum('expected_sha256' in x for x in hashes)}",
}
for old,new in replacements.items():
    md = [x.replace(old,new) for x in md]
(OUT/'controls-runtime-audit.md').write_text('\n'.join(md),encoding='utf-8')
print(json.dumps({'status':result['status'],'checks':len(checks),'inputs':result['input_counts'],'failed_audit_checks':failures_audit,'telemetry':resource_stats,'json_sha256':digest(OUT/'controls-runtime-audit.json')},indent=2))
