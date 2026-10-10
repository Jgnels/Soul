"""Separate Heartland playtest front door; existing four-faction saves stay untouched."""
from pathlib import Path
import play_four_faction_alpha as front
front.HEARTLAND=True
front.E=front.R/'Evidence/HumanHeartlandDepth-20261010'
front.SESSIONS=front.R/'Saved/CompositionPlaytest/HeartlandAlpha/HumanSessions'
front.CURRENT=front.SESSIONS/'current-session.json'
front.CONFIG=Path(__file__).with_name('heartland_playtest.json')
front.SLOT=Path('Saved/RBSave/Domains/Soul.Composition3500.HeartlandAlpha.domain.rbsave')
if __name__=='__main__':
 try:raise SystemExit(front.main())
 except (OSError,ValueError,KeyError) as error:print('SOUL Heartland:',error);raise SystemExit(1)
