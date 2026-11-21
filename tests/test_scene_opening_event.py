import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src/overlays/sc01_0077"
CC = os.environ.get("CC", "cc")


@unittest.skipUnless(shutil.which(CC), "a host C compiler is required")
class OpeningEventTests(unittest.TestCase):
    def test_middle_states_and_bypass(self):
        addresses = ("8017de84", "8017defc", "8017d98c", "80171b44", "8017df5c",
                     "8017dfd8", "80171b4c", "8012a018", "8012a048")
        with tempfile.TemporaryDirectory() as folder:
            exe = Path(folder) / "opening-event"
            subprocess.run(
                [CC, "-std=c11", "-Wall", "-Wextra", "-Werror", "-O2",
                 "-I", ROOT / "include", "-I", SCENE,
                 ROOT / "tests/scene_opening_event_probe.c",
                 *(SCENE / (address + ".c") for address in addresses),
                 # Both scene members have the same release-mode leaf.
                 ROOT / "src/overlays/sc02_0035/80147060.c",
                 ROOT / "src/main/80029124.c", ROOT / "src/main/80029178.c",
                 "-o", exe],
                check=True,
            )
            subprocess.run([exe], check=True)
