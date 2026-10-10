"""Human New/Continue front door; delegates verification and the thermal guard."""
from pathlib import Path
import argparse, datetime, json, subprocess, sys, uuid
import play_candidate

R = Path(__file__).resolve().parents[2]
E = R / 'Evidence/FourFactionAlpha-20261009'
SESSIONS = R / 'Saved/CompositionPlaytest/FourFactionAlpha/HumanSessions'
CURRENT = SESSIONS / 'current-session.json'
CONFIG = Path(__file__).with_name('four_faction_playtest.json')
SLOT = Path('Saved/RBSave/Domains/Soul.Composition3500.FourFactionAlpha.domain.rbsave')
HEARTLAND = False

def current_session():
    if not CURRENT.is_file(): return None
    data = json.loads(CURRENT.read_text())
    name = data['session']
    if not name or any(c not in 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_' for c in name):
        raise ValueError('Invalid Human session pointer; no save was changed.')
    user = (SESSIONS / name).resolve()
    if user.parent != SESSIONS.resolve() or not user.is_dir():
        raise ValueError('Human session is missing or outside its safe directory.')
    return user

def choose_session(new):
    if new:
        stamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ')
        return SESSIONS / ('Jeff-' + stamp + '-' + uuid.uuid4().hex[:8])
    user = current_session()
    if not user or not (user / SLOT).is_file():
        raise ValueError('No F5 save in the current Human session. Choose NEW; earlier sessions are preserved.')
    return user

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--new', action='store_true')
    mode.add_argument('--continue', dest='resume', action='store_true')
    parser.add_argument('--playtest-fps', type=int, choices=(30,40))
    parser.add_argument('--minutes', type=int, default=120)
    parser.add_argument('--dry-run', action='store_true')
    args = parser.parse_args()
    config = json.loads(CONFIG.read_text())
    cap = args.playtest_fps or config['default_fps']
    if cap not in (30,40): raise ValueError('Playtest cap must be 30 or 40 FPS.')
    if not args.new and not args.resume:
        print('\nSOUL - HUMAN HEARTLAND' if HEARTLAND else '\nSOUL - FOUR-FACTION HUMAN PLAYTEST')
        print('N: NEW campaign (keeps every older session)')
        print('C: CONTINUE current session from its last F5 save')
        print(f'FPS: {cap}. Target option: --playtest-fps 40; safe option: --playtest-fps 30.')
        print('85 C thermal guard. Save with F5; exit with Alt+F4. No automatic gameplay inputs.')
        answer = input('[N] New / [C] Continue / [Q] Quit: ').strip().lower()
        if answer == 'q': return 0
        if answer not in ('n','c'): raise ValueError('Choose N or C.')
        args.new = answer == 'n'; args.resume = answer == 'c'
    if config.get('note'): print(config['note'])
    user = choose_session(args.new)
    print('Verifying the current cooked build and campaign assets...', flush=True)
    plan = play_candidate.prepare(R/config['stage_receipt'],False,args.minutes,
        alpha=True,heartland=HEARTLAND,evidence_root=E,playtest_fps=cap,user_directory=user,continue_campaign=args.resume)
    if args.dry_run:
        print(json.dumps(plan,indent=2)); return 0
    sys.path.insert(0,str(R/'Tools'))
    from qualify_soul_vertical import conflicting_processes
    if conflicting_processes():raise ValueError('Close the existing Soul/Unreal session before starting another. Current saves are unchanged.')
    if args.new:
        user.mkdir(parents=True,exist_ok=False)
        pointer = CURRENT.with_suffix('.pending')
        pointer.write_text(json.dumps({'session':user.name,'created_utc':datetime.datetime.now(datetime.timezone.utc).isoformat()},indent=2)+'\n')
        pointer.replace(CURRENT)  # Only the session selector changes; no save is removed/replaced.
    print(f"{'NEW' if args.new else 'CONTINUE'} | {cap} FPS | Save folder: {user}",flush=True)
    print(f'Guarded session limit: {args.minutes} minutes. Keep this console open.',flush=True)
    output=Path(plan['output'])
    receipt=E/'Local'/('human-launch-'+output.name+'.json')
    receipt.write_text(json.dumps(plan,indent=2)+'\n')
    result=subprocess.run(plan['command'],cwd=R).returncode
    summary=output/'summary.json'
    if summary.is_file():
        report=json.loads(summary.read_text())
        if report.get('stop_reason')=='thermal_cutoff_85c':
            print('Stopped at the 85 C safety limit. Let the GPU cool; use the 30 FPS option.')
    if (user / SLOT).is_file():
        print('Session ended. Your last F5 save is preserved. Use CONTINUE next time.')
    else:
        print('Session ended without an F5 save. Choose NEW next time; press F5 during play to enable CONTINUE.')
    return result

if __name__=='__main__':
    try: raise SystemExit(main())
    except (OSError,ValueError,KeyError) as error:
        print('SOUL launcher:',error,file=sys.stderr);raise SystemExit(1)
