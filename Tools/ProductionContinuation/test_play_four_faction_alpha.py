"""New/Continue never repurposes proof saves or destroys an earlier Human session."""
import json, tempfile, unittest
from pathlib import Path
from unittest.mock import patch
import play_four_faction_alpha as front

class HumanSessionBoundary(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name);self.sessions=self.root/'HumanSessions';self.sessions.mkdir()
        self.pointer=self.sessions/'current-session.json'
        for name,value in [('SESSIONS',self.sessions),('CURRENT',self.pointer)]:
            p=patch.object(front,name,value);p.start();self.addCleanup(p.stop)
    def saved(self):
        user=self.sessions/'Jeff-existing';(user/front.SLOT).parent.mkdir(parents=True)
        (user/front.SLOT).write_bytes(b'existing RBSave bytes')
        self.pointer.write_text(json.dumps({'session':user.name}));return user
    def test_new_is_distinct_and_preserves_previous_save(self):
        old=self.saved();a=front.choose_session(True);b=front.choose_session(True)
        self.assertNotEqual(a,b);self.assertNotEqual(a,old);self.assertFalse(a.exists())
        self.assertEqual((old/front.SLOT).read_bytes(),b'existing RBSave bytes')
        self.assertEqual(front.current_session(),old)
    def test_continue_requires_actual_F5_and_reuses_exact_directory(self):
        with self.assertRaisesRegex(ValueError,'No F5'):front.choose_session(False)
        old=self.saved();self.assertEqual(front.choose_session(False),old)
    def test_pointer_cannot_escape_or_target_proof_directory(self):
        self.pointer.write_text(json.dumps({'session':'../../Evidence/qualification'}))
        with self.assertRaises(ValueError):front.current_session()
    def test_missing_session_is_not_recreated(self):
        self.pointer.write_text(json.dumps({'session':'Jeff-missing'}))
        with self.assertRaises(ValueError):front.choose_session(False)
        self.assertFalse((self.sessions/'Jeff-missing').exists())

if __name__=='__main__':unittest.main(verbosity=2)
