"""Manual Siege V0 start fixture. No gameplay automation; prior Heartland saves stay isolated."""
from pathlib import Path
import sys
R=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(R/'Tools/ProductionContinuation'))
import play_four_faction_alpha as front
front.HEARTLAND=True
front.E=R/'Evidence/HumanCapitalSiege-20261010'
front.SESSIONS=R/'Saved/CompositionPlaytest/HeartlandAlpha/HumanSessions/SiegeV0'
front.CURRENT=front.SESSIONS/'current-session.json'
front.CONFIG=Path(__file__).with_name('playtest.json')
front.SLOT=Path('Saved/RBSave/Domains/Soul.Composition3500.HeartlandAlpha.SiegeV0Playtest.domain.rbsave')
prepare_base=front.play_candidate.prepare
def prepare(*args,**kwargs):
    plan=prepare_base(*args,**kwargs)
    plan['command']+=['--ue-arg=-SoulSiegeV0','--ue-arg=-SoulSiegeFixture']
    plan['command'][plan['command'].index('--resolution')+1]='1280x800'
    assert not any('Qualification' in arg or 'SoulAutobattle' in arg for arg in plan['command'])
    plan['save_slot']='Soul.Composition3500.HeartlandAlpha.SiegeV0Playtest'
    plan['start_fixture']='Human army at Crossroads; Dwarf occupation of fortified Human Capital; starting Human total preserved.'
    return plan
front.play_candidate.prepare=prepare
if __name__=='__main__':
    print('SOUL — HUMAN CAPITAL SIEGE V0 | isolated start fixture | no automated gameplay')
    try:raise SystemExit(front.main())
    except (OSError,ValueError,KeyError) as error:print('SOUL Siege:',error);raise SystemExit(1)
