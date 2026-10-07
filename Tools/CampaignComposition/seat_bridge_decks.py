"""Seat candidate bridge endpoints on measured banks; no Landscape changes."""
import unreal as u,json,math
from pathlib import Path
E=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929/Evidence/ProductionWorldComposition-20261007')
actors={a.get_actor_label():a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()};sites={c['gate_id']:c for c in json.loads((E/'selected-crossing-sites-r3.json').read_text())['crossings']};data=json.loads((E/'bridge-placement-r1.json').read_text())
for r in data['bridges']:
 c=sites[r['id']];za,zb=r['bank_z_m'];deck=(za+zb)/2+.01;pitch=math.degrees(math.atan2(zb-za,r['span_m']));rot=u.Rotator(pitch=pitch,yaw=r['yaw'],roll=0);sx,sy,sz=r['scale'];center=u.Vector(c['center_xy_m'][0]*100-175000,c['center_xy_m'][1]*100-175000,deck*100)
 actor=actors['Composition_Bridge_'+r['id']];loc=center+rot.get_forward_vector()*(990*sx)+rot.get_right_vector()*(293.9635*sy);actor.set_actor_location_and_rotation(loc,rot,False,False);r.update(deck_z_m=deck,pitch_deg=pitch,deck_gradient=(zb-za)/r['span_m'],status='endpoints seated 1 cm above measured banks; native bridge deck shape still visual-only')
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(E/'bridge-placement-r2.json').write_text(json.dumps(data,indent=2));print('DECKS_SEATED',len(data['bridges']))
