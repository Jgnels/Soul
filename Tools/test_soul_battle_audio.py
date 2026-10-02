"""Exercise actual PCM preparation without loading Unreal or licensed assets."""
import array
from pathlib import Path
import tempfile
import unittest
import wave
from prepare_soul_battle_audio import read_mono, prepare

class BattleAudioTests(unittest.TestCase):
    def write_clip(self, path, channels, samples, rate=44100, width=2):
        with wave.open(str(path), "wb") as wav:
            wav.setnchannels(channels); wav.setsampwidth(width); wav.setframerate(rate)
            wav.writeframes(array.array("h", samples).tobytes())

    def test_stereo_preserves_frames_and_caps_peak(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "source.wav"
            self.write_clip(path, 2, [32767,32767,1000,3000,-32767,-32767])
            original = path.read_bytes()
            pcm, rate, info = read_mono(path)
            samples = array.array("h"); samples.frombytes(pcm)
            self.assertEqual(len(samples),3)
            self.assertLessEqual(max(abs(x) for x in samples),29491)
            self.assertAlmostEqual(samples[1]/samples[0],2000/32767,places=4)
            self.assertEqual(path.read_bytes(),original)
            self.assertEqual(rate,44100)
            self.assertAlmostEqual(info["duration"],3/44100)

    def test_quiet_mono_not_amplified(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/"mono.wav"
            self.write_clip(path,1,[100,-200,300])
            pcm,_,info=read_mono(path)
            values=array.array("h");values.frombytes(pcm)
            self.assertEqual(list(values),[100,-200,300])
            self.assertEqual(info["gain"],1)

    def test_cancelled_or_silent_audio_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/"cancel.wav"
            self.write_clip(path,2,[1000,-1000,2000,-2000])
            with self.assertRaisesRegex(ValueError,"phase-cancelled"):read_mono(path)

    def test_missing_inputs_do_not_create_partial_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp)
            with self.assertRaises(FileNotFoundError):prepare(root/"missing",root/"output")
            self.assertFalse((root/"output").exists())

    def test_donor_tree_cannot_be_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp)
            for output in [root,root/"child",root.parent]:
                with self.subTest(output=output),self.assertRaisesRegex(ValueError,"separate"):prepare(root,output)
