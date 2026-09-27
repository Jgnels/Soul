"""Source/geometry regression only. Does not run UE, render, or inject player input."""
import math
from pathlib import Path
import re
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[4]
REF = None
if len(sys.argv) == 3 and sys.argv[1] == "--source-ref":
    REF = sys.argv[2]
    sys.argv[1:] = []


def source(relative):
    if REF:
        return subprocess.check_output(
            ["git", "show", f"{REF}:{relative}"], cwd=ROOT, encoding="utf-8"
        )
    return (ROOT / relative).read_text(encoding="utf-8")


def vector(arguments):
    return tuple(float(n.strip().removesuffix("f")) for n in arguments.split(","))


def axes(rotation):
    # UE FRotator (pitch, yaw, roll), here with zero roll.
    pitch, yaw, roll = map(math.radians, rotation)
    assert roll == 0
    sp, cp, sy, cy = math.sin(pitch), math.cos(pitch), math.sin(yaw), math.cos(yaw)
    return (cp * cy, cp * sy, sp), (-sy, cy, 0), (-sp * cy, -sp * sy, cp)


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


class CampaignViewTests(unittest.TestCase):
    def setUp(self):
        mode = source("Source/Soul/Private/SoulFounderPlaytestGameMode.cpp")
        graph = source("Source/Soul/Private/SoulFounderPlaytestCampaignActor.cpp")
        self.labels = source("Source/Soul/Private/SoulPlaytestRegionActor.cpp")
        camera = re.search(r"SpawnActor<ACameraActor>\(ACameraActor::StaticClass\(\),FVector\(([^)]+)\),FRotator\(([^)]+)\)\)", mode)
        self.assertIsNotNone(camera, "read the actual campaign camera")
        self.origin = vector(camera[1])
        self.forward, self.right, self.up = axes(vector(camera[2]))
        self.width = float(re.search(r"SetOrthoWidth\(([\d.]+)\)", mode)[1])
        self.constrained = "SetConstraintAspectRatio(true)" in mode
        self.aspect = 16 / 9
        if self.constrained:
            aspect = re.search(r"SetAspectRatio\(([\d.]+)f / ([\d.]+)f\)", mode)
            self.assertIsNotNone(aspect)
            self.aspect = float(aspect[1]) / float(aspect[2])
        self.regions = [(name, vector(position)) for name, position in re.findall(
            r'\{TEXT\("([a-z_]+)"\), FVector\(([^)]+)\)\}', graph)]
        self.assertEqual(len(self.regions), 9)

    def test_all_region_markers_fit_including_capital_and_second_target(self):
        # Includes a 100 cm marker margin, larger than the selected sphere radius.
        for viewport in ((1280, 720), (1920, 1080), (1024, 768), (2560, 1080)):
            aspect = self.aspect if self.constrained else viewport[0] / viewport[1]
            for name, position in self.regions:
                with self.subTest(viewport=viewport, region=name):
                    delta = tuple(p - c for p, c in zip(position, self.origin))
                    self.assertLess(abs(dot(delta, self.right)) + 100, self.width / 2)
                    self.assertLess(abs(dot(delta, self.up)) + 100, self.width / (2 * aspect))

    def test_region_names_face_camera_and_read_left_to_right(self):
        rotation = re.search(r"Label->SetRelativeRotation\(FRotator\(([^)]+)\)\)", self.labels)
        normal, local_y, local_z = axes(vector(rotation[1]) if rotation else (0, 0, 0))
        self.assertGreater(dot(normal, tuple(-n for n in self.forward)), 0.99)
        # UE TextRender glyphs advance along local -Y; glyph tops are local +Z.
        self.assertGreater(dot(tuple(-n for n in local_y), self.right), 0.99)
        self.assertGreater(dot(local_z, self.up), 0.99)


if __name__ == "__main__":
    unittest.main(verbosity=2)
